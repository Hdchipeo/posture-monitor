/**
 * @file button_ctrl.h
 * @brief Pushbutton interface wrapping espressif/button for posture calibration and snooze.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BUTTON_EVENT_TARE_CALIBRATE,  /*!< User held button for >2 seconds to trigger zero calibration */
    BUTTON_EVENT_SNOOZE_10MIN,    /*!< User double-clicked button to snooze */
    BUTTON_EVENT_SINGLE_CLICK     /*!< User single-clicked to inspect/stop immediate alert */
} button_action_event_t;

typedef void (*button_event_cb_t)(button_action_event_t event, void *user_data);

/**
 * @brief Initialize the button controller using espressif/button driver.
 *
 * @param[in] callback Function invoked when single, double or long press occurs.
 * @param[in] user_data Optional user context passed to callback.
 * @return ESP_OK on success.
 */
esp_err_t button_ctrl_init(button_event_cb_t callback, void *user_data);

#ifdef __cplusplus
}
#endif
