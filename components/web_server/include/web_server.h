/**
 * @file web_server.h
 * @brief Embedded HTTP & WebSocket server for Posture Monitor.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#pragma once

#include "esp_err.h"
#include "esp_http_server.h"
#include "telemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start the HTTP server with embedded web assets,
 *        WebSocket handler (/ws), and REST APIs (/api/...).
 *
 * @return ESP_OK on success.
 */
esp_err_t web_server_start(void);

/**
 * @brief Stop the HTTP server.
 *
 * @return ESP_OK on success.
 */
esp_err_t web_server_stop(void);

/**
 * @brief Broadcast telemetry JSON to all connected WebSocket clients.
 *
 * @param[in] telem Telemetry snapshot to push.
 */
void web_server_broadcast_telemetry(const posture_telemetry_t *telem);

#ifdef __cplusplus
}
#endif
