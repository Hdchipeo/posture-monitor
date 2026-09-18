/**
 * @file wifi_manager.h
 * @brief Wi-Fi SoftAP and Station connection management for Posture Monitor.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POSTURE_WIFI_AP_SSID      "Posture-Monitor-AP"
#define POSTURE_WIFI_AP_PASS      "posture123"
#define POSTURE_WIFI_AP_MAX_CONN  4
#define POSTURE_WIFI_AP_CHANNEL   6

/**
 * @brief Initialize Wi-Fi subsystem and start SoftAP mode.
 * Allows phones and laptops to connect directly without a router.
 *
 * @return ESP_OK on success.
 */
esp_err_t wifi_manager_init_softap(void);

/**
 * @brief Get active Wi-Fi RSSI (if connected to station) or simulated AP signal.
 *
 * @return RSSI in dBm.
 */
int8_t wifi_manager_get_rssi(void);

#ifdef __cplusplus
}
#endif
