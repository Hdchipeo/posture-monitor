/**
 * @file StorageManager.cpp
 * @brief Implementation of NVS storage using Arduino Preferences with CRC32.
 */

#include "StorageManager.h"
#include "Config.h"

StorageManager::StorageManager() {}

uint32_t StorageManager::calculateCRC32(const uint8_t *data, size_t length) {
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

bool StorageManager::begin() {
    return _prefs.begin("posture_cfg", false);
}

void StorageManager::resetToDefault(CalibrationData &data) {
    data.magic = STORAGE_MAGIC;
    data.pitchOffset = 0.0f;
    data.rollOffset = 0.0f;
    data.angleThreshold = DEFAULT_SLOUCH_THRESH_DEG;
    data.slouchDelaySeconds = DEFAULT_SLOUCH_TOLERANCE_S;
    data.escalationDelaySeconds = DEFAULT_ESCALATION_TIME_S;

    size_t payloadSize = sizeof(CalibrationData) - sizeof(uint32_t);
    data.crc32 = calculateCRC32((const uint8_t *)&data, payloadSize);

    saveCalibration(data);
    Serial.println(F("[Storage] Factory defaults generated and persisted."));
}

bool StorageManager::loadCalibration(CalibrationData &data) {
    if (!_prefs.isKey("calib_blob")) {
        Serial.println(F("[Storage] No existing calibration key in NVS. Loading defaults..."));
        resetToDefault(data);
        return true;
    }

    size_t readLen = _prefs.getBytes("calib_blob", &data, sizeof(CalibrationData));
    if (readLen != sizeof(CalibrationData)) {
        Serial.printf("[Storage] Incomplete data read (%u bytes). Resetting...\n", (unsigned int)readLen);
        resetToDefault(data);
        return false;
    }

    if (data.magic != STORAGE_MAGIC) {
        Serial.println(F("[Storage] Magic header mismatch! Resetting..."));
        resetToDefault(data);
        return false;
    }

    size_t payloadSize = sizeof(CalibrationData) - sizeof(uint32_t);
    uint32_t expectedCRC = calculateCRC32((const uint8_t *)&data, payloadSize);
    if (expectedCRC != data.crc32) {
        Serial.printf("[Storage] CRC32 corrupted (Calc: 0x%08X, Stored: 0x%08X)! Resetting...\n",
                      (unsigned int)expectedCRC, (unsigned int)data.crc32);
        resetToDefault(data);
        return false;
    }

    Serial.printf("[Storage] Calibration loaded: Pitch_Offset=%.2f, Roll_Offset=%.2f, Thresh=%.1f\n",
                  data.pitchOffset, data.rollOffset, data.angleThreshold);
    return true;
}

bool StorageManager::saveCalibration(const CalibrationData &data) {
    CalibrationData copy = data;
    copy.magic = STORAGE_MAGIC;

    size_t payloadSize = sizeof(CalibrationData) - sizeof(uint32_t);
    copy.crc32 = calculateCRC32((const uint8_t *)&copy, payloadSize);

    size_t written = _prefs.putBytes("calib_blob", &copy, sizeof(CalibrationData));
    if (written == sizeof(CalibrationData)) {
        Serial.printf("[Storage] Saved to NVS (Pitch: %.2f, Roll: %.2f, CRC: 0x%08X)\n",
                      copy.pitchOffset, copy.rollOffset, (unsigned int)copy.crc32);
        return true;
    } else {
        Serial.println(F("[Storage] Error writing to NVS!"));
        return false;
    }
}
