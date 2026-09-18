/**
 * @file telemetry.c
 * @brief Thread-safe telemetry snapshot and session analytics engine.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#include "telemetry.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "TELEMETRY";

#define MAX_HISTORY_EVENTS 20

typedef struct {
    char time[16];
    char type[12];
    char title[32];
    char desc[64];
} telemetry_event_t;

static SemaphoreHandle_t s_telem_mutex = NULL;
static posture_telemetry_t s_current_snapshot = {
    .state = POSTURE_STATE_GOOD,
    .threshold = 15.0f,
    .battery_pct = 92
};

// Circular history ring buffer
static telemetry_event_t s_events[MAX_HISTORY_EVENTS];
static uint16_t s_event_head = 0;
static uint16_t s_event_count = 0;

// Session duration tracking
static uint32_t s_total_samples = 0;
static uint32_t s_good_samples = 0;
static uint32_t s_slouch_samples = 0;
static uint32_t s_alert_samples = 0;

static const char *state_to_str(posture_fsm_state_t state) {
    switch (state) {
        case POSTURE_STATE_CALIBRATING:      return "CALIBRATING";
        case POSTURE_STATE_GOOD:             return "GOOD";
        case POSTURE_STATE_SUSPECTED_SLOUCH: return "SUSPECTED_SLOUCH";
        case POSTURE_STATE_ALERT_L1:         return "ALERT_L1";
        case POSTURE_STATE_ALERT_L2:         return "ALERT_L2";
        case POSTURE_STATE_SNOOZED:          return "SNOOZED";
        default:                             return "GOOD";
    }
}

esp_err_t telemetry_init(void) {
    if (s_telem_mutex == NULL) {
        s_telem_mutex = xSemaphoreCreateMutex();
        if (!s_telem_mutex) {
            ESP_LOGE(TAG, "Failed to create telemetry mutex");
            return ESP_ERR_NO_MEM;
        }
    }
    ESP_LOGI(TAG, "Telemetry engine initialized");
    return ESP_OK;
}

void telemetry_update_snapshot(const posture_telemetry_t *snapshot) {
    if (!snapshot || !s_telem_mutex) return;

    if (xSemaphoreTake(s_telem_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        s_current_snapshot = *snapshot;

        // Update session counters
        s_total_samples++;
        switch (snapshot->state) {
            case POSTURE_STATE_GOOD:
                s_good_samples++;
                break;
            case POSTURE_STATE_SUSPECTED_SLOUCH:
                s_slouch_samples++;
                break;
            case POSTURE_STATE_ALERT_L1:
            case POSTURE_STATE_ALERT_L2:
                s_alert_samples++;
                break;
            default:
                break;
        }

        xSemaphoreGive(s_telem_mutex);
    }
}

void telemetry_get_snapshot(posture_telemetry_t *out_snapshot) {
    if (!out_snapshot || !s_telem_mutex) return;

    if (xSemaphoreTake(s_telem_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        *out_snapshot = s_current_snapshot;
        xSemaphoreGive(s_telem_mutex);
    }
}

size_t telemetry_get_snapshot_json(char *buf, size_t max_len) {
    if (!buf || max_len == 0) return 0;

    posture_telemetry_t snap;
    telemetry_get_snapshot(&snap);

    int written = snprintf(buf, max_len,
        "{\"type\":\"telemetry\","
        "\"timestamp\":%lld,"
        "\"state\":\"%s\","
        "\"pitch\":%.2f,"
        "\"roll\":%.2f,"
        "\"pitch_error\":%.2f,"
        "\"roll_error\":%.2f,"
        "\"deviation\":%.2f,"
        "\"threshold\":%.1f,"
        "\"alert_level\":%u,"
        "\"calibrated\":%s,"
        "\"snoozed\":%s,"
        "\"sensor_ok\":%s,"
        "\"battery\":%u,"
        "\"rssi\":%d,"
        "\"free_heap\":%lu,"
        "\"uptime\":%lu}",
        (long long)snap.timestamp_ms,
        state_to_str(snap.state),
        snap.pitch,
        snap.roll,
        snap.pitch_error,
        snap.roll_error,
        snap.deviation,
        snap.threshold,
        snap.alert_level,
        snap.is_calibrating ? "true" : "false",
        snap.is_snoozed ? "true" : "false",
        snap.sensor_ok ? "true" : "false",
        snap.battery_pct,
        snap.wifi_rssi,
        (unsigned long)snap.free_heap,
        (unsigned long)snap.uptime_s
    );

    return (written > 0 && (size_t)written < max_len) ? (size_t)written : 0;
}

void telemetry_record_event(const char *type, const char *title, const char *desc) {
    if (!s_telem_mutex) return;

    if (xSemaphoreTake(s_telem_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        uint32_t sec = (uint32_t)(esp_timer_get_time() / 1000000ULL);
        uint32_t hrs = sec / 3600;
        uint32_t mins = (sec % 3600) / 60;
        uint32_t secs = sec % 60;

        telemetry_event_t *evt = &s_events[s_event_head];
        snprintf(evt->time, sizeof(evt->time), "%02lu:%02lu:%02lu", (unsigned long)hrs, (unsigned long)mins, (unsigned long)secs);
        strncpy(evt->type, type ? type : "good", sizeof(evt->type) - 1);
        strncpy(evt->title, title ? title : "Event", sizeof(evt->title) - 1);
        strncpy(evt->desc, desc ? desc : "", sizeof(evt->desc) - 1);

        s_event_head = (s_event_head + 1) % MAX_HISTORY_EVENTS;
        if (s_event_count < MAX_HISTORY_EVENTS) {
            s_event_count++;
        }

        xSemaphoreGive(s_telem_mutex);
    }
}

size_t telemetry_get_history_json(char *buf, size_t max_len) {
    if (!buf || max_len == 0 || !s_telem_mutex) return 0;

    size_t offset = 0;
    if (xSemaphoreTake(s_telem_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        uint32_t total = (s_total_samples > 0) ? s_total_samples : 1;
        uint32_t good_pct = (s_good_samples * 100) / total;
        uint32_t slouch_pct = (s_slouch_samples * 100) / total;
        uint32_t alert_pct = (s_alert_samples * 100) / total;

        int written = snprintf(buf, max_len,
            "{\"good_pct\":%lu,\"slouch_pct\":%lu,\"alert_pct\":%lu,\"events\":[",
            (unsigned long)good_pct, (unsigned long)slouch_pct, (unsigned long)alert_pct);

        if (written > 0 && (size_t)written < max_len) {
            offset += (size_t)written;
        }

        // Iterate backwards from newest to oldest
        for (uint16_t i = 0; i < s_event_count; i++) {
            int idx = (s_event_head - 1 - i + MAX_HISTORY_EVENTS) % MAX_HISTORY_EVENTS;
            const telemetry_event_t *e = &s_events[idx];

            written = snprintf(buf + offset, max_len - offset,
                "%s{\"time\":\"%s\",\"type\":\"%s\",\"title\":\"%s\",\"desc\":\"%s\"}",
                (i > 0) ? "," : "",
                e->time, e->type, e->title, e->desc);

            if (written > 0 && (size_t)written < (max_len - offset)) {
                offset += (size_t)written;
            } else {
                break;
            }
        }

        if (offset + 3 < max_len) {
            buf[offset++] = ']';
            buf[offset++] = '}';
            buf[offset] = '\0';
        }

        xSemaphoreGive(s_telem_mutex);
    }

    return offset;
}
