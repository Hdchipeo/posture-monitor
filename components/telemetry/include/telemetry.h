/**
 * @file telemetry.h
 * @brief Thread-safe telemetry snapshot and session history buffer for Posture Monitor.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "posture_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Complete real-time snapshot model matching the web dashboard contract.
 */
typedef struct {
    int64_t            timestamp_ms;    /*!< Epoch timestamp in milliseconds */
    posture_fsm_state_t state;          /*!< Current FSM evaluation state */
    float              pitch;           /*!< Filtered pitch angle in degrees */
    float              roll;            /*!< Filtered roll angle in degrees */
    float              pitch_error;     /*!< Absolute delta from pitch baseline */
    float              roll_error;      /*!< Absolute delta from roll baseline */
    float              deviation;       /*!< Combined angular deviation */
    float              threshold;       /*!< Active trigger threshold */
    uint8_t            alert_level;     /*!< 0=None, 1=Haptic L1, 2=Alarm L2 */
    bool               is_calibrating;  /*!< True if in tare calibration */
    bool               is_snoozed;      /*!< True if alerts currently snoozed */
    bool               sensor_ok;       /*!< True if MPU6050 sensor is communicating properly */
    uint8_t            battery_pct;     /*!< Estimated battery percentage */
    int8_t             wifi_rssi;       /*!< Wi-Fi RSSI in dBm */
    uint32_t           free_heap;       /*!< Free heap memory in bytes */
    uint32_t           uptime_s;        /*!< System uptime in seconds */
} posture_telemetry_t;

/**
 * @brief Initialize the telemetry component and mutex locks.
 *
 * @return ESP_OK on success.
 */
esp_err_t telemetry_init(void);

/**
 * @brief Atomically publish an updated telemetry snapshot from the sensor task.
 *
 * @param[in] snapshot Pointer to the latest telemetry values.
 */
void telemetry_update_snapshot(const posture_telemetry_t *snapshot);

/**
 * @brief Thread-safely copy the latest telemetry snapshot.
 *
 * @param[out] out_snapshot Destination pointer.
 */
void telemetry_get_snapshot(posture_telemetry_t *out_snapshot);

/**
 * @brief Serialize current telemetry snapshot to a compact JSON string.
 *
 * @param[out] buf     Destination buffer.
 * @param[in]  max_len Maximum buffer length.
 * @return Number of characters written.
 */
size_t telemetry_get_snapshot_json(char *buf, size_t max_len);

/**
 * @brief Record an alert, calibration, or correction event into the circular ring buffer.
 *
 * @param[in] type  Event category ("good", "warn", "alert", "calib")
 * @param[in] title Event header title
 * @param[in] desc  Detailed description
 */
void telemetry_record_event(const char *type, const char *title, const char *desc);

/**
 * @brief Serialize session summary and event history to JSON.
 *
 * @param[out] buf     Destination buffer.
 * @param[in]  max_len Maximum buffer length.
 * @return Number of characters written.
 */
size_t telemetry_get_history_json(char *buf, size_t max_len);

#ifdef __cplusplus
}
#endif
