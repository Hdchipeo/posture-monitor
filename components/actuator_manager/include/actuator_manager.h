/**
 * @file actuator_manager.h
 * @brief Non-blocking actuator sequencer for vibration motor and buzzer.
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
    ALERT_PATTERN_IDLE = 0,        /*!< All actuators turned off */
    ALERT_PATTERN_CALIB_START,     /*!< Single short buzz indicating calibration started */
    ALERT_PATTERN_CALIB_DONE,      /*!< Two crisp pulses indicating calibration completed */
    ALERT_PATTERN_LEVEL1_HAPTIC,   /*!< Gentle intermittent haptic pulse (every 1.2s) */
    ALERT_PATTERN_LEVEL2_ALARM     /*!< Aggressive vibration + synchronous buzzer beeps */
} alert_pattern_t;

/**
 * @brief Initialize GPIO pins and high-resolution timer for actuator sequencing.
 *
 * @return ESP_OK on success, or driver initialization error.
 */
esp_err_t actuator_manager_init(void);

/**
 * @brief Set the active alert pattern. Operation is non-blocking.
 *
 * @param[in] pattern Desired alert pattern.
 */
void actuator_manager_set_pattern(alert_pattern_t pattern);

/**
 * @brief Query whether the vibration motor is currently actively pulsing.
 *        Used by posture filter to freeze/attenuate accelerometer noise coupling.
 *
 * @return true if motor is physically vibrating right now.
 */
bool actuator_manager_is_vibrating(void);

#ifdef __cplusplus
}
#endif
