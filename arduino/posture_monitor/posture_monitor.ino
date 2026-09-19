/**
 * @file posture_monitor.ino
 * @brief Main Arduino sketch for ESP32 & ESP32-C3 Smart Posture Monitor.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#include "Config.h"
#include "MPU6050Driver.h"
#include "StorageManager.h"
#include "ActuatorManager.h"
#include "PostureCore.h"

static MPU6050Driver   mpu;
static StorageManager  storage;
static ActuatorManager actuator(PIN_VIBRATION_MOTOR, PIN_BUZZER, ENABLE_BUZZER);
static PostureCore     core;

static uint32_t lastSampleTimeMs = 0;
static uint32_t lastReportTimeMs = 0;

// Button gesture detection
static bool buttonPressed = false;
static uint32_t btnPressTimeMs = 0;
static uint32_t lastBtnReleaseMs = 0;
static uint8_t clickCount = 0;

void handleButton() {
    bool rawState = (digitalRead(PIN_BUTTON) == LOW); // Active LOW
    uint32_t now = millis();

    if (rawState && !buttonPressed) {
        // Button just pressed down
        buttonPressed = true;
        btnPressTimeMs = now;
    } else if (!rawState && buttonPressed) {
        // Button just released
        buttonPressed = false;
        uint32_t pressDuration = now - btnPressTimeMs;

        if (pressDuration >= 1800) {
            // Long Press (>1.8s) -> Tare / Recalibrate Neutral Posture
            Serial.println(F("[Button] Long press detected -> Initiating Tare Calibration..."));
            actuator.setPattern(ALERT_CALIB_START);
            core.startCalibration(CALIBRATION_SAMPLE_COUNT);
            clickCount = 0;
        } else if (pressDuration >= 40) {
            // Valid short click
            clickCount++;
            lastBtnReleaseMs = now;
        }
    }

    // Double-click window evaluation (350ms window after release)
    if (clickCount > 0 && !buttonPressed) {
        if (now - lastBtnReleaseMs > 350) {
            if (clickCount >= 2) {
                Serial.println(F("[Button] Double-click detected -> Snooze 10 minutes!"));
                core.snooze(SNOOZE_DURATION_SECONDS);
                actuator.setPattern(ALERT_IDLE);
            } else if (clickCount == 1) {
                Serial.println(F("[Button] Single click detected -> Silencing active alert."));
                actuator.setPattern(ALERT_IDLE);
            }
            clickCount = 0;
        }
    }
}

static uint32_t lastLedToggleMs = 0;
static bool ledState = false;

void updateStatusLed(PostureState state) {
    uint32_t now = millis();
    uint32_t interval = 2000; // Default Good heartbeat period

    switch (state) {
        case STATE_GOOD:
            // Heartbeat: brief 60ms flash every 2000ms
            if (now - lastLedToggleMs < 60) {
                digitalWrite(PIN_STATUS_LED, HIGH);
            } else if (now - lastLedToggleMs < 2000) {
                digitalWrite(PIN_STATUS_LED, LOW);
            } else {
                lastLedToggleMs = now;
            }
            return;

        case STATE_SUSPECTED_SLOUCH:
        case STATE_ALERT_L1:
            interval = 250; // Warn: 2 Hz blink
            break;

        case STATE_ALERT_L2:
            interval = 100; // Alarm: 5 Hz rapid strobe
            break;

        case STATE_SNOOZED:
            interval = 800; // Snoozed: 0.6 Hz slow blink
            break;

        case STATE_CALIBRATING:
            interval = 50;  // Sampling: 10 Hz ultra-fast cadence
            break;
    }

    if (now - lastLedToggleMs >= interval) {
        lastLedToggleMs = now;
        ledState = !ledState;
        digitalWrite(PIN_STATUS_LED, ledState ? HIGH : LOW);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println(F("============================================================"));
    Serial.println(F("     Arduino Smart Posture Monitor System v1.1.2            "));
    Serial.printf ("     Target Hardware: %s\n", CHIP_MODEL_STR);
    Serial.printf ("     Pins: SDA=%d, SCL=%d, Motor=%d, Buzzer=%d, LED=%d, Btn=%d\n",
                   PIN_I2C_SDA, PIN_I2C_SCL, PIN_VIBRATION_MOTOR, PIN_BUZZER, PIN_STATUS_LED, PIN_BUTTON);
    Serial.println(F("============================================================"));

    pinMode(PIN_BUTTON, INPUT_PULLUP);
    pinMode(PIN_STATUS_LED, OUTPUT);

    // Power-On Self-Test (POST) LED Cadence: 3 fast 50ms blinks
    for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_STATUS_LED, HIGH);
        delay(50);
        digitalWrite(PIN_STATUS_LED, LOW);
        delay(50);
    }

    // 1. Storage / NVS
    storage.begin();
    CalibrationData calib;
    storage.loadCalibration(calib);

    // 2. Actuators
    actuator.begin();

    // 3. Sensor IMU
    if (!mpu.begin(PIN_I2C_SDA, PIN_I2C_SCL)) {
        Serial.println(F("[WARNING] MPU6050 not detected at startup. Will retry in background..."));
    }

    // 4. Core Algorithm & FSM
    core.begin(calib);

    lastSampleTimeMs = millis();
    lastReportTimeMs = millis();
    Serial.println(F("[System] Ready! Sit upright and hold button >2s to Tare neutral posture."));
}

void loop() {
    // 1. Non-blocking actuator tick update
    actuator.update();

    // 2. Non-blocking button handler
    handleButton();

    // 3. Diagnostic Status LED
    updateStatusLed(core.getState());

    // 4. Fixed-frequency sensor sampling and filter loop (50Hz = 20ms)
    uint32_t now = millis();
    if (now - lastSampleTimeMs >= SAMPLING_PERIOD_MS) {
        float dt = (float)(now - lastSampleTimeMs) / 1000.0f;
        lastSampleTimeMs = now;

        RawIMUData raw;
        if (mpu.readRaw(raw)) {
            // Update filter with acoustic motor decoupling
            core.updateFilter(raw, dt, actuator.isVibrating());

            // Handle Calibration
            if (core.isCalibrating()) {
                CalibrationData updatedCalib;
                if (core.feedCalibrationSample(updatedCalib)) {
                    // Completed: save to NVS and signal done
                    storage.saveCalibration(updatedCalib);
                    actuator.setPattern(ALERT_CALIB_DONE);
                }
            } else {
                // Regular FSM processing
                core.processFSM(dt);

                switch (core.getState()) {
                    case STATE_GOOD:
                    case STATE_SUSPECTED_SLOUCH:
                    case STATE_SNOOZED:
                        actuator.setPattern(ALERT_IDLE);
                        break;
                    case STATE_ALERT_L1:
                        actuator.setPattern(ALERT_LEVEL1_HAPTIC);
                        break;
                    case STATE_ALERT_L2:
                        actuator.setPattern(ALERT_LEVEL2_ALARM);
                        break;
                    default:
                        break;
                }
            }
        } else {
            // Sensor offline: silence actuators for safety
            actuator.setPattern(ALERT_IDLE);

            // Attempt reconnection every 2 seconds if sensor not healthy
            static uint32_t lastReconnectAttempt = 0;
            if (!mpu.isHealthy() && (now - lastReconnectAttempt > 2000)) {
                lastReconnectAttempt = now;
                Serial.println(F("[Sensor] Attempting to reconnect MPU6050..."));
                mpu.begin(PIN_I2C_SDA, PIN_I2C_SCL);
            }
        }
    }

    // 5. Periodic diagnostic telemetry to Serial Monitor (every 5 seconds)
    if (now - lastReportTimeMs >= 5000) {
        lastReportTimeMs = now;
        Angles angles = core.getAngles();

        if (mpu.isHealthy()) {
            Serial.printf("[STATUS] Roll (Cui): %5.1f° | Pitch (Nghieng): %5.1f° | Yaw (Xoay): %5.1f° | Dev: %4.1f° | Score: %2u | State: %d | FreeHeap: %u B\n",
                          angles.roll, angles.pitch, angles.yaw, core.getDeviation(), core.getPostureScore(),
                          core.getState(), (unsigned int)ESP.getFreeHeap());
        } else {
            Serial.printf("[STATUS] SENSOR OFFLINE (Check SDA=%d, SCL=%d) | State: %d | FreeHeap: %u B\n",
                          PIN_I2C_SDA, PIN_I2C_SCL, core.getState(), (unsigned int)ESP.getFreeHeap());
        }
    }
}
