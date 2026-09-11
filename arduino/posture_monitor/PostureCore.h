/**
 * @file PostureCore.h
 * @brief Complementary filter orientation estimator and posture FSM for Arduino.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#pragma once

#include <Arduino.h>
#include "StorageManager.h"
#include "MPU6050Driver.h"

enum PostureState {
    STATE_CALIBRATING = 0,
    STATE_GOOD,
    STATE_SUSPECTED_SLOUCH,
    STATE_ALERT_L1,
    STATE_ALERT_L2,
    STATE_SNOOZED
};

struct Angles {
    float pitch;
    float roll;
};

class PostureCore {
public:
    PostureCore();

    void begin(const CalibrationData &calib);
    void updateFilter(const RawIMUData &raw, float dt, bool isVibrating);
    void processFSM(float dt);

    void startCalibration(uint16_t sampleCount);
    bool isCalibrating() const;
    bool feedCalibrationSample(CalibrationData &outCalib);

    void snooze(uint32_t seconds);
    PostureState getState() const;
    Angles getAngles() const;
    CalibrationData getCalibration() const;
    void setCalibration(const CalibrationData &calib);

private:
    CalibrationData _calib;
    PostureState _state;
    Angles _currentAngles;

    bool _filterInitialized;
    float _slouchTimer;
    float _snoozeTimer;

    bool _isCalibrating;
    uint16_t _calibTargetSamples;
    uint16_t _calibCurrentSamples;
    float _calibSumPitch;
    float _calibSumRoll;
};
