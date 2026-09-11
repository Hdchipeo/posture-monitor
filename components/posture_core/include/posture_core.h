/**
 * @file posture_core.h
 * @brief Finite State Machine (FSM) and posture evaluation engine.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "storage_manager.h"
#include "mpu6050_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    POSTURE_STATE_CALIBRATING = 0, /*!< In zero-offset calibration mode */
    POSTURE_STATE_GOOD,            /*!< Posture orientation within acceptable threshold */
    POSTURE_STATE_SUSPECTED_SLOUCH,/*!< Angular deviation detected, grace timer running */
    POSTURE_STATE_ALERT_L1,        /*!< Slouch grace exceeded, Level 1 haptic alert active */
    POSTURE_STATE_ALERT_L2,        /*!< Persistent slouch, Level 2 audio-haptic alarm active */
    POSTURE_STATE_SNOOZED          /*!< User temporarily silenced monitoring */
} posture_fsm_state_t;

/**
 * @brief Initialize the posture evaluation engine with loaded calibration.
 *
 * @param[in] initial_calib Loaded or default calibration parameters.
 * @return ESP_OK on success.
 */
esp_err_t posture_core_init(const posture_calib_data_t *initial_calib);

/**
 * @brief Process an IMU orientation sample and step the FSM.
 *
 * @param[in]  dt             Time step in seconds.
 * @param[in]  current_angles Latest pitch and roll angles.
 * @param[out] out_state      Updated FSM state.
 */
void posture_core_process_sample(float dt, const posture_angles_t *current_angles, posture_fsm_state_t *out_state);

/**
 * @brief Initiate a zero-reference baseline calibration cycle.
 */
void posture_core_start_calibration(void);

/**
 * @brief Query if the core is currently collecting calibration samples.
 *
 * @return true if calibration in progress.
 */
bool posture_core_is_calibrating(void);

/**
 * @brief Feed a sample during calibration mode.
 *
 * @param[in]  angles    Sample angles.
 * @param[out] out_calib Updated calibration if calibration completes with this sample.
 * @return
 *      - ESP_OK: Calibration completed.
 *      - ESP_ERR_NOT_FINISHED: Still collecting samples.
 */
esp_err_t posture_core_add_calibration_sample(const posture_angles_t *angles, posture_calib_data_t *out_calib);

/**
 * @brief Snooze the alert manager for the given duration in seconds.
 *
 * @param[in] seconds Snooze duration.
 */
void posture_core_snooze(uint32_t seconds);

/**
 * @brief Get copy of active calibration data.
 *
 * @param[out] out_calib Destination pointer.
 */
void posture_core_get_calib(posture_calib_data_t *out_calib);

/**
 * @brief Update active calibration parameters.
 *
 * @param[in] new_calib New parameters to adopt.
 */
void posture_core_update_calib(const posture_calib_data_t *new_calib);

#ifdef __cplusplus
}
#endif
