# Actuator Manager Component

## 1. Overview
The `actuator_manager` component provides an asynchronous, non-blocking sequencing engine for the Mini Vibration Motor and Buzzer on the ESP32-C3. It ensures that complex haptic pulses and audible alarms do not stall or introduce jitter into the real-time posture sampling loop.

## 2. Technical Features
- **Deterministic 100ms Base Tick**: Powered by the hardware-backed `esp_timer` running in a dedicated FreeRTOS timer task.
- **Progressive Alert Patterns**:
  - `ALERT_PATTERN_IDLE`: Quiescent state, low power consumption.
  - `ALERT_PATTERN_CALIB_START`: 200ms single confirmation buzz.
  - `ALERT_PATTERN_CALIB_DONE`: Dual 150ms confirmation pulses.
  - `ALERT_PATTERN_LEVEL1_HAPTIC`: Intermittent 200ms vibration pulses every 1.2 seconds.
  - `ALERT_PATTERN_LEVEL2_ALARM`: High-urgency synchronous vibration (200ms) and buzzer audio chirps every 500ms.
- **Acoustic Sensor Decoupling Signal**: Exposes `actuator_manager_is_vibrating()` so that IMU filters can suppress mechanical motor vibration artifacts.

## 3. Electrical Driving Requirements
- **Vibration Motor**: Must be driven via an N-channel logic-level MOSFET (e.g., AO3400, 2N7002) with a Schottky flyback diode (e.g., 1N5819) across motor terminals and decoupling capacitor ($10\mu\text{F} \parallel 100\text{nF}$).
- **Buzzer**: Driven via NPN transistor (e.g., S8050) or MOSFET depending on buzzer type.

## 4. API Reference
- `esp_err_t actuator_manager_init(void)`: Configures GPIOs and starts timer.
- `void actuator_manager_set_pattern(alert_pattern_t pattern)`: Sets active pattern asynchronously.
- `bool actuator_manager_is_vibrating(void)`: Returns current physical state of the motor.
