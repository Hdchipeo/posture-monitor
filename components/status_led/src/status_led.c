/**
 * @file status_led.c
 * @brief Non-blocking Status LED Driver using esp_timer.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#include "status_led.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "STATUS_LED";

#ifndef CONFIG_POSTURE_STATUS_LED_GPIO
#define CONFIG_POSTURE_STATUS_LED_GPIO 5
#endif

#ifndef CONFIG_POSTURE_STATUS_LED_ACTIVE_LEVEL
#define CONFIG_POSTURE_STATUS_LED_ACTIVE_LEVEL 1
#endif

static status_led_mode_t s_current_mode = STATUS_LED_MODE_SOLID;
static esp_timer_handle_t s_led_timer = NULL;
static uint32_t s_tick_counter = 0;
static uint32_t s_boot_post_ticks = 40; // Power-On Self Test: 2 seconds (40 * 50ms) solid ON

static inline void set_led_level(bool on) {
#if CONFIG_POSTURE_STATUS_LED_ACTIVE_LEVEL
    gpio_set_level((gpio_num_t)CONFIG_POSTURE_STATUS_LED_GPIO, on ? 1 : 0);
#else
    gpio_set_level((gpio_num_t)CONFIG_POSTURE_STATUS_LED_GPIO, on ? 0 : 1);
#endif
}

static void status_led_timer_cb(void *arg) {
    s_tick_counter++;

    // Power-On Self Test (POST): 2 seconds (40 ticks @ 50ms)
    // Ticks 40..21 (first 1000ms): Force GPIO output 3.3V (HIGH) directly
    // Ticks 20..1  (second 1000ms): Force GPIO output 0V (LOW) directly
    // This physically tests both Active-HIGH and Active-LOW LED connections at boot.
    if (s_boot_post_ticks > 0) {
        if (s_boot_post_ticks > 20) {
            gpio_set_level((gpio_num_t)CONFIG_POSTURE_STATUS_LED_GPIO, 1);
        } else {
            gpio_set_level((gpio_num_t)CONFIG_POSTURE_STATUS_LED_GPIO, 0);
        }
        s_boot_post_ticks--;
        if (s_boot_post_ticks == 0) {
            s_current_mode = STATUS_LED_MODE_HEARTBEAT;
            s_tick_counter = 0;
            ESP_LOGI(TAG, "POST complete, entering operational mode (Active %s)",
                     CONFIG_POSTURE_STATUS_LED_ACTIVE_LEVEL ? "HIGH" : "LOW");
        }
        return;
    }

    switch (s_current_mode) {
        case STATUS_LED_MODE_OFF:
            set_led_level(false);
            break;

        case STATUS_LED_MODE_SOLID:
            set_led_level(true);
            break;

        case STATUS_LED_MODE_HEARTBEAT: {
            // Prominent visible heartbeat double-pulse (2000ms cycle = 40 ticks @ 50ms)
            // Lub: ticks 0..2 (150ms ON), Pause: ticks 3..4 (100ms OFF)
            // Dub: ticks 5..7 (150ms ON), Rest: ticks 8..39 (1600ms OFF)
            uint32_t sub = s_tick_counter % 40;
            bool on = (sub <= 2) || (sub >= 5 && sub <= 7);
            set_led_level(on);
            break;
        }

        case STATUS_LED_MODE_CALIBRATING: {
            // 5 Hz blink (100ms ON, 100ms OFF = 4 ticks cycle)
            uint32_t sub = s_tick_counter % 4;
            set_led_level(sub < 2);
            break;
        }

        case STATUS_LED_MODE_SLOUCH_WARN: {
            // 2 Hz blink (250ms ON, 250ms OFF = 10 ticks cycle)
            uint32_t sub = s_tick_counter % 10;
            set_led_level(sub < 5);
            break;
        }

        case STATUS_LED_MODE_ALARM: {
            // 10 Hz rapid strobe (50ms ON, 50ms OFF = 2 ticks cycle)
            uint32_t sub = s_tick_counter % 2;
            set_led_level(sub == 0);
            break;
        }

        case STATUS_LED_MODE_LOW_BATTERY: {
            // Triple blink: 150ms ON, 100ms OFF x 3, then 1250ms OFF
            uint32_t sub = s_tick_counter % 40;
            bool on = (sub <= 2) || (sub >= 5 && sub <= 7) || (sub >= 10 && sub <= 12);
            set_led_level(on);
            break;
        }

        case STATUS_LED_MODE_SNOOZED: {
            // Slow pulse: 500ms ON (10 ticks), 1500ms OFF (30 ticks)
            uint32_t sub = s_tick_counter % 40;
            set_led_level(sub < 10);
            break;
        }

        default:
            set_led_level(false);
            break;
    }
}

esp_err_t status_led_init(void) {
    // 1. Explicitly reset pin to disconnect JTAG MTDI peripheral multiplexer
    gpio_reset_pin((gpio_num_t)CONFIG_POSTURE_STATUS_LED_GPIO);

    // 2. Configure pin as output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_POSTURE_STATUS_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED GPIO %d: %s", CONFIG_POSTURE_STATUS_LED_GPIO, esp_err_to_name(err));
        return err;
    }

    // 3. Maximize drive capability (up to ~40mA source/sink)
    gpio_set_drive_capability((gpio_num_t)CONFIG_POSTURE_STATUS_LED_GPIO, GPIO_DRIVE_CAP_3);

    // 4. Ensure sleep mode does not isolate this pin
    gpio_sleep_sel_dis((gpio_num_t)CONFIG_POSTURE_STATUS_LED_GPIO);

    // 5. Start Power-On Self Test: Begin with Phase 1 (HIGH)
    s_current_mode = STATUS_LED_MODE_SOLID;
    s_boot_post_ticks = 40;
    gpio_set_level((gpio_num_t)CONFIG_POSTURE_STATUS_LED_GPIO, 1);

    const esp_timer_create_args_t timer_args = {
        .callback = &status_led_timer_cb,
        .name = "led_seq",
        .dispatch_method = ESP_TIMER_TASK,
    };

    err = esp_timer_create(&timer_args, &s_led_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LED timer: %s", esp_err_to_name(err));
        return err;
    }

    // 50ms periodic ticker (50,000 microseconds)
    err = esp_timer_start_periodic(s_led_timer, 50000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start periodic LED timer: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Status LED initialized on GPIO %d (Active %s). POST: 1s HIGH (3.3V) + 1s LOW (0V)",
             CONFIG_POSTURE_STATUS_LED_GPIO,
             CONFIG_POSTURE_STATUS_LED_ACTIVE_LEVEL ? "HIGH" : "LOW");
    return ESP_OK;
}

void status_led_set_mode(status_led_mode_t mode) {
    // If still in boot POST test, do not override
    if (s_boot_post_ticks > 0) {
        return;
    }

    if (s_current_mode != mode) {
        s_current_mode = mode;
        s_tick_counter = 0; // Reset cadence phase for snappy transition
        if (mode == STATUS_LED_MODE_OFF) {
            set_led_level(false);
        } else if (mode == STATUS_LED_MODE_SOLID) {
            set_led_level(true);
        }
    }
}

status_led_mode_t status_led_get_mode(void) {
    return s_current_mode;
}
