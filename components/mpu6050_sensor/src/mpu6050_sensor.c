/**
 * @file mpu6050_sensor.c
 * @brief Driver implementation integrating espressif/mpu6050 with vibration-aware complementary filtering.
 */

#include "mpu6050_sensor.h"
#include "mpu6050.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include <math.h>

static const char *TAG = "MPU6050_SENSOR";

#ifndef CONFIG_POSTURE_I2C_SDA_GPIO
#define CONFIG_POSTURE_I2C_SDA_GPIO 21
#endif

#ifndef CONFIG_POSTURE_I2C_SCL_GPIO
#define CONFIG_POSTURE_I2C_SCL_GPIO 22
#endif

#if CONFIG_IDF_TARGET_ESP32
#if (CONFIG_POSTURE_I2C_SDA_GPIO >= 6 && CONFIG_POSTURE_I2C_SDA_GPIO <= 11)
#error "CONFIG_POSTURE_I2C_SDA_GPIO cannot use GPIO 6-11 on ESP32! These pins are dedicated to SPI Flash."
#endif
#if (CONFIG_POSTURE_I2C_SCL_GPIO >= 6 && CONFIG_POSTURE_I2C_SCL_GPIO <= 11)
#error "CONFIG_POSTURE_I2C_SCL_GPIO cannot use GPIO 6-11 on ESP32! These pins are dedicated to SPI Flash."
#endif
#endif

#ifndef CONFIG_POSTURE_I2C_PORT_NUM
#define CONFIG_POSTURE_I2C_PORT_NUM 0
#endif

#ifndef CONFIG_POSTURE_I2C_FREQ_HZ
#define CONFIG_POSTURE_I2C_FREQ_HZ 400000
#endif

#ifndef CONFIG_POSTURE_FILTER_ALPHA_X100
#define CONFIG_POSTURE_FILTER_ALPHA_X100 96
#endif

#define RAD_TO_DEG 57.29577951308232f

static mpu6050_handle_t s_mpu6050_handle = NULL;
static float s_pitch = 0.0f;
static float s_roll = 0.0f;
static bool s_filter_initialized = false;
static bool s_is_healthy = false;
static uint32_t s_consecutive_failures = 0;
static int64_t s_last_recovery_time = 0;

/**
 * @brief Clock 9 pulses on SCL if SDA is stuck LOW to release hung I2C bus.
 */
static void i2c_bus_recovery(gpio_num_t sda, gpio_num_t scl) {
    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << sda) | (1ULL << scl),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&conf);

    if (gpio_get_level(sda) == 0) {
        ESP_LOGW(TAG, "SDA line held LOW by slave! Initiating 9-clock recovery sequence...");
        for (int i = 0; i < 9; i++) {
            gpio_set_level(scl, 0);
            esp_rom_delay_us(5);
            gpio_set_level(scl, 1);
            esp_rom_delay_us(5);
            if (gpio_get_level(sda) == 1) {
                break;
            }
        }
        // Generate STOP condition
        gpio_set_level(sda, 0);
        esp_rom_delay_us(5);
        gpio_set_level(scl, 1);
        esp_rom_delay_us(5);
        gpio_set_level(sda, 1);
        esp_rom_delay_us(5);
    }
}

esp_err_t mpu6050_sensor_init(void) {
    // 1. Check & Recover bus if needed
    i2c_bus_recovery((gpio_num_t)CONFIG_POSTURE_I2C_SDA_GPIO, (gpio_num_t)CONFIG_POSTURE_I2C_SCL_GPIO);

    // 2. Configure I2C controller
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_POSTURE_I2C_SDA_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = CONFIG_POSTURE_I2C_SCL_GPIO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = CONFIG_POSTURE_I2C_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(CONFIG_POSTURE_I2C_PORT_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(CONFIG_POSTURE_I2C_PORT_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return err;
    }

    // 3. Create MPU6050 handle using Espressif component
    if (s_mpu6050_handle) {
        mpu6050_delete(s_mpu6050_handle);
        s_mpu6050_handle = NULL;
    }

    s_mpu6050_handle = mpu6050_create(CONFIG_POSTURE_I2C_PORT_NUM, MPU6050_I2C_ADDRESS);
    if (!s_mpu6050_handle) {
        ESP_LOGE(TAG, "Failed to create mpu6050 device handle at addr 0x%02X", MPU6050_I2C_ADDRESS);
        return ESP_FAIL;
    }

    err = mpu6050_wake_up(s_mpu6050_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wake up MPU6050: %s", esp_err_to_name(err));
        return err;
    }

    // Set +/- 2g for accelerometer and +/- 250 dps for gyroscope
    err = mpu6050_config(s_mpu6050_handle, ACCE_FS_2G, GYRO_FS_250DPS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure MPU6050 full-scale ranges: %s", esp_err_to_name(err));
        return err;
    }

    s_filter_initialized = false;
    s_is_healthy = true;
    s_consecutive_failures = 0;
    ESP_LOGI(TAG, "MPU6050 sensor successfully configured (I2C SDA: %d, SCL: %d, Freq: %d Hz)",
             CONFIG_POSTURE_I2C_SDA_GPIO, CONFIG_POSTURE_I2C_SCL_GPIO, CONFIG_POSTURE_I2C_FREQ_HZ);
    return ESP_OK;
}

esp_err_t mpu6050_sensor_update(float dt, bool freeze_accel_bias, posture_angles_t *out_angles) {
    if (!s_mpu6050_handle || !out_angles) return ESP_ERR_INVALID_STATE;

    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;

    esp_err_t err = mpu6050_get_acce(s_mpu6050_handle, &acce);
    if (err == ESP_OK) {
        err = mpu6050_get_gyro(s_mpu6050_handle, &gyro);
    }

    if (err != ESP_OK) {
        s_consecutive_failures++;
        s_is_healthy = false;
        int64_t now = esp_timer_get_time();
        if (s_consecutive_failures >= 10 && (now - s_last_recovery_time > 2000000LL)) {
            s_last_recovery_time = now;
            ESP_LOGW(TAG, "MPU6050 I2C communication stalled (%lu errors). Attempting recovery...",
                     (unsigned long)s_consecutive_failures);
            if (mpu6050_sensor_init() == ESP_OK) {
                ESP_LOGI(TAG, "MPU6050 recovered and re-initialized successfully!");
                s_consecutive_failures = 0;
                s_is_healthy = true;
            }
        }
        return err;
    }

    s_consecutive_failures = 0;
    s_is_healthy = true;

    // Convert raw measurements to physical angles
    // acce values are in g (where 1.0 = 1g)
    float ax = acce.acce_x;
    float ay = acce.acce_y;
    float az = acce.acce_z;

    // gyro values are in deg/sec
    float gx = gyro.gyro_x;
    float gy = gyro.gyro_y;
    float gz = gyro.gyro_z;

    float pitch_acc = atan2f(ay, sqrtf(ax * ax + az * az)) * RAD_TO_DEG;
    float roll_acc  = atan2f(-ax, az) * RAD_TO_DEG;

    if (!s_filter_initialized) {
        s_pitch = pitch_acc;
        s_roll = roll_acc;
        s_filter_initialized = true;
    } else {
        if (freeze_accel_bias) {
            // While the mini vibration motor is active, freeze the accelerometer gravity reference
            // to eliminate actuator acoustic vibration noise and integrate gyroscope angular velocity only.
            s_pitch += gx * dt;
            s_roll  += gy * dt;
        } else {
            float alpha = (float)CONFIG_POSTURE_FILTER_ALPHA_X100 / 100.0f;
            s_pitch = alpha * (s_pitch + gx * dt) + (1.0f - alpha) * pitch_acc;
            s_roll  = alpha * (s_roll  + gy * dt) + (1.0f - alpha) * roll_acc;
        }
    }

    out_angles->pitch = s_pitch;
    out_angles->roll = s_roll;
    out_angles->yaw_rate = gz;

    return ESP_OK;
}

bool mpu6050_sensor_is_healthy(void) {
    return s_is_healthy;
}

esp_err_t mpu6050_sensor_sleep(void) {
    if (!s_mpu6050_handle) return ESP_ERR_INVALID_STATE;
    // Put MPU6050 into low power sleep mode
    return mpu6050_sleep(s_mpu6050_handle);
}
