/**
 * @file Config.h
 * @brief Hardware pin mapping and algorithm configurations for Arduino ESP32 / ESP32-C3.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Target Hardware Detection & Pin Assignments
// -----------------------------------------------------------------------------

#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ARDUINO_ESP32C3_DEV) || defined(ARDUINO_ESP32_C3)
    // ESP32-C3 RISC-V Default Pins
    #define PIN_I2C_SDA         8   // Alternative: 4
    #define PIN_I2C_SCL         9   // Alternative: 5
    #define PIN_VIBRATION_MOTOR 6   // Drives N-MOSFET (AO3400)
    #define PIN_BUZZER          7   // Drives Buzzer transistor
    #define PIN_STATUS_LED      5   // Status diagnostic LED (Active HIGH)
    #define PIN_BATTERY_ADC     1   // Battery voltage divider (100k-100k, ADC1_CH1)
    #define PIN_BUTTON          0   // Active LOW Boot button (or GPIO 9)
    #define CHIP_MODEL_STR      "ESP32-C3 (RISC-V)"
#else
    // ESP32 Classic (Xtensa Dual-Core) Default Pins
    // WARNING: Never use GPIO 6-11 on ESP32 (Internal SPI Flash)!
    #define PIN_I2C_SDA         21  // Standard I2C SDA
    #define PIN_I2C_SCL         22  // Standard I2C SCL
    #define PIN_VIBRATION_MOTOR 18  // Drives N-MOSFET (AO3400)
    #define PIN_BUZZER          19  // Drives Buzzer transistor
    #define PIN_STATUS_LED      2   // Diagnostic Status LED
    #define PIN_BATTERY_ADC     34  // Battery voltage divider (ADC1_CH6)
    #define PIN_BUTTON          0   // On-board BOOT button (Active LOW)
    #define CHIP_MODEL_STR      "ESP32 (Xtensa Dual-Core)"
#endif

// Compile-time safety check against SPI Flash pins on ESP32 Classic
#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(ARDUINO_ESP32C3_DEV) && !defined(ARDUINO_ESP32_C3)
    #if (PIN_I2C_SDA >= 6 && PIN_I2C_SDA <= 11) || \
        (PIN_I2C_SCL >= 6 && PIN_I2C_SCL <= 11) || \
        (PIN_VIBRATION_MOTOR >= 6 && PIN_VIBRATION_MOTOR <= 11) || \
        (PIN_BUZZER >= 6 && PIN_BUZZER <= 11)
        #error "Critical Error: Pins 6-11 are dedicated to SPI Flash on ESP32! Please change pins in Config.h."
    #endif
#endif

// -----------------------------------------------------------------------------
// Algorithm Parameters
// -----------------------------------------------------------------------------
#define SAMPLING_RATE_HZ          50           // 50Hz update loop (20ms)
#define SAMPLING_PERIOD_MS        (1000 / SAMPLING_RATE_HZ)
#define COMPLEMENTARY_ALPHA       0.96f        // Gyroscope weight (0.96 Gyro + 0.04 Accel)
#define DEFAULT_SLOUCH_THRESH_DEG 15.0f        // Trigger deviation threshold (degrees)
#define DEFAULT_SLOUCH_TOLERANCE_S 5           // Grace period before Level 1 Haptic alert (seconds)
#define DEFAULT_ESCALATION_TIME_S  15          // Grace period before Level 2 Audio alarm (seconds)
#define CALIBRATION_SAMPLE_COUNT  100          // Samples to average neutral posture (2 seconds at 50Hz)
#define SNOOZE_DURATION_SECONDS   600          // 10 minutes snooze

// Buzzer feature toggle (set to false for silent haptic-only operation)
#define ENABLE_BUZZER             true
