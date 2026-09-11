/**
 * @file actuator_manager.c
 * @brief Non-blocking actuator sequencer using high-resolution esp_timer.
 */

#include "actuator_manager.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "ACTUATOR_MGR";

#ifndef CONFIG_POSTURE_VIBRATION_GPIO
#define CONFIG_POSTURE_VIBRATION_GPIO 18
#endif

#ifndef CONFIG_POSTURE_BUZZER_GPIO
#define CONFIG_POSTURE_BUZZER_GPIO 19
#endif

#if CONFIG_IDF_TARGET_ESP32
#if (CONFIG_POSTURE_VIBRATION_GPIO >= 6 && CONFIG_POSTURE_VIBRATION_GPIO <= 11)
#error "CONFIG_POSTURE_VIBRATION_GPIO cannot use GPIO 6-11 on ESP32! These pins are dedicated to SPI Flash."
#endif
#if (CONFIG_POSTURE_BUZZER_GPIO >= 6 && CONFIG_POSTURE_BUZZER_GPIO <= 11)
#error "CONFIG_POSTURE_BUZZER_GPIO cannot use GPIO 6-11 on ESP32! These pins are dedicated to SPI Flash."
#endif
#endif

#ifndef CONFIG_POSTURE_ENABLE_BUZZER
#define CONFIG_POSTURE_ENABLE_BUZZER 1
#endif

static alert_pattern_t s_current_pattern = ALERT_PATTERN_IDLE;
static esp_timer_handle_t s_timer_handle = NULL;
static uint32_t s_tick_counter = 0;
static bool s_motor_state = false;

static inline void set_motor(bool on) {
    s_motor_state = on;
    gpio_set_level(CONFIG_POSTURE_VIBRATION_GPIO, on ? 1 : 0);
}

static inline void set_buzzer(bool on) {
#if CONFIG_POSTURE_ENABLE_BUZZER
    gpio_set_level(CONFIG_POSTURE_BUZZER_GPIO, on ? 1 : 0);
#else
    (void)on;
    gpio_set_level(CONFIG_POSTURE_BUZZER_GPIO, 0);
#endif
}

static void actuator_timer_cb(void *arg) {
    s_tick_counter++;

    switch (s_current_pattern) {
        case ALERT_PATTERN_IDLE:
            set_motor(false);
            set_buzzer(false);
            break;

        case ALERT_PATTERN_CALIB_START:
            // Single pulse: 200ms ON, then back to IDLE
            if (s_tick_counter <= 2) {
                set_motor(true);
            } else {
                set_motor(false);
                s_current_pattern = ALERT_PATTERN_IDLE;
            }
            set_buzzer(false);
            break;

        case ALERT_PATTERN_CALIB_DONE:
            // Double pulse: 150ms ON, 150ms OFF, 150ms ON, then IDLE
            if (s_tick_counter == 1 || s_tick_counter == 2) {
                set_motor(true);
            } else if (s_tick_counter == 3) {
                set_motor(false);
            } else if (s_tick_counter == 4 || s_tick_counter == 5) {
                set_motor(true);
            } else {
                set_motor(false);
                s_current_pattern = ALERT_PATTERN_IDLE;
            }
            set_buzzer(false);
            break;

        case ALERT_PATTERN_LEVEL1_HAPTIC: {
            // Repeat cycle of 12 ticks (1200ms):
            // Tick 1-2: ON (200ms), Tick 3-12: OFF (1000ms)
            uint32_t sub_tick = s_tick_counter % 12;
            if (sub_tick < 2) {
                set_motor(true);
            } else {
                set_motor(false);
            }
            set_buzzer(false);
            break;
        }

        case ALERT_PATTERN_LEVEL2_ALARM: {
            // Repeat cycle of 5 ticks (500ms):
            // Tick 1-2: ON (200ms), Tick 3-5: OFF (300ms)
            uint32_t sub_tick = s_tick_counter % 5;
            if (sub_tick < 2) {
                set_motor(true);
                set_buzzer(true);
            } else {
                set_motor(false);
                set_buzzer(false);
            }
            break;
        }

        default:
            set_motor(false);
            set_buzzer(false);
            break;
    }
}

esp_err_t actuator_manager_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_POSTURE_VIBRATION_GPIO) | (1ULL << CONFIG_POSTURE_BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure actuator GPIOs: %s", esp_err_to_name(err));
        return err;
    }

    set_motor(false);
    set_buzzer(false);

    const esp_timer_create_args_t timer_args = {
        .callback = &actuator_timer_cb,
        .name = "actuator_seq",
        .dispatch_method = ESP_TIMER_TASK,
    };

    err = esp_timer_create(&timer_args, &s_timer_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create actuator timer: %s", esp_err_to_name(err));
        return err;
    }

    // 100ms periodic cadence (100,000 microseconds)
    err = esp_timer_start_periodic(s_timer_handle, 100000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start periodic actuator timer: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Actuator manager initialized on Motor GPIO %d, Buzzer GPIO %d",
             CONFIG_POSTURE_VIBRATION_GPIO, CONFIG_POSTURE_BUZZER_GPIO);
    return ESP_OK;
}

void actuator_manager_set_pattern(alert_pattern_t pattern) {
    if (s_current_pattern != pattern) {
        s_current_pattern = pattern;
        s_tick_counter = 0; // Reset phase for instant response
        if (pattern == ALERT_PATTERN_IDLE) {
            set_motor(false);
            set_buzzer(false);
        }
    }
}

bool actuator_manager_is_vibrating(void) {
    return s_motor_state;
}
