/**
 * @file posture_core.c
 * @brief Implementation of posture state machine and progressive alert escalation.
 */

#include "posture_core.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "POSTURE_CORE";

#ifndef CONFIG_POSTURE_CALIBRATION_SAMPLES
#define CONFIG_POSTURE_CALIBRATION_SAMPLES 100
#endif

static posture_calib_data_t s_calib;
static posture_fsm_state_t s_state = POSTURE_STATE_GOOD;
static float s_slouch_timer = 0.0f;
static float s_snooze_timer = 0.0f;

// Calibration tracking
static bool s_is_calibrating = false;
static uint32_t s_calib_sample_count = 0;
static float s_sum_pitch = 0.0f;
static float s_sum_roll = 0.0f;

esp_err_t posture_core_init(const posture_calib_data_t *initial_calib) {
    if (!initial_calib) return ESP_ERR_INVALID_ARG;

    s_calib = *initial_calib;
    s_state = POSTURE_STATE_GOOD;
    s_slouch_timer = 0.0f;
    s_snooze_timer = 0.0f;
    s_is_calibrating = false;

    ESP_LOGI(TAG, "Posture core initialized (Thresh: %.1f deg, L1: %u s, L2: %u s)",
             s_calib.angle_threshold, (unsigned int)s_calib.slouch_delay_s, (unsigned int)s_calib.escalation_delay_s);
    return ESP_OK;
}

void posture_core_start_calibration(void) {
    s_is_calibrating = true;
    s_calib_sample_count = 0;
    s_sum_pitch = 0.0f;
    s_sum_roll = 0.0f;
    s_state = POSTURE_STATE_CALIBRATING;
    ESP_LOGI(TAG, "Starting zero-reference calibration (%d samples)...", CONFIG_POSTURE_CALIBRATION_SAMPLES);
}

bool posture_core_is_calibrating(void) {
    return s_is_calibrating;
}

esp_err_t posture_core_add_calibration_sample(const posture_angles_t *angles, posture_calib_data_t *out_calib) {
    if (!s_is_calibrating || !angles) return ESP_ERR_INVALID_STATE;

    s_sum_pitch += angles->pitch;
    s_sum_roll += angles->roll;
    s_calib_sample_count++;

    if (s_calib_sample_count >= CONFIG_POSTURE_CALIBRATION_SAMPLES) {
        s_calib.pitch_offset = s_sum_pitch / (float)s_calib_sample_count;
        s_calib.roll_offset = s_sum_roll / (float)s_calib_sample_count;
        s_is_calibrating = false;
        s_state = POSTURE_STATE_GOOD;
        s_slouch_timer = 0.0f;

        if (out_calib) {
            *out_calib = s_calib;
        }

        ESP_LOGI(TAG, "Calibration completed: Pitch_Base=%.2f deg, Roll_Base=%.2f deg",
                 s_calib.pitch_offset, s_calib.roll_offset);
        return ESP_OK;
    }

    return ESP_ERR_NOT_FINISHED;
}

void posture_core_snooze(uint32_t seconds) {
    s_snooze_timer = (float)seconds;
    s_state = POSTURE_STATE_SNOOZED;
    s_slouch_timer = 0.0f;
    ESP_LOGI(TAG, "Posture monitor snoozed for %u seconds", (unsigned int)seconds);
}

void posture_core_process_sample(float dt, const posture_angles_t *current_angles, posture_fsm_state_t *out_state) {
    if (!current_angles) return;

    if (s_is_calibrating) {
        *out_state = POSTURE_STATE_CALIBRATING;
        return;
    }

    // Check Snooze mode
    if (s_snooze_timer > 0.0f) {
        s_snooze_timer -= dt;
        if (s_snooze_timer <= 0.0f) {
            s_snooze_timer = 0.0f;
            s_state = POSTURE_STATE_GOOD;
            ESP_LOGI(TAG, "Snooze expired. Resuming active monitoring.");
        } else {
            *out_state = POSTURE_STATE_SNOOZED;
            return;
        }
    }

    // Compute deviation from calibrated zero reference
    float delta_pitch = fabsf(current_angles->pitch - s_calib.pitch_offset);
    float delta_roll  = fabsf(current_angles->roll  - s_calib.roll_offset);

    bool is_slouching = (delta_pitch > s_calib.angle_threshold) || (delta_roll > s_calib.angle_threshold);

    if (!is_slouching) {
        // Returned to good posture: instantly clear slouch timer and reset to GOOD state
        if (s_state != POSTURE_STATE_GOOD) {
            ESP_LOGI(TAG, "Posture corrected to GOOD (delta_p=%.1f, delta_r=%.1f)", delta_pitch, delta_roll);
        }
        s_state = POSTURE_STATE_GOOD;
        s_slouch_timer = 0.0f;
    } else {
        // Accumulate slouch duration
        s_slouch_timer += dt;

        if (s_slouch_timer < (float)s_calib.slouch_delay_s) {
            s_state = POSTURE_STATE_SUSPECTED_SLOUCH;
        } else if (s_slouch_timer < (float)(s_calib.slouch_delay_s + s_calib.escalation_delay_s)) {
            if (s_state != POSTURE_STATE_ALERT_L1) {
                ESP_LOGW(TAG, "Posture SLOUCH persisted for %.1f s -> Entering LEVEL 1 ALERT", s_slouch_timer);
            }
            s_state = POSTURE_STATE_ALERT_L1;
        } else {
            if (s_state != POSTURE_STATE_ALERT_L2) {
                ESP_LOGE(TAG, "Posture SLOUCH persistent for %.1f s -> Escalating to LEVEL 2 ALARM", s_slouch_timer);
            }
            s_state = POSTURE_STATE_ALERT_L2;
        }
    }

    *out_state = s_state;
}

void posture_core_get_calib(posture_calib_data_t *out_calib) {
    if (out_calib) *out_calib = s_calib;
}

void posture_core_update_calib(const posture_calib_data_t *new_calib) {
    if (new_calib) s_calib = *new_calib;
}
