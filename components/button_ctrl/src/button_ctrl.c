/**
 * @file button_ctrl.c
 * @brief Pushbutton controller implementation integrating espressif/button.
 */

#include "button_ctrl.h"
#include "iot_button.h"
#include "button_gpio.h"
#include "esp_log.h"

static const char *TAG = "BUTTON_CTRL";

#ifndef CONFIG_POSTURE_BUTTON_GPIO
#define CONFIG_POSTURE_BUTTON_GPIO 9
#endif

static button_event_cb_t s_user_cb = NULL;
static void *s_user_ctx = NULL;
static button_handle_t s_btn_handle = NULL;

static void button_single_click_cb(void *button_handle, void *usr_data) {
    ESP_LOGI(TAG, "Button Single Click detected");
    if (s_user_cb) {
        s_user_cb(BUTTON_EVENT_SINGLE_CLICK, s_user_ctx);
    }
}

static void button_double_click_cb(void *button_handle, void *usr_data) {
    ESP_LOGI(TAG, "Button Double Click detected (Snooze request)");
    if (s_user_cb) {
        s_user_cb(BUTTON_EVENT_SNOOZE_10MIN, s_user_ctx);
    }
}

static void button_long_press_cb(void *button_handle, void *usr_data) {
    ESP_LOGI(TAG, "Button Long Press detected (>2s) -> Zero Calibration Tare requested");
    if (s_user_cb) {
        s_user_cb(BUTTON_EVENT_TARE_CALIBRATE, s_user_ctx);
    }
}

esp_err_t button_ctrl_init(button_event_cb_t callback, void *user_data) {
    s_user_cb = callback;
    s_user_ctx = user_data;

    button_config_t btn_cfg = {
        .long_press_time = 2000,
        .short_press_time = 180,
    };

    button_gpio_config_t gpio_cfg = {
        .gpio_num = CONFIG_POSTURE_BUTTON_GPIO,
        .active_level = 0,
        .enable_power_save = true,
    };

    esp_err_t err = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &s_btn_handle);
    if (err != ESP_OK || !s_btn_handle) {
        ESP_LOGE(TAG, "Failed to create gpio button device: %s", esp_err_to_name(err));
        return err;
    }

    button_event_args_t double_click_args = {
        .multiple_clicks = { .clicks = 2 }
    };

    button_event_args_t long_press_args = {
        .long_press = { .press_time = 2000 }
    };

    iot_button_register_cb(s_btn_handle, BUTTON_SINGLE_CLICK, NULL, button_single_click_cb, NULL);
    iot_button_register_cb(s_btn_handle, BUTTON_MULTIPLE_CLICK, &double_click_args, button_double_click_cb, NULL);
    iot_button_register_cb(s_btn_handle, BUTTON_LONG_PRESS_START, &long_press_args, button_long_press_cb, NULL);

    ESP_LOGI(TAG, "Button controller initialized on GPIO %d (Active LOW)", CONFIG_POSTURE_BUTTON_GPIO);
    return ESP_OK;
}
