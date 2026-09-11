# MPU6050 Sensor Component

## 1. Overview
The `mpu6050_sensor` component integrates the official `espressif/mpu6050` component from the ESP Component Registry, adding an advanced orientation calculation pipeline tailored for body posture monitoring.

## 2. Key Technical Innovations
- **I2C Bus Recovery**: Performs a hardware clock pulsing routine (9 clock pulses on SCL) prior to driver initialization to unwedge any slave hanging the SDA line.
- **Actuator Vibration Decoupling**: Accepts a `freeze_accel_bias` parameter from `actuator_manager`. When the mini vibration motor is running, the complementary filter suppresses high-frequency accelerometer vibration spikes and relies exclusively on gyroscope angular rate integration.
- **Parametric Complementary Filter**: Blends accelerometer gravity vector ($1 - \alpha$) with high-rate gyroscope integration ($\alpha$) configured via Kconfig.

## 3. Trigonometric Angle Calculation
$$\text{Pitch}_{\text{acc}} = \text{atan2}\left(a_y, \sqrt{a_x^2 + a_z^2}\right) \times \frac{180}{\pi}$$

$$\text{Roll}_{\text{acc}} = \text{atan2}\left(-a_x, a_z\right) \times \frac{180}{\pi}$$

$$\theta_t = \alpha \cdot (\theta_{t-1} + \omega \cdot \Delta t) + (1 - \alpha) \cdot \theta_{\text{acc}}$$

## 4. Configuration Options (Kconfig)
- `CONFIG_POSTURE_I2C_SDA_GPIO`: GPIO line for I2C data (default: 4).
- `CONFIG_POSTURE_I2C_SCL_GPIO`: GPIO line for I2C clock (default: 5).
- `CONFIG_POSTURE_I2C_FREQ_HZ`: I2C clock frequency (default: 400000 Hz).
- `CONFIG_POSTURE_FILTER_ALPHA_X100`: Gyroscope weighting coefficient $\alpha \times 100$ (default: 96 $\rightarrow$ 0.96).

## 5. API Reference
- `esp_err_t mpu6050_sensor_init(void)`: Recovers I2C bus, initializes MPU6050 and configures $\pm 2g$ and $\pm 250^\circ/\text{s}$.
- `esp_err_t mpu6050_sensor_update(float dt, bool freeze_accel_bias, posture_angles_t *out_angles)`: Computes current orientation angles.
- `esp_err_t mpu6050_sensor_sleep(void)`: Enters low-power sensor sleep mode.
