/**
 * @file battery_monitor.h
 * @brief Precision Battery Voltage and Percentage Monitor for ESP32-C3.
 *
 * Measures cell voltage via a 100k - 100k resistor divider on ADC1 Channel 1 (GPIO 1).
 * Employs hardware eFuse calibration, 16x multisampling, IIR/EMA low-pass filtering,
 * and piecewise linear State-of-Charge (SoC) interpolation.
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

/**
 * @brief Initialize the ADC1 oneshot driver, attenuation, and eFuse calibration.
 *
 * @return ESP_OK on success, or appropriate error code.
 */
esp_err_t battery_monitor_init(void);

/**
 * @brief Sample battery voltage with 16x oversampling and update filtered estimate.
 *        Thread-safe. Recommended to call every 1-2 seconds.
 *
 * @return ESP_OK on success.
 */
esp_err_t battery_monitor_sample(void);

/**
 * @brief Get the latest estimated battery percentage (0 to 100%).
 *
 * @return uint8_t Battery percentage [0..100].
 */
uint8_t battery_monitor_get_percentage(void);

/**
 * @brief Get the latest filtered battery cell voltage in millivolts.
 *
 * @return uint32_t Cell voltage in mV (e.g. 3700 mV for 3.7V).
 */
uint32_t battery_monitor_get_voltage_mv(void);

/**
 * @brief Check if battery level is below low-battery threshold (< 15%).
 *
 * @return true if low battery condition is detected.
 */
bool battery_monitor_is_low(void);

#ifdef __cplusplus
}
#endif
