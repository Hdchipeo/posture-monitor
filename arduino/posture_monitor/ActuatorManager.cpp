/**
 * @file ActuatorManager.cpp
 * @brief Implementation of non-blocking actuator pattern sequencer.
 */

#include "ActuatorManager.h"

ActuatorManager::ActuatorManager(int motorPin, int buzzerPin, bool buzzerEnabled)
    : _motorPin(motorPin),
      _buzzerPin(buzzerPin),
      _buzzerEnabled(buzzerEnabled),
      _currentPattern(ALERT_IDLE),
      _lastTickMs(0),
      _tickCount(0),
      _motorActive(false) {}

void ActuatorManager::begin() {
    pinMode(_motorPin, OUTPUT);
    pinMode(_buzzerPin, OUTPUT);
    setMotor(false);
    setBuzzer(false);
    _lastTickMs = millis();
}

void ActuatorManager::setMotor(bool on) {
    _motorActive = on;
    digitalWrite(_motorPin, on ? HIGH : LOW);
}

void ActuatorManager::setBuzzer(bool on) {
    if (_buzzerEnabled) {
        digitalWrite(_buzzerPin, on ? HIGH : LOW);
    } else {
        digitalWrite(_buzzerPin, LOW);
    }
}

void ActuatorManager::setPattern(AlertPattern pattern) {
    if (_currentPattern != pattern) {
        _currentPattern = pattern;
        _tickCount = 0;
        if (pattern == ALERT_IDLE) {
            setMotor(false);
            setBuzzer(false);
        }
    }
}

bool ActuatorManager::isVibrating() const {
    return _motorActive;
}

void ActuatorManager::update() {
    uint32_t now = millis();
    if (now - _lastTickMs < 100) {
        return; // 100ms base tick cadence
    }
    _lastTickMs = now;
    _tickCount++;

    switch (_currentPattern) {
        case ALERT_IDLE:
            setMotor(false);
            setBuzzer(false);
            break;

        case ALERT_CALIB_START:
            if (_tickCount <= 2) {
                setMotor(true);
            } else {
                setMotor(false);
                _currentPattern = ALERT_IDLE;
            }
            setBuzzer(false);
            break;

        case ALERT_CALIB_DONE:
            if (_tickCount == 1 || _tickCount == 2) {
                setMotor(true);
            } else if (_tickCount == 3) {
                setMotor(false);
            } else if (_tickCount == 4 || _tickCount == 5) {
                setMotor(true);
            } else {
                setMotor(false);
                _currentPattern = ALERT_IDLE;
            }
            setBuzzer(false);
            break;

        case ALERT_LEVEL1_HAPTIC: {
            // 1200ms cycle: 200ms ON, 1000ms OFF
            uint32_t subTick = _tickCount % 12;
            setMotor(subTick < 2);
            setBuzzer(false);
            break;
        }

        case ALERT_LEVEL2_ALARM: {
            // 500ms cycle: 200ms ON (Motor + Buzzer), 300ms OFF
            uint32_t subTick = _tickCount % 5;
            bool active = (subTick < 2);
            setMotor(active);
            setBuzzer(active);
            break;
        }
    }
}
