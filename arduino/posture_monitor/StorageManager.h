/**
 * @file StorageManager.h
 * @brief NVS persistence using Arduino Preferences.h with CRC32 integrity check.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#pragma once

#include <Arduino.h>
#include <Preferences.h>

#define STORAGE_MAGIC 0x504F5354 // "POST"

struct CalibrationData {
    uint32_t magic;
    float pitchOffset;
    float rollOffset;
    float angleThreshold;
    uint32_t slouchDelaySeconds;
    uint32_t escalationDelaySeconds;
    uint32_t crc32;
} __attribute__((packed));

class StorageManager {
public:
    StorageManager();
    bool begin();
    bool loadCalibration(CalibrationData &data);
    bool saveCalibration(const CalibrationData &data);
    void resetToDefault(CalibrationData &data);

private:
    Preferences _prefs;
    uint32_t calculateCRC32(const uint8_t *data, size_t length);
};
