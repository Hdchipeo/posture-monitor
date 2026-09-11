/**
 * @file app_main.c
 * @brief Main application orchestrator for the ESP32-C3 Posture Monitor.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "storage_manager.h"
#include "actuator_manager.h"
#include "mpu6050_sensor.h"
#include "posture_core.h"
#include "button_ctrl.h"

static const char *TAG = "POSTURE_MAIN";

#ifndef CONFIG_POSTURE_SAMPLING_RATE_HZ
#define CONFIG_POSTURE_SAMPLING_RATE_HZ 50
#endif

typedef enum {
    APP_EVENT_BUTTON_TARE,
    APP_EVENT_BUTTON_SNOOZE,
    APP_EVENT_BUTTON_CLICK
} app_event_type_t;

static QueueHandle_t s_event_queue = NULL;

static void on_button_action(button_action_event_t event, void *user_data) {
    app_event_type_t app_event;
    switch (event) {
        case BUTTON_EVENT_TARE_CALIBRATE:
            app_event = APP_EVENT_BUTTON_TARE;
            break;
        case BUTTON_EVENT_SNOOZE_10MIN:
            app_event = APP_EVENT_BUTTON_SNOOZE;
            break;
        case BUTTON_EVENT_SINGLE_CLICK:
        default:
            app_event = APP_EVENT_BUTTON_CLICK;
            break;
    }
    if (s_event_queue) {
        xQueueSend(s_event_queue, &app_event, 0);
    }
}

static void posture_monitor_task(void *pvParameters) {
    const float dt = 1.0f / (float)CONFIG_POSTURE_SAMPLING_RATE_HZ;
    const TickType_t xPeriod = pdMS_TO_TICKS(1000 / CONFIG_POSTURE_SAMPLING_RATE_HZ);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    posture_angles_t angles = {0};
    posture_fsm_state_t fsm_state = POSTURE_STATE_GOOD;
    uint32_t report_counter = 0;

    ESP_LOGI(TAG, "Posture monitor task running at %d Hz (dt = %.3f s)",
             CONFIG_POSTURE_SAMPLING_RATE_HZ, dt);

    while (1) {
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        // Process button events from queue
        app_event_type_t evt;
        while (xQueueReceive(s_event_queue, &evt, 0) == pdTRUE) {
            switch (evt) {
                case APP_EVENT_BUTTON_TARE:
                    ESP_LOGI(TAG, "Initiating Tare Calibration requested by button");
                    actuator_manager_set_pattern(ALERT_PATTERN_CALIB_START);
                    posture_core_start_calibration();
                    break;
                case APP_EVENT_BUTTON_SNOOZE:
                    ESP_LOGI(TAG, "Snooze 10 minutes requested");
                    posture_core_snooze(600);
                    actuator_manager_set_pattern(ALERT_PATTERN_IDLE);
                    break;
                case APP_EVENT_BUTTON_CLICK:
                    ESP_LOGI(TAG, "Single click received");
                    actuator_manager_set_pattern(ALERT_PATTERN_IDLE);
                    break;
            }
        }

        // 1. Read IMU with motor vibration noise decoupling
        bool motor_active = actuator_manager_is_vibrating();
        esp_err_t err = mpu6050_sensor_update(dt, motor_active, &angles);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Sensor update failed (code: %s), skipping step", esp_err_to_name(err));
            continue;
        }

        // 2. Handle calibration state if active
        if (posture_core_is_calibrating()) {
            posture_calib_data_t completed_calib;
            esp_err_t calib_status = posture_core_add_calibration_sample(&angles, &completed_calib);
            if (calib_status == ESP_OK) {
                // Calibration just finished: persist to NVS flash
                storage_manager_save_calibration(&completed_calib);
                actuator_manager_set_pattern(ALERT_PATTERN_CALIB_DONE);
                ESP_LOGI(TAG, "Calibration stored to NVS: Pitch=%.2f deg, Roll=%.2f deg",
                         completed_calib.pitch_offset, completed_calib.roll_offset);
            }
            continue;
        }

        // 3. Process posture evaluation FSM
        posture_core_process_sample(dt, &angles, &fsm_state);

        // 4. Update actuator patterns based on FSM state
        switch (fsm_state) {
            case POSTURE_STATE_GOOD:
            case POSTURE_STATE_SUSPECTED_SLOUCH:
            case POSTURE_STATE_SNOOZED:
                actuator_manager_set_pattern(ALERT_PATTERN_IDLE);
                break;

            case POSTURE_STATE_ALERT_L1:
                actuator_manager_set_pattern(ALERT_PATTERN_LEVEL1_HAPTIC);
                break;

            case POSTURE_STATE_ALERT_L2:
                actuator_manager_set_pattern(ALERT_PATTERN_LEVEL2_ALARM);
                break;

            default:
                break;
        }

        // 5. Periodic telemetry log (every 5 seconds)
        report_counter++;
        if (report_counter >= (CONFIG_POSTURE_SAMPLING_RATE_HZ * 5)) {
            report_counter = 0;
            posture_calib_data_t active_calib;
            posture_core_get_calib(&active_calib);
            ESP_LOGI(TAG, "STATUS | Pitch: %6.1f deg (Delta: %5.1f) | Roll: %6.1f deg (Delta: %5.1f) | State: %d | FreeHeap: %lu bytes",
                     angles.pitch, angles.pitch - active_calib.pitch_offset,
                     angles.roll, angles.roll - active_calib.roll_offset,
                     fsm_state, (unsigned long)esp_get_free_heap_size());
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "============================================================");
    ESP_LOGI(TAG, "       ESP32-C3 Posture Monitor & Alert System v1.0         ");
    ESP_LOGI(TAG, "============================================================");

    // 1. Create inter-task event queue
    s_event_queue = xQueueCreate(8, sizeof(app_event_type_t));

    // 2. Initialize NVS Storage Manager
    ESP_ERROR_CHECK(storage_manager_init());

    posture_calib_data_t calib_cfg;
    storage_manager_load_calibration(&calib_cfg);

    // 3. Initialize Actuator Manager
    ESP_ERROR_CHECK(actuator_manager_init());

    // 4. Initialize MPU6050 Sensor Driver
    esp_err_t sensor_err = mpu6050_sensor_init();
    if (sensor_err != ESP_OK) {
        ESP_LOGE(TAG, "CRITICAL: MPU6050 initialization failed! Halting startup.");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // 5. Initialize Posture Core FSM
    ESP_ERROR_CHECK(posture_core_init(&calib_cfg));

    // 6. Initialize Button Controller
    ESP_ERROR_CHECK(button_ctrl_init(on_button_action, NULL));

    // 7. Launch main posture monitoring task
    // Stack: 3584 bytes, Priority: 5
    xTaskCreatePinnedToCore(posture_monitor_task, "posture_task", 3584, NULL, 5, NULL, 0);

    ESP_LOGI(TAG, "System operational. Press and hold button (>2s) to Tare calibration.");
}
