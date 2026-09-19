/**
 * @file status_led.h
 * @brief Non-blocking Status LED Driver for ESP32-C3 on GPIO 5.
 *
 * Provides visual indication patterns including heartbeat, calibration,
 * slouch warning, high-priority alarm, and low-battery alerts.
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
    STATUS_LED_MODE_OFF = 0,         /*!< LED strictly turned off */
    STATUS_LED_MODE_SOLID,           /*!< Steady ON */
    STATUS_LED_MODE_HEARTBEAT,       /*!< Subtle 50ms heartbeat pulse every 2s (Normal Good Posture) */
    STATUS_LED_MODE_CALIBRATING,     /*!< Fast 5 Hz flashing (Tare Zero Calibration in progress) */
    STATUS_LED_MODE_SLOUCH_WARN,     /*!< Moderate 2 Hz flashing (Level 1 Slouch Warning) */
    STATUS_LED_MODE_ALARM,           /*!< Rapid 10 Hz strobe (Level 2 Slouch Alarm) */
    STATUS_LED_MODE_LOW_BATTERY,     /*!< Triple blink alert every 2 seconds (< 15% Battery) */
    STATUS_LED_MODE_SNOOZED          /*!< Slow relaxed pulse (Monitoring Snoozed) */
} status_led_mode_t;

/**
 * @brief Initialize GPIO 5 as digital output and launch background cadence timer.
 *
 * @return ESP_OK on success, or GPIO/timer initialization error code.
 */
esp_err_t status_led_init(void);

/**
 * @brief Set the active visual status pattern. Non-blocking operation.
 *
 * @param[in] mode Desired LED indication mode.
 */
void status_led_set_mode(status_led_mode_t mode);

/**
 * @brief Query current LED mode.
 *
 * @return Active status_led_mode_t.
 */
status_led_mode_t status_led_get_mode(void);

#ifdef __cplusplus
}
#endif
