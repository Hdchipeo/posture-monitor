/**
 * @file storage_manager.h
 * @brief Non-Volatile Storage (NVS) manager with CRC32 integrity verification for Posture Monitor.
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

#define STORAGE_PAYLOAD_MAGIC 0x504F5354 // "POST"

/**
 * @brief Persistent calibration and configuration structure.
 */
typedef struct {
    uint32_t magic;               /*!< Magic word to verify struct validity */
    float    pitch_offset;        /*!< Calibrated neutral pitch angle (degrees) */
    float    roll_offset;         /*!< Calibrated neutral roll angle (degrees) */
    float    angle_threshold;     /*!< Angle deviation trigger threshold (degrees) */
    uint32_t slouch_delay_s;      /*!< Slouch tolerance window before L1 alert (seconds) */
    uint32_t escalation_delay_s;  /*!< Window before L2 buzzer alarm (seconds) */
    uint32_t crc32;               /*!< CRC32 checksum over the preceding fields */
} __attribute__((packed)) posture_calib_data_t;

/**
 * @brief Initialize NVS storage and ensure flash partition readiness.
 *
 * @return
 *      - ESP_OK: Initialized successfully.
 *      - ESP_FAIL: Failed to initialize NVS.
 */
esp_err_t storage_manager_init(void);

/**
 * @brief Load calibration data from NVS. If no entry exists or CRC is invalid,
 *        fallback values are loaded and saved automatically.
 *
 * @param[out] out_data Pointer to receiving struct.
 * @return
 *      - ESP_OK: Data read and validated.
 *      - ESP_ERR_INVALID_CRC: Corrupted data recovered with defaults.
 */
esp_err_t storage_manager_load_calibration(posture_calib_data_t *out_data);

/**
 * @brief Compute CRC32 and persist calibration data into NVS flash.
 *
 * @param[in] in_data Pointer to data to persist.
 * @return
 *      - ESP_OK: Successfully saved and committed.
 *      - ESP_FAIL: Flash write or commit error.
 */
esp_err_t storage_manager_save_calibration(const posture_calib_data_t *in_data);

/**
 * @brief Reset calibration parameters to factory defaults and write to NVS.
 *
 * @param[out] out_data Pointer to receive reset values.
 * @return ESP_OK on success.
 */
esp_err_t storage_manager_reset_to_default(posture_calib_data_t *out_data);

#ifdef __cplusplus
}
#endif
