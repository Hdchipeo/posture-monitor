# Arduino Posture Monitor (ESP32 & ESP32-C3)

## 1. Overview
This directory contains the standalone Arduino-compatible version of the Posture Monitor system. It is designed to compile directly in **Arduino IDE** or **PlatformIO** with **zero external library dependencies** (all sensor communication is handled natively via `Wire.h`).

## 2. Hardware Support & Automatic Pin Selection
The sketch automatically detects the active board target at compile time:

| Signal | ESP32 Classic (Xtensa Dual-Core) | ESP32-C3 (RISC-V) | Description |
| :--- | :--- | :--- | :--- |
| **I2C SDA** | GPIO 21 | GPIO 8 (or 4) | MPU6050 SDA |
| **I2C SCL** | GPIO 22 | GPIO 9 (or 5) | MPU6050 SCL |
| **Vibration Motor** | GPIO 18 | GPIO 6 | Gate of N-channel MOSFET (AO3400) |
| **Buzzer** | GPIO 19 | GPIO 7 | Transistor driver for buzzer |
| **Button** | GPIO 0 (BOOT) | GPIO 0 (or 9) | Active LOW push button |

> [!WARNING]
> On ESP32 Classic, GPIO 6 to 11 are connected to the internal SPI flash chip. The sketch includes a compile-time safety check to prevent pin collisions.

## 3. How to Use with Arduino IDE

### 3.1 Setup Arduino IDE
1. Install Arduino IDE (version 2.x recommended).
2. Add ESP32 board support URL to **File -> Preferences -> Additional Boards Manager URLs**:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. In **Tools -> Board -> Boards Manager**, search for `esp32` by Espressif and click **Install**.

### 3.2 Open & Upload
1. Open the file `arduino/posture_monitor/posture_monitor.ino` in Arduino IDE.
2. Under **Tools -> Board**, select your board:
   - For ESP32 Classic: **ESP32 Dev Module**
   - For ESP32-C3: **ESP32C3 Dev Module**
3. Select your serial port in **Tools -> Port**.
4. Click **Upload** (Ctrl+U / Cmd+U).
5. Open Serial Monitor at **115200 baud**.

## 4. User Interaction
- **Hold Button > 2 seconds**: Initiates neutral posture calibration (Tare). The motor will buzz once at start and twice upon completion.
- **Double Click Button**: Snoozes alerts for 10 minutes.
- **Single Click**: Silences any currently sounding alert.
