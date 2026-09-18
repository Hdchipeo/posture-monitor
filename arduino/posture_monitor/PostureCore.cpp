/**
 * @file PostureCore.cpp
 * @brief Implementation of posture filter and progressive escalation FSM.
 */

#include "PostureCore.h"
#include "Config.h"
#include <math.h>

#define RAD_TO_DEG_F 57.29577951308232f

PostureCore::PostureCore()
    : _state(STATE_GOOD),
      _currentAngles{0.0f, 0.0f},
      _filterInitialized(false),
      _slouchTimer(0.0f),
      _snoozeTimer(0.0f),
      _isCalibrating(false),
      _calibTargetSamples(100),
      _calibCurrentSamples(0),
      _calibSumPitch(0.0f),
      _calibSumRoll(0.0f) {}

void PostureCore::begin(const CalibrationData &calib) {
    _calib = calib;
    _state = STATE_GOOD;
    _slouchTimer = 0.0f;
    _snoozeTimer = 0.0f;
    _filterInitialized = false;
    _isCalibrating = false;
}

void PostureCore::updateFilter(const RawIMUData &raw, float dt, bool isVibrating) {
    // Accelerometer: +/- 2g scale -> 16384 LSB/g
    float ax = (float)raw.ax / 16384.0f;
    float ay = (float)raw.ay / 16384.0f;
    float az = (float)raw.az / 16384.0f;

    // Gyroscope: +/- 250 dps scale -> 131.0 LSB/(deg/s)
    float gx = (float)raw.gx / 131.0f;
    float gy = (float)raw.gy / 131.0f;

    float pitchAcc = atan2f(ay, sqrtf(ax * ax + az * az)) * RAD_TO_DEG_F;
    float rollAcc  = atan2f(-ax, az) * RAD_TO_DEG_F;

    if (!_filterInitialized) {
        _currentAngles.pitch = pitchAcc;
        _currentAngles.roll = rollAcc;
        _filterInitialized = true;
    } else {
        if (isVibrating) {
            // Freeze accelerometer gravity update during active vibration to decouple acoustic noise
            _currentAngles.pitch += gx * dt;
            _currentAngles.roll  += gy * dt;
        } else {
            _currentAngles.pitch = COMPLEMENTARY_ALPHA * (_currentAngles.pitch + gx * dt) + (1.0f - COMPLEMENTARY_ALPHA) * pitchAcc;
            _currentAngles.roll  = COMPLEMENTARY_ALPHA * (_currentAngles.roll  + gy * dt) + (1.0f - COMPLEMENTARY_ALPHA) * rollAcc;
        }
    }
}

void PostureCore::startCalibration(uint16_t sampleCount) {
    _isCalibrating = true;
    _calibTargetSamples = sampleCount;
    _calibCurrentSamples = 0;
    _calibSumPitch = 0.0f;
    _calibSumRoll = 0.0f;
    _state = STATE_CALIBRATING;
    Serial.println(F("[PostureCore] Calibration started. Sit upright and hold still..."));
}

bool PostureCore::isCalibrating() const {
    return _isCalibrating;
}

bool PostureCore::feedCalibrationSample(CalibrationData &outCalib) {
    if (!_isCalibrating) return false;

    _calibSumPitch += _currentAngles.pitch;
    _calibSumRoll  += _currentAngles.roll;
    _calibCurrentSamples++;

    if (_calibCurrentSamples >= _calibTargetSamples) {
        _calib.pitchOffset = _calibSumPitch / (float)_calibCurrentSamples;
        _calib.rollOffset  = _calibSumRoll  / (float)_calibCurrentSamples;
        _isCalibrating = false;
        _state = STATE_GOOD;
        _slouchTimer = 0.0f;

        outCalib = _calib;
        Serial.printf("[PostureCore] Calibration complete: Base Pitch=%.2f, Base Roll=%.2f\n",
                      _calib.pitchOffset, _calib.rollOffset);
        return true;
    }
    return false;
}

void PostureCore::snooze(uint32_t seconds) {
    _snoozeTimer = (float)seconds;
    _state = STATE_SNOOZED;
    _slouchTimer = 0.0f;
    Serial.printf("[PostureCore] Snoozed for %u seconds.\n", (unsigned int)seconds);
}

void PostureCore::processFSM(float dt) {
    if (_isCalibrating) {
        _state = STATE_CALIBRATING;
        return;
    }

    if (_snoozeTimer > 0.0f) {
        _snoozeTimer -= dt;
        if (_snoozeTimer <= 0.0f) {
            _snoozeTimer = 0.0f;
            _state = STATE_GOOD;
            Serial.println(F("[PostureCore] Snooze expired. Resuming monitoring."));
        } else {
            _state = STATE_SNOOZED;
            return;
        }
    }

    float deltaPitch = fabsf(_currentAngles.pitch - _calib.pitchOffset);
    float deltaRoll  = fabsf(_currentAngles.roll  - _calib.rollOffset);

    bool isSlouching = (deltaPitch > _calib.angleThreshold) || (deltaRoll > _calib.angleThreshold);

    if (!isSlouching) {
        if (_state != STATE_GOOD) {
            Serial.printf("[PostureCore] Posture restored to GOOD (Delta P: %.1f, R: %.1f)\n", deltaPitch, deltaRoll);
        }
        _state = STATE_GOOD;
        _slouchTimer = 0.0f;
    } else {
        _slouchTimer += dt;
        if (_slouchTimer < (float)_calib.slouchDelaySeconds) {
            _state = STATE_SUSPECTED_SLOUCH;
        } else if (_slouchTimer < (float)(_calib.slouchDelaySeconds + _calib.escalationDelaySeconds)) {
            _state = STATE_ALERT_L1;
        } else {
            _state = STATE_ALERT_L2;
        }
    }
}

PostureState PostureCore::getState() const {
    return _state;
}

Angles PostureCore::getAngles() const {
    return _currentAngles;
}

float PostureCore::getDeviation() const {
    float deltaPitch = fabsf(_currentAngles.pitch - _calib.pitchOffset);
    float deltaRoll  = fabsf(_currentAngles.roll  - _calib.rollOffset);
    return sqrtf(deltaPitch * deltaPitch + deltaRoll * deltaRoll);
}

uint8_t PostureCore::getPostureScore() const {
    float dev = getDeviation();
    float penalty = (dev / _calib.angleThreshold) * 45.0f;
    if (penalty > 90.0f) penalty = 90.0f;
    return (uint8_t)roundf(100.0f - penalty);
}

CalibrationData PostureCore::getCalibration() const {
    return _calib;
}

void PostureCore::setCalibration(const CalibrationData &calib) {
    _calib = calib;
}
