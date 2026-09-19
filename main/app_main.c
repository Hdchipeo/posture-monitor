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
#include <math.h>

#include "storage_manager.h"
#include "actuator_manager.h"
#include "mpu6050_sensor.h"
#include "posture_core.h"
#include "button_ctrl.h"
#include "telemetry.h"
#include "web_server.h"
#include "wifi_manager.h"
#include "status_led.h"
#include "battery_monitor.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"

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
                    mpu6050_sensor_reset_yaw();
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
        bool sensor_ok = (err == ESP_OK);

        if (!sensor_ok) {
            static int64_t s_last_sensor_warn = 0;
            int64_t now = esp_timer_get_time();
            if (now - s_last_sensor_warn > 3000000LL) {
                s_last_sensor_warn = now;
                ESP_LOGW(TAG, "Sensor update failed (code: %s). Check MPU6050 wiring (SDA=GPIO 8, SCL=GPIO 9, 3.3V).", esp_err_to_name(err));
            }
        }

        // 2. Handle calibration state if active (only if sensor healthy)
        if (sensor_ok && posture_core_is_calibrating()) {
            status_led_set_mode(STATUS_LED_MODE_CALIBRATING);
            posture_calib_data_t completed_calib;
            esp_err_t calib_status = posture_core_add_calibration_sample(&angles, &completed_calib);
            if (calib_status == ESP_OK) {
                // Calibration just finished: persist to NVS flash
                storage_manager_save_calibration(&completed_calib);
                actuator_manager_set_pattern(ALERT_PATTERN_CALIB_DONE);
                telemetry_record_event("calib", "Tare Calibrated", "Neutral zero baseline saved to NVS");
                ESP_LOGI(TAG, "Calibration stored to NVS: Pitch=%.2f deg, Roll=%.2f deg",
                         completed_calib.pitch_offset, completed_calib.roll_offset);
            }
        } else if (sensor_ok) {
            // 3. Process posture evaluation FSM
            posture_fsm_state_t prev_state = fsm_state;
            posture_core_process_sample(dt, &angles, &fsm_state);

            // Record posture transition events
            if (fsm_state != prev_state) {
                if (fsm_state == POSTURE_STATE_ALERT_L1) {
                    telemetry_record_event("warn", "Slouch Warning L1", "Haptic pulse active");
                } else if (fsm_state == POSTURE_STATE_ALERT_L2) {
                    telemetry_record_event("alert", "Slouch Alert L2", "Persistent slouch - Alarm active");
                } else if (fsm_state == POSTURE_STATE_GOOD &&
                           (prev_state == POSTURE_STATE_ALERT_L1 || prev_state == POSTURE_STATE_ALERT_L2 || prev_state == POSTURE_STATE_SUSPECTED_SLOUCH)) {
                    telemetry_record_event("good", "Posture Corrected", "Returned to neutral alignment");
                }
            }

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

            // Update Status LED mode independently
            if (battery_monitor_is_low()) {
                status_led_set_mode(STATUS_LED_MODE_LOW_BATTERY);
            } else {
                switch (fsm_state) {
                    case POSTURE_STATE_GOOD:
                        status_led_set_mode(STATUS_LED_MODE_HEARTBEAT);
                        break;

                    case POSTURE_STATE_SUSPECTED_SLOUCH:
                    case POSTURE_STATE_ALERT_L1:
                        status_led_set_mode(STATUS_LED_MODE_SLOUCH_WARN);
                        break;

                    case POSTURE_STATE_ALERT_L2:
                        status_led_set_mode(STATUS_LED_MODE_ALARM);
                        break;

                    case POSTURE_STATE_SNOOZED:
                        status_led_set_mode(STATUS_LED_MODE_SNOOZED);
                        break;

                    default:
                        status_led_set_mode(STATUS_LED_MODE_HEARTBEAT);
                        break;
                }
            }
        } else {
            // Sensor offline: silence vibration/buzzer and show alarm on LED
            actuator_manager_set_pattern(ALERT_PATTERN_IDLE);
            status_led_set_mode(STATUS_LED_MODE_ALARM);
        }

        // Periodic battery voltage sampling (every 1 second = 50 samples @ 50Hz)
        static uint32_t s_battery_sample_counter = 0;
        s_battery_sample_counter++;
        if (s_battery_sample_counter >= CONFIG_POSTURE_SAMPLING_RATE_HZ) {
            s_battery_sample_counter = 0;
            battery_monitor_sample();
        }

        // 5. Broadcast real-time telemetry snapshot to WebSocket clients at 10 Hz (every 5 sensor samples)
        static uint32_t telem_stream_counter = 0;
        telem_stream_counter++;
        if (telem_stream_counter >= (CONFIG_POSTURE_SAMPLING_RATE_HZ / 10)) {
            telem_stream_counter = 0;
            posture_calib_data_t active_calib;
            posture_core_get_calib(&active_calib);
            float d_pitch = fabsf(angles.pitch - active_calib.pitch_offset);
            float d_roll  = fabsf(angles.roll  - active_calib.roll_offset);
            float dev     = sqrtf(d_pitch * d_pitch + d_roll * d_roll);

            posture_telemetry_t telem = {
                .timestamp_ms = esp_timer_get_time() / 1000ULL,
                .state = fsm_state,
                .pitch = angles.pitch,
                .roll = angles.roll,
                .yaw = angles.yaw,
                .pitch_error = d_pitch,
                .roll_error = d_roll,
                .deviation = dev,
                .threshold = active_calib.angle_threshold,
                .alert_level = (fsm_state == POSTURE_STATE_ALERT_L2) ? 2 : ((fsm_state == POSTURE_STATE_ALERT_L1) ? 1 : 0),
                .is_calibrating = posture_core_is_calibrating(),
                .is_snoozed = (fsm_state == POSTURE_STATE_SNOOZED),
                .sensor_ok = sensor_ok,
                .battery_pct = battery_monitor_get_percentage(),
                .wifi_rssi = wifi_manager_get_rssi(),
                .free_heap = (uint32_t)esp_get_free_heap_size(),
                .uptime_s = (uint32_t)(esp_timer_get_time() / 1000000ULL)
            };

            telemetry_update_snapshot(&telem);
            web_server_broadcast_telemetry(&telem);
        }

        // 6. Periodic telemetry log (every 5 seconds)
        report_counter++;
        if (report_counter >= (CONFIG_POSTURE_SAMPLING_RATE_HZ * 5)) {
            report_counter = 0;
            posture_calib_data_t active_calib;
            posture_core_get_calib(&active_calib);
            ESP_LOGI(TAG, "STATUS | Roll (Cui/Ngua): %5.1f | Pitch (Nghieng): %5.1f | Yaw (Xoay): %5.1f | State: %d | Bat: %u%% | Heap: %lu B",
                     angles.roll, angles.pitch, angles.yaw,
                     fsm_state, (unsigned int)battery_monitor_get_percentage(),
                     (unsigned long)esp_get_free_heap_size());
        }
    }
}

void app_main(void) {
    const esp_app_desc_t *app_desc = esp_app_get_description();
    ESP_LOGI(TAG, "============================================================");
    ESP_LOGI(TAG, "   ESP32-C3 Posture Monitor & Alert System v%s", app_desc->version);
    ESP_LOGI(TAG, "   Built: %s %s | IDF: %s", app_desc->date, app_desc->time, app_desc->idf_ver);
    ESP_LOGI(TAG, "============================================================");

    // Validate current OTA running partition and cancel rollback
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "First boot of newly flashed OTA partition [%s]. Validating...", running->label);
            esp_ota_mark_app_valid_cancel_rollback();
            ESP_LOGI(TAG, "OTA partition [%s] marked VALID. Rollback cancelled.", running->label);
        } else {
            ESP_LOGI(TAG, "Running on active OTA partition [%s] (state: 0x%02x)", running->label, ota_state);
        }
    } else {
        ESP_LOGI(TAG, "Running on partition [%s]", running ? running->label : "unknown");
    }

    // 1. Create inter-task event queue
    s_event_queue = xQueueCreate(8, sizeof(app_event_type_t));

    // 2. Initialize NVS Storage Manager
    ESP_ERROR_CHECK(storage_manager_init());

    posture_calib_data_t calib_cfg;
    storage_manager_load_calibration(&calib_cfg);

    // 3. Initialize Actuators and Status LED
    ESP_ERROR_CHECK(actuator_manager_init());
    ESP_ERROR_CHECK(status_led_init());

    // 4. Initialize Battery Monitor (ADC1 GPIO 1 with 100k-100k divider)
    ESP_ERROR_CHECK(battery_monitor_init());

    // 5. Initialize MPU6050 Sensor Driver
    esp_err_t sensor_err = mpu6050_sensor_init();
    if (sensor_err != ESP_OK) {
        ESP_LOGE(TAG, "CRITICAL: MPU6050 initialization failed! Halting startup.");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // 6. Initialize Posture Core FSM
    ESP_ERROR_CHECK(posture_core_init(&calib_cfg));

    // 7. Initialize Button Controller
    ESP_ERROR_CHECK(button_ctrl_init(on_button_action, NULL));

    // 8. Initialize Telemetry Engine
    ESP_ERROR_CHECK(telemetry_init());

    // 9. Launch main posture monitoring task immediately (Sensing, LED & Haptics run first)
    xTaskCreatePinnedToCore(posture_monitor_task, "posture_task", 4096, NULL, 5, NULL, 0);

    // 10. Start Wi-Fi SoftAP and Web Server
    ESP_ERROR_CHECK(wifi_manager_init_softap());
    ESP_ERROR_CHECK(web_server_start());

    ESP_LOGI(TAG, "System operational. Connect phone to 'Posture-Monitor-AP' and visit http://192.168.4.1");
}
