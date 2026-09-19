/**
 * @file battery_monitor.c
 * @brief High-precision battery state-of-charge estimator for ESP32-C3.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#include "battery_monitor.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "BAT_MON";

#ifndef CONFIG_POSTURE_BATTERY_ADC_GPIO
#define CONFIG_POSTURE_BATTERY_ADC_GPIO 1
#endif

#ifndef CONFIG_POSTURE_BATTERY_LOW_THRESH_PCT
#define CONFIG_POSTURE_BATTERY_LOW_THRESH_PCT 15
#endif

// On ESP32-C3: GPIO 1 is ADC1_CHANNEL_1
#define BATTERY_ADC_UNIT     ADC_UNIT_1
#define BATTERY_ADC_CHANNEL  ADC_CHANNEL_1
#define BATTERY_ADC_ATTEN    ADC_ATTEN_DB_12
#define OVERSAMPLE_COUNT     16

// 100k - 100k voltage divider: V_cell = V_pin * (100 + 100) / 100 = 2.0
#define DIVIDER_RATIO_NUM    2
#define DIVIDER_RATIO_DENOM  1

typedef struct {
    uint16_t mv;
    uint8_t  pct;
} soc_point_t;

// Standard Li-ion (3.7V nominal, 4.2V max) Open-Circuit Voltage discharge table
static const soc_point_t s_soc_curve[] = {
    { 4180, 100 },
    { 4060,  90 },
    { 3980,  80 },
    { 3920,  70 },
    { 3870,  60 },
    { 3820,  50 },
    { 3780,  40 },
    { 3740,  30 },
    { 3680,  20 },
    { 3550,  10 },
    { 3400,   5 },
    { 3250,   0 }
};
#define SOC_CURVE_SIZE (sizeof(s_soc_curve) / sizeof(s_soc_curve[0]))

static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_cali_handle_t s_cali_handle = NULL;
static bool s_cali_enabled = false;
static SemaphoreHandle_t s_bat_mutex = NULL;

static uint32_t s_filtered_voltage_mv = 0;
static uint8_t s_battery_pct = 100;
static bool s_is_initialized = false;

static uint8_t calculate_soc(uint32_t cell_mv) {
    // When cell_mv < 2500mV, no battery is connected (powered directly by USB rail)
    if (cell_mv < 2500) {
        return 100; // Treat as USB powered full capacity
    }
    if (cell_mv >= s_soc_curve[0].mv) {
        return 100;
    }
    if (cell_mv <= s_soc_curve[SOC_CURVE_SIZE - 1].mv) {
        return 0;
    }

    for (size_t i = 0; i < SOC_CURVE_SIZE - 1; i++) {
        if (cell_mv <= s_soc_curve[i].mv && cell_mv >= s_soc_curve[i + 1].mv) {
            uint32_t v_high = s_soc_curve[i].mv;
            uint32_t v_low  = s_soc_curve[i + 1].mv;
            uint32_t p_high = s_soc_curve[i].pct;
            uint32_t p_low  = s_soc_curve[i + 1].pct;

            uint32_t pct = p_low + ((cell_mv - v_low) * (p_high - p_low)) / (v_high - v_low);
            return (uint8_t)pct;
        }
    }
    return 0;
}

esp_err_t battery_monitor_init(void) {
    if (s_is_initialized) {
        return ESP_OK;
    }

    s_bat_mutex = xSemaphoreCreateMutex();
    if (!s_bat_mutex) {
        ESP_LOGE(TAG, "Failed to create battery mutex");
        return ESP_ERR_NO_MEM;
    }

    // 1. Initialize ADC1 Oneshot Unit
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = BATTERY_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_config, &s_adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC1 unit: %s", esp_err_to_name(err));
        return err;
    }

    // 2. Configure ADC Channel on GPIO 1
    adc_oneshot_chan_cfg_t chan_config = {
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_oneshot_config_channel(s_adc_handle, BATTERY_ADC_CHANNEL, &chan_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel 1: %s", esp_err_to_name(err));
        return err;
    }

    gpio_sleep_sel_dis((gpio_num_t)CONFIG_POSTURE_BATTERY_ADC_GPIO);

    // 3. Configure Hardware eFuse Calibration (Curve fitting on ESP32-C3)
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = BATTERY_ADC_UNIT,
        .chan = BATTERY_ADC_CHANNEL,
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_cali_create_scheme_curve_fitting(&cali_config, &s_cali_handle);
    if (err == ESP_OK) {
        s_cali_enabled = true;
        ESP_LOGI(TAG, "ADC Curve-fitting calibration active on GPIO 1");
    } else {
        ESP_LOGW(TAG, "Curve-fitting calibration unavailable (%s), using uncalibrated fallback", esp_err_to_name(err));
    }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = BATTERY_ADC_UNIT,
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_cali_create_scheme_line_fitting(&cali_config, &s_cali_handle);
    if (err == ESP_OK) {
        s_cali_enabled = true;
        ESP_LOGI(TAG, "ADC Line-fitting calibration active on GPIO 1");
    } else {
        ESP_LOGW(TAG, "Line-fitting calibration unavailable (%s), using uncalibrated fallback", esp_err_to_name(err));
    }
#endif

    s_is_initialized = true;

    // Take initial reading immediately to seed filtered values
    battery_monitor_sample();

    ESP_LOGI(TAG, "Battery monitor initialized on GPIO 1 (Divider: 100k-100k). Initial: %lu mV (%u%%)",
             (unsigned long)s_filtered_voltage_mv, s_battery_pct);

    return ESP_OK;
}

esp_err_t battery_monitor_sample(void) {
    if (!s_is_initialized || !s_adc_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    // Multisampling to reject RF noise and impedance droop
    uint32_t raw_accum = 0;
    int sample = 0;
    int valid_samples = 0;

    for (int i = 0; i < OVERSAMPLE_COUNT; i++) {
        esp_err_t err = adc_oneshot_read(s_adc_handle, BATTERY_ADC_CHANNEL, &sample);
        if (err == ESP_OK) {
            raw_accum += sample;
            valid_samples++;
        }
    }

    if (valid_samples == 0) {
        ESP_LOGW(TAG, "ADC read failed across all oversamples");
        return ESP_FAIL;
    }

    int raw_avg = (int)(raw_accum / valid_samples);
    int pin_voltage_mv = 0;

    if (s_cali_enabled && s_cali_handle) {
        adc_cali_raw_to_voltage(s_cali_handle, raw_avg, &pin_voltage_mv);
    } else {
        // Fallback approximation for 12dB attenuation (0 ~ 2500mV full scale on ESP32-C3)
        pin_voltage_mv = (raw_avg * 2500) / 4095;
    }

    // Multiply by divider ratio: 100k + 100k / 100k = 2.0
    uint32_t measured_cell_mv = ((uint32_t)pin_voltage_mv * DIVIDER_RATIO_NUM) / DIVIDER_RATIO_DENOM;

    if (xSemaphoreTake(s_bat_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        if (s_filtered_voltage_mv == 0) {
            s_filtered_voltage_mv = measured_cell_mv;
        } else {
            // Exponential Moving Average (EMA) with alpha = 0.2
            // s_filtered = 0.2 * new + 0.8 * old = (2 * new + 8 * old) / 10
            s_filtered_voltage_mv = (measured_cell_mv * 2 + s_filtered_voltage_mv * 8) / 10;
        }
        s_battery_pct = calculate_soc(s_filtered_voltage_mv);
        xSemaphoreGive(s_bat_mutex);
    }

    return ESP_OK;
}

uint8_t battery_monitor_get_percentage(void) {
    uint8_t pct = 100;
    if (s_bat_mutex && xSemaphoreTake(s_bat_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        pct = s_battery_pct;
        xSemaphoreGive(s_bat_mutex);
    } else {
        pct = s_battery_pct;
    }
    return pct;
}

uint32_t battery_monitor_get_voltage_mv(void) {
    uint32_t mv = 0;
    if (s_bat_mutex && xSemaphoreTake(s_bat_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        mv = s_filtered_voltage_mv;
        xSemaphoreGive(s_bat_mutex);
    } else {
        mv = s_filtered_voltage_mv;
    }
    return mv;
}

bool battery_monitor_is_low(void) {
    uint32_t mv = battery_monitor_get_voltage_mv();
    // Do not flag low battery if disconnected / powered directly via USB
    if (mv < 2500) {
        return false;
    }
    return battery_monitor_get_percentage() < CONFIG_POSTURE_BATTERY_LOW_THRESH_PCT;
}
