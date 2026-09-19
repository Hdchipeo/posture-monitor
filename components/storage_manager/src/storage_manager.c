/**
 * @file storage_manager.c
 * @brief Implementation of NVS manager with CRC32 integrity verification.
 */

#include "storage_manager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "STORAGE_MGR";

#ifndef CONFIG_POSTURE_NVS_NAMESPACE
#define CONFIG_POSTURE_NVS_NAMESPACE "posture_cfg"
#endif

#ifndef CONFIG_POSTURE_ANGLE_THRESHOLD_DEG
#define CONFIG_POSTURE_ANGLE_THRESHOLD_DEG 15
#endif

#ifndef CONFIG_POSTURE_SLOUCH_TOLERANCE_TIME_S
#define CONFIG_POSTURE_SLOUCH_TOLERANCE_TIME_S 5
#endif

#ifndef CONFIG_POSTURE_ESCALATION_TIME_S
#define CONFIG_POSTURE_ESCALATION_TIME_S 15
#endif

#define NVS_KEY_CALIB "calib_blob"

// Standard IEEE 802.3 CRC32 implementation
static uint32_t calculate_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

esp_err_t storage_manager_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition truncated or version mismatch. Erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS flash: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "NVS flash initialized successfully");
    return ESP_OK;
}

esp_err_t storage_manager_reset_to_default(posture_calib_data_t *out_data) {
    if (!out_data) return ESP_ERR_INVALID_ARG;

    out_data->magic = STORAGE_PAYLOAD_MAGIC;
    out_data->pitch_offset = 0.0f;
    out_data->roll_offset = 0.0f;
    out_data->angle_threshold = (float)CONFIG_POSTURE_ANGLE_THRESHOLD_DEG;
    out_data->slouch_delay_s = CONFIG_POSTURE_SLOUCH_TOLERANCE_TIME_S;
    out_data->escalation_delay_s = CONFIG_POSTURE_ESCALATION_TIME_S;

    size_t payload_len = sizeof(posture_calib_data_t) - sizeof(uint32_t);
    out_data->crc32 = calculate_crc32((const uint8_t *)out_data, payload_len);

    return storage_manager_save_calibration(out_data);
}

esp_err_t storage_manager_load_calibration(posture_calib_data_t *out_data) {
    if (!out_data) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(CONFIG_POSTURE_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No existing configuration in NVS. Loading factory defaults...");
        return storage_manager_reset_to_default(out_data);
    }

    size_t required_size = sizeof(posture_calib_data_t);
    err = nvs_get_blob(handle, NVS_KEY_CALIB, out_data, &required_size);
    nvs_close(handle);

    if (err != ESP_OK || required_size != sizeof(posture_calib_data_t)) {
        ESP_LOGW(TAG, "Corrupt or missing blob size (%zu bytes). Resetting...", required_size);
        return storage_manager_reset_to_default(out_data);
    }

    if (out_data->magic != STORAGE_PAYLOAD_MAGIC) {
        ESP_LOGE(TAG, "Magic word mismatch (0x%08X != 0x%08X). Resetting...",
                 (unsigned int)out_data->magic, (unsigned int)STORAGE_PAYLOAD_MAGIC);
        return storage_manager_reset_to_default(out_data);
    }

    size_t payload_len = sizeof(posture_calib_data_t) - sizeof(uint32_t);
    uint32_t expected_crc = calculate_crc32((const uint8_t *)out_data, payload_len);
    if (expected_crc != out_data->crc32) {
        ESP_LOGE(TAG, "CRC32 checksum mismatch (Calculated: 0x%08X, Stored: 0x%08X). Data corrupted!",
                 (unsigned int)expected_crc, (unsigned int)out_data->crc32);
        return storage_manager_reset_to_default(out_data);
    }

    // Migration: detect legacy pre-fix horizontal reference (where roll_offset was ~ -90 deg)
    if (out_data->roll_offset < -45.0f && out_data->roll_offset > -135.0f) {
        ESP_LOGW(TAG, "Legacy Roll offset (~ -90 deg) detected (%.2f deg). Migrating to vertical reference (+90 deg)...",
                 out_data->roll_offset);
        out_data->roll_offset += 90.0f;
        storage_manager_save_calibration(out_data);
    }

    ESP_LOGI(TAG, "Calibration loaded: Pitch_Offset=%.2f deg, Roll_Offset=%.2f deg, Thresh=%.1f deg",
             out_data->pitch_offset, out_data->roll_offset, out_data->angle_threshold);
    return ESP_OK;
}

esp_err_t storage_manager_save_calibration(const posture_calib_data_t *in_data) {
    if (!in_data) return ESP_ERR_INVALID_ARG;

    posture_calib_data_t copy = *in_data;
    copy.magic = STORAGE_PAYLOAD_MAGIC;
    size_t payload_len = sizeof(posture_calib_data_t) - sizeof(uint32_t);
    copy.crc32 = calculate_crc32((const uint8_t *)&copy, payload_len);

    nvs_handle_t handle;
    esp_err_t err = nvs_open(CONFIG_POSTURE_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle for write: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(handle, NVS_KEY_CALIB, &copy, sizeof(posture_calib_data_t));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Calibration committed to NVS (Pitch: %.2f, Roll: %.2f, CRC: 0x%08X)",
                 copy.pitch_offset, copy.roll_offset, (unsigned int)copy.crc32);
    } else {
        ESP_LOGE(TAG, "Failed to commit calibration to NVS: %s", esp_err_to_name(err));
    }
    return err;
}
