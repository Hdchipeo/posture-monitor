/**
 * @file ActuatorManager.h
 * @brief Non-blocking haptic and buzzer pattern sequencer using millis().
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#pragma once

#include <Arduino.h>

enum AlertPattern {
    ALERT_IDLE = 0,
    ALERT_CALIB_START,
    ALERT_CALIB_DONE,
    ALERT_LEVEL1_HAPTIC,
    ALERT_LEVEL2_ALARM
};

class ActuatorManager {
public:
    ActuatorManager(int motorPin, int buzzerPin, bool buzzerEnabled = true);

    void begin();
    void setPattern(AlertPattern pattern);
    void update(); // Must be called frequently from loop()
    bool isVibrating() const;

private:
    int _motorPin;
    int _buzzerPin;
    bool _buzzerEnabled;
    AlertPattern _currentPattern;
    uint32_t _lastTickMs;
    uint32_t _tickCount;
    bool _motorActive;

    void setMotor(bool on);
    void setBuzzer(bool on);
};
