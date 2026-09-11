/**
 * @file MPU6050Driver.cpp
 * @brief Implementation of MPU6050 registers, bus recovery and burst reads.
 */

#include "MPU6050Driver.h"

#define REG_SMPLRT_DIV   0x19
#define REG_CONFIG       0x1A
#define REG_GYRO_CONFIG  0x1B
#define REG_ACCEL_CONFIG 0x1C
#define REG_ACCEL_XOUT_H 0x3B
#define REG_PWR_MGMT_1   0x6B
#define REG_WHO_AM_I     0x75

MPU6050Driver::MPU6050Driver(uint8_t address) : _address(address) {}

void MPU6050Driver::recoverBus(int sdaPin, int sclPin) {
    pinMode(sdaPin, INPUT_PULLUP);
    pinMode(sclPin, OUTPUT_OPEN_DRAIN);

    if (digitalRead(sdaPin) == LOW) {
        Serial.println(F("[MPU6050] SDA stuck LOW! Pulsing SCL 9 times to unwedge bus..."));
        for (int i = 0; i < 9; i++) {
            digitalWrite(sclPin, LOW);
            delayMicroseconds(5);
            digitalWrite(sclPin, HIGH);
            delayMicroseconds(5);
            if (digitalRead(sdaPin) == HIGH) break;
        }
        // Send STOP
        pinMode(sdaPin, OUTPUT_OPEN_DRAIN);
        digitalWrite(sdaPin, LOW);
        delayMicroseconds(5);
        digitalWrite(sclPin, HIGH);
        delayMicroseconds(5);
        digitalWrite(sdaPin, HIGH);
        delayMicroseconds(5);
    }
}

bool MPU6050Driver::begin(int sdaPin, int sclPin, uint32_t freqHz) {
    recoverBus(sdaPin, sclPin);

    Wire.begin(sdaPin, sclPin, freqHz);

    // Verify WHO_AM_I register
    uint8_t whoAmI = 0;
    if (!readRegisters(REG_WHO_AM_I, &whoAmI, 1) || whoAmI != 0x68) {
        Serial.printf("[MPU6050] Device check failed! WHO_AM_I returned: 0x%02X (Expected: 0x68)\n", whoAmI);
        return false;
    }

    // 1. Wake up and set clock source to PLL with X axis gyroscope reference
    writeRegister(REG_PWR_MGMT_1, 0x01);

    // 2. DLPF Configuration: 44Hz low pass filter to eliminate high-frequency motor vibrations
    writeRegister(REG_CONFIG, 0x03);

    // 3. Sample rate divider: 1kHz / (1 + 4) = 200Hz internal sampling
    writeRegister(REG_SMPLRT_DIV, 0x04);

    // 4. Gyroscope range: +/- 250 deg/s
    writeRegister(REG_GYRO_CONFIG, 0x00);

    // 5. Accelerometer range: +/- 2g
    writeRegister(REG_ACCEL_CONFIG, 0x00);

    Serial.println(F("[MPU6050] Sensor initialized successfully (DLPF: 44Hz, +/-2g, +/-250dps)."));
    return true;
}

bool MPU6050Driver::readRaw(RawIMUData &data) {
    uint8_t buf[14];
    if (!readRegisters(REG_ACCEL_XOUT_H, buf, 14)) return false;

    data.ax = (int16_t)((buf[0] << 8) | buf[1]);
    data.ay = (int16_t)((buf[2] << 8) | buf[3]);
    data.az = (int16_t)((buf[4] << 8) | buf[5]);
    data.temp = (int16_t)((buf[6] << 8) | buf[7]);
    data.gx = (int16_t)((buf[8] << 8) | buf[9]);
    data.gy = (int16_t)((buf[10] << 8) | buf[11]);
    data.gz = (int16_t)((buf[12] << 8) | buf[13]);
    return true;
}

void MPU6050Driver::sleep() {
    writeRegister(REG_PWR_MGMT_1, 0x40); // SLEEP bit = 1
}

void MPU6050Driver::wakeUp() {
    writeRegister(REG_PWR_MGMT_1, 0x01);
}

bool MPU6050Driver::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
}

bool MPU6050Driver::readRegisters(uint8_t startReg, uint8_t *buffer, size_t length) {
    Wire.beginTransmission(_address);
    Wire.write(startReg);
    if (Wire.endTransmission(false) != 0) return false;

    size_t received = Wire.requestFrom(_address, length);
    if (received != length) return false;

    for (size_t i = 0; i < length; i++) {
        buffer[i] = Wire.read();
    }
    return true;
}
