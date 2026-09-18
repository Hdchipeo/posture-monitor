/**
 * @file mpu6050_sensor.h
 * @brief High-reliability MPU6050 IMU driver and posture orientation filter.
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

typedef struct {
    float pitch;    /*!< Forward / Backward inclination angle in degrees */
    float roll;     /*!< Lateral (Left / Right) inclination angle in degrees */
    float yaw_rate; /*!< Z-axis rotational rate in degrees per second */
} posture_angles_t;

/**
 * @brief Initialize I2C bus and the MPU6050 sensor from ESP Component Registry.
 *        Includes I2C bus lockup recovery routine.
 *
 * @return ESP_OK if communication is established and sensor is configured.
 */
esp_err_t mpu6050_sensor_init(void);

/**
 * @brief Poll the IMU and update the complementary filter orientation angles.
 *
 * @param[in]  dt                Time delta in seconds since last update.
 * @param[in]  freeze_accel_bias If true (e.g. while motor is vibrating), ignore dynamic accel spikes.
 * @param[out] out_angles        Computed Euler angles in degrees.
 * @return ESP_OK on successful sample and calculation.
 */
esp_err_t mpu6050_sensor_update(float dt, bool freeze_accel_bias, posture_angles_t *out_angles);

/**
 * @brief Check if MPU6050 communication is currently active and healthy.
 *
 * @return true if communicating without recent bus errors.
 */
bool mpu6050_sensor_is_healthy(void);

/**
 * @brief Hardware deinit / put sensor to low-power sleep mode.
 *
 * @return ESP_OK on success.
 */
esp_err_t mpu6050_sensor_sleep(void);

#ifdef __cplusplus
}
#endif
