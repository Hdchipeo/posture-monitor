/**
 * @file MPU6050Driver.h
 * @brief Self-contained MPU6050 driver via Wire.h with I2C bus recovery.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#pragma once

#include <Arduino.h>
#include <Wire.h>

struct RawIMUData {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    int16_t temp;
};

class MPU6050Driver {
public:
    MPU6050Driver(uint8_t address = 0x68);

    /**
     * @brief Perform 9-clock bus recovery and initialize MPU6050.
     * @param sdaPin GPIO for SDA.
     * @param sclPin GPIO for SCL.
     * @param freqHz I2C clock frequency (default 400kHz).
     * @return true if WHO_AM_I responded correctly (0x68).
     */
    bool begin(int sdaPin, int sclPin, uint32_t freqHz = 400000);

    /**
     * @brief Read 14 bytes of sensor data in a single burst.
     */
    bool readRaw(RawIMUData &data);

    /**
     * @brief Put the MPU6050 into low power sleep mode.
     */
    void sleep();

    /**
     * @brief Wake up MPU6050 from sleep mode.
     */
    void wakeUp();

private:
    uint8_t _address;
    void recoverBus(int sdaPin, int sclPin);
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegisters(uint8_t startReg, uint8_t *buffer, size_t length);
};
