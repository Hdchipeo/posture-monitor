/**
 * @file web_server.c
 * @brief Production HTTP & WebSocket Server serving Apple Health dashboard for ESP32-C3.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#include "web_server.h"
#include "telemetry.h"
#include "posture_core.h"
#include "storage_manager.h"
#include "actuator_manager.h"
#include "battery_monitor.h"
#include "status_led.h"
#include "mpu6050_sensor.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "WEB_SERVER";

// Embedded Web Assets via CMake EMBED_TXTFILES
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

extern const uint8_t style_css_start[]  asm("_binary_style_css_start");
extern const uint8_t style_css_end[]    asm("_binary_style_css_end");

extern const uint8_t app_js_start[]     asm("_binary_app_js_start");
extern const uint8_t app_js_end[]       asm("_binary_app_js_end");

extern const uint8_t chart_js_start[]   asm("_binary_chart_js_start");
extern const uint8_t chart_js_end[]     asm("_binary_chart_js_end");

extern const uint8_t skeleton3d_js_start[] asm("_binary_skeleton3d_js_start");
extern const uint8_t skeleton3d_js_end[]   asm("_binary_skeleton3d_js_end");

static httpd_handle_t s_server = NULL;

// --------------------------------------------------------------------------
// STATIC FILE CHUNKED STREAMING (Eliminates TCP Send Buffer Stalls)
// --------------------------------------------------------------------------
static esp_err_t send_embedded_file_chunked(httpd_req_t *req, const uint8_t *start, const uint8_t *end, const char *content_type) {
    httpd_resp_set_type(req, content_type);
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Connection", "close");
    size_t total_len = end - start;
    if (total_len > 0 && *(end - 1) == '\0') {
        total_len--;
    }
    size_t offset = 0;
    const size_t chunk_size = 2048; // 2 KB chunks, safely below TCP_SND_BUF 5760

    ESP_LOGI(TAG, "HTTP GET %s (%zu bytes) -> client fd=%d", req->uri, total_len, httpd_req_to_sockfd(req));

    while (offset < total_len) {
        size_t send_len = total_len - offset;
        if (send_len > chunk_size) {
            send_len = chunk_size;
        }
        esp_err_t res = httpd_resp_send_chunk(req, (const char *)(start + offset), send_len);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Chunk send failed on %s at %zu/%zu: %s", req->uri, offset, total_len, esp_err_to_name(res));
            httpd_resp_send_chunk(req, NULL, 0);
            return res;
        }
        offset += send_len;
    }
    return httpd_resp_send_chunk(req, NULL, 0); // Terminate chunked transfer
}

static esp_err_t index_html_handler(httpd_req_t *req) {
    return send_embedded_file_chunked(req, index_html_start, index_html_end, "text/html");
}

static esp_err_t style_css_handler(httpd_req_t *req) {
    return send_embedded_file_chunked(req, style_css_start, style_css_end, "text/css");
}

static esp_err_t app_js_handler(httpd_req_t *req) {
    return send_embedded_file_chunked(req, app_js_start, app_js_end, "application/javascript");
}

static esp_err_t chart_js_handler(httpd_req_t *req) {
    return send_embedded_file_chunked(req, chart_js_start, chart_js_end, "application/javascript");
}

static esp_err_t skeleton3d_js_handler(httpd_req_t *req) {
    return send_embedded_file_chunked(req, skeleton3d_js_start, skeleton3d_js_end, "application/javascript");
}

// --------------------------------------------------------------------------
// WEBSOCKET HANDLER (/ws)
// --------------------------------------------------------------------------
static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WebSocket client handshake completed (fd=%d)", httpd_req_to_sockfd(req));
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        return ret;
    }

    if (ws_pkt.len) {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf) {
            ws_pkt.payload = buf;
            ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
            if (ret == ESP_OK) {
                ESP_LOGD(TAG, "WS payload received: %s", (char *)buf);
            }
            free(buf);
        }
    }

    return ESP_OK;
}

// Broadcast frame struct for async helper
struct ws_async_send_arg {
    int fd;
    char payload[512];
};

static void ws_async_send(void *arg) {
    struct ws_async_send_arg *a = (struct ws_async_send_arg *)arg;
    if (!a) return;

    if (s_server && httpd_ws_get_fd_info(s_server, a->fd) == HTTPD_WS_CLIENT_WEBSOCKET) {
        httpd_ws_frame_t ws_pkt;
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
        ws_pkt.payload = (uint8_t *)a->payload;
        ws_pkt.len = strlen(a->payload);
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;

        httpd_ws_send_frame_async(s_server, a->fd, &ws_pkt);
    }
    free(a);
}

void web_server_broadcast_telemetry(const posture_telemetry_t *telem) {
    if (!s_server) return;

    size_t num_fds = 8;
    int client_fds[8];
    if (httpd_get_client_list(s_server, &num_fds, client_fds) != ESP_OK || num_fds == 0) {
        return;
    }

    bool has_ws = false;
    for (size_t i = 0; i < num_fds; i++) {
        if (httpd_ws_get_fd_info(s_server, client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {
            has_ws = true;
            break;
        }
    }
    if (!has_ws) return;

    char json_buf[512];
    size_t len = telemetry_get_snapshot_json(json_buf, sizeof(json_buf));
    if (len == 0) return;

    for (size_t i = 0; i < num_fds; i++) {
        int fd = client_fds[i];
        if (httpd_ws_get_fd_info(s_server, fd) == HTTPD_WS_CLIENT_WEBSOCKET) {
            struct ws_async_send_arg *arg = malloc(sizeof(struct ws_async_send_arg));
            if (arg) {
                arg->fd = fd;
                strncpy(arg->payload, json_buf, sizeof(arg->payload) - 1);
                arg->payload[sizeof(arg->payload) - 1] = '\0';
                if (httpd_queue_work(s_server, ws_async_send, arg) != ESP_OK) {
                    free(arg);
                }
            }
        }
    }
}

// --------------------------------------------------------------------------
// CAPTIVE PORTAL REDIRECT HANDLERS (iOS / Android Auto-Popup)
// --------------------------------------------------------------------------
static esp_err_t captive_redirect_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_send(req, "Redirecting to Posture Monitor...", HTTPD_RESP_USE_STRLEN);
    ESP_LOGI(TAG, "Captive probe '%s' -> redirected to http://192.168.4.1/", req->uri);
    return ESP_OK;
}

static esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
    if (strstr(req->uri, "favicon") || strstr(req->uri, "apple-touch-icon")) {
        httpd_resp_set_status(req, "204 No Content");
        return httpd_resp_send(req, NULL, 0);
    }

    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_send(req, "Redirecting to Posture Monitor...", HTTPD_RESP_USE_STRLEN);
    ESP_LOGI(TAG, "Unknown URI '%s' -> 302 Redirect to http://192.168.4.1/", req->uri);
    return ESP_OK;
}

// --------------------------------------------------------------------------
// REST API HANDLERS (/api/...)
// --------------------------------------------------------------------------
static esp_err_t api_status_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "HTTP GET /api/status from fd=%d", httpd_req_to_sockfd(req));
    char buf[512];
    size_t len = telemetry_get_snapshot_json(buf, sizeof(buf));
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, buf, len);
}

static esp_err_t api_config_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "HTTP GET /api/config from fd=%d", httpd_req_to_sockfd(req));
    posture_calib_data_t calib;
    posture_core_get_calib(&calib);

    const esp_partition_t *running = esp_ota_get_running_partition();
    const char *slot_label = (running != NULL) ? running->label : "factory";
    const esp_app_desc_t *app_desc = esp_app_get_description();

    char resp[384];
    int written = snprintf(resp, sizeof(resp),
        "{\"threshold\":%.1f,\"slouch_delay_s\":%lu,\"escalation_delay_s\":%lu,"
        "\"pitch_offset\":%.2f,\"roll_offset\":%.2f,\"buzzer_enabled\":true,"
        "\"ota_slot\":\"%s\",\"version\":\"%s\",\"build_date\":\"%s\",\"build_time\":\"%s\",\"idf_ver\":\"%s\"}",
        calib.angle_threshold,
        (unsigned long)calib.slouch_delay_s,
        (unsigned long)calib.escalation_delay_s,
        calib.pitch_offset,
        calib.roll_offset,
        slot_label,
        app_desc->version,
        app_desc->date,
        app_desc->time,
        app_desc->idf_ver);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, resp, written);
}

static esp_err_t api_config_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "HTTP POST /api/config from fd=%d", httpd_req_to_sockfd(req));
    char content[256];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';

    cJSON *root = cJSON_Parse(content);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    posture_calib_data_t calib;
    posture_core_get_calib(&calib);

    cJSON *thresh = cJSON_GetObjectItem(root, "threshold");
    if (thresh && cJSON_IsNumber(thresh)) {
        calib.angle_threshold = (float)thresh->valuedouble;
    }

    cJSON *slouch = cJSON_GetObjectItem(root, "slouch_delay_s");
    if (slouch && cJSON_IsNumber(slouch)) {
        calib.slouch_delay_s = (uint32_t)slouch->valueint;
    }

    cJSON *escalation = cJSON_GetObjectItem(root, "escalation_delay_s");
    if (escalation && cJSON_IsNumber(escalation)) {
        calib.escalation_delay_s = (uint32_t)escalation->valueint;
    }

    cJSON_Delete(root);

    // Save to NVS Flash and active core
    storage_manager_save_calibration(&calib);
    posture_core_update_calib(&calib);

    ESP_LOGI(TAG, "Updated config via REST: Thresh=%.1f deg, Grace=%lu s, Escalation=%lu s",
             calib.angle_threshold, (unsigned long)calib.slouch_delay_s, (unsigned long)calib.escalation_delay_s);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_sendstr(req, "{\"status\":\"success\"}");
}

static esp_err_t api_calibrate_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "HTTP POST /api/calibrate from fd=%d", httpd_req_to_sockfd(req));
    actuator_manager_set_pattern(ALERT_PATTERN_CALIB_START);
    mpu6050_sensor_reset_yaw();
    posture_core_start_calibration();
    telemetry_record_event("calib", "Tare Calibrated", "Zero baseline initialized via Web UI");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_sendstr(req, "{\"status\":\"calibrating\"}");
}

static esp_err_t api_snooze_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "HTTP POST /api/snooze from fd=%d", httpd_req_to_sockfd(req));
    char content[128];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    uint32_t seconds = 600; // default 10m

    if (ret > 0) {
        content[ret] = '\0';
        cJSON *root = cJSON_Parse(content);
        if (root) {
            cJSON *sec_item = cJSON_GetObjectItem(root, "seconds");
            if (sec_item && cJSON_IsNumber(sec_item)) {
                seconds = (uint32_t)sec_item->valueint;
            }
            cJSON_Delete(root);
        }
    }

    ESP_LOGI(TAG, "Snooze requested for %lu seconds via REST", (unsigned long)seconds);
    posture_core_snooze(seconds);
    telemetry_record_event("good", "Alerts Snoozed", "User snoozed monitoring");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_sendstr(req, "{\"status\":\"snoozed\"}");
}

static esp_err_t api_history_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "HTTP GET /api/history from fd=%d", httpd_req_to_sockfd(req));
    char buf[2048];
    size_t len = telemetry_get_history_json(buf, sizeof(buf));
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, buf, len);
}

// --------------------------------------------------------------------------
// OTA FIRMWARE UPDATE HANDLER (/api/ota)
// --------------------------------------------------------------------------
static void ota_restart_timer_callback(void *arg) {
    ESP_LOGI(TAG, "OTA restart timer expired. Rebooting now...");
    esp_restart();
}

static void schedule_delayed_restart(uint32_t delay_ms) {
    const esp_timer_create_args_t timer_args = {
        .callback = &ota_restart_timer_callback,
        .name = "ota_reboot",
        .dispatch_method = ESP_TIMER_TASK,
    };
    esp_timer_handle_t timer;
    esp_err_t err = esp_timer_create(&timer_args, &timer);
    if (err == ESP_OK) {
        esp_timer_start_once(timer, (uint64_t)delay_ms * 1000ULL);
    } else {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        esp_restart();
    }
}

static esp_err_t api_ota_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "OTA upload initiated: total_len=%d bytes from fd=%d", req->content_len, httpd_req_to_sockfd(req));

    // 1. Safety Check: Low Battery Protection
    if (battery_monitor_is_low() || battery_monitor_get_percentage() < 20) {
        ESP_LOGE(TAG, "OTA rejected: Battery too low (%u%%)", (unsigned int)battery_monitor_get_percentage());
        httpd_resp_set_status(req, "403 Forbidden");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        return httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Battery is below 20%. Please connect USB charger before updating.\"}");
    }

    if (req->content_len <= 0) {
        ESP_LOGE(TAG, "OTA rejected: Content-Length is empty or zero");
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        return httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"No firmware file provided in upload.\"}");
    }

    // 2. Identify target OTA partition
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        ESP_LOGE(TAG, "Cannot find passive OTA partition! Check partition table.");
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        return httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"No passive OTA partition found on device.\"}");
    }

    ESP_LOGI(TAG, "Target OTA partition: '%s' at offset 0x%lx (size: 0x%lx)",
             update_partition->label, (unsigned long)update_partition->address, (unsigned long)update_partition->size);

    // 3. Pause Actuators and Signal Rapid Blink Status LED
    actuator_manager_set_pattern(ALERT_PATTERN_IDLE);
    status_led_set_mode(STATUS_LED_MODE_CALIBRATING); // 5 Hz rapid strobe indicates OTA in-progress

    esp_ota_handle_t ota_handle = 0;
    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        status_led_set_mode(STATUS_LED_MODE_HEARTBEAT);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        return httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Failed to begin OTA flash session.\"}");
    }

    // 4. Stream and Write Binary Chunks
    const size_t buf_size = 1024;
    char *ota_buf = malloc(buf_size);
    if (!ota_buf) {
        esp_ota_abort(ota_handle);
        status_led_set_mode(STATUS_LED_MODE_HEARTBEAT);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        return httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Failed to allocate buffer for OTA.\"}");
    }

    int remaining = req->content_len;
    bool is_first_chunk = true;

    while (remaining > 0) {
        int to_read = (remaining < (int)buf_size) ? remaining : (int)buf_size;
        int recv_len = httpd_req_recv(req, ota_buf, to_read);
        if (recv_len <= 0) {
            if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {
                continue; // Retry receive on timeout
            }
            ESP_LOGE(TAG, "OTA chunk receive error (recv_len=%d)", recv_len);
            free(ota_buf);
            esp_ota_abort(ota_handle);
            status_led_set_mode(STATUS_LED_MODE_HEARTBEAT);
            httpd_resp_set_status(req, "500 Internal Server Error");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            return httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Network connection disconnected during OTA.\"}");
        }

        // Validate Magic Byte 0xE9 on the very first byte of the binary image
        if (is_first_chunk) {
            is_first_chunk = false;
            if ((uint8_t)ota_buf[0] != 0xE9) {
                ESP_LOGE(TAG, "OTA image validation failed: byte 0 is 0x%02X (expected 0xE9)", (uint8_t)ota_buf[0]);
                free(ota_buf);
                esp_ota_abort(ota_handle);
                status_led_set_mode(STATUS_LED_MODE_HEARTBEAT);
                httpd_resp_set_status(req, "400 Bad Request");
                httpd_resp_set_type(req, "application/json");
                httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
                return httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Invalid firmware file. File must be a valid ESP32 .bin binary.\"}");
            }
        }

        err = esp_ota_write(ota_handle, ota_buf, recv_len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed at %d bytes remaining: %s", remaining, esp_err_to_name(err));
            free(ota_buf);
            esp_ota_abort(ota_handle);
            status_led_set_mode(STATUS_LED_MODE_HEARTBEAT);
            httpd_resp_set_status(req, "500 Internal Server Error");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            return httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Flash write error during OTA.\"}");
        }

        remaining -= recv_len;
    }

    free(ota_buf);

    // 5. Finalize and validate OTA checksum
    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end validation failed: %s", esp_err_to_name(err));
        status_led_set_mode(STATUS_LED_MODE_HEARTBEAT);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        return httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Firmware image validation/checksum failed.\"}");
    }

    // 6. Switch active boot partition to new slot
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        status_led_set_mode(STATUS_LED_MODE_HEARTBEAT);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        return httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"Failed to set new boot partition.\"}");
    }

    esp_app_desc_t new_app_desc;
    if (esp_ota_get_partition_description(update_partition, &new_app_desc) == ESP_OK) {
        ESP_LOGI(TAG, "OTA upgrade SUCCESSFUL! Target partition '%s' verified with App '%s' v%s (Built: %s %s | IDF %s). Rebooting in 1.5s...",
                 update_partition->label, new_app_desc.project_name, new_app_desc.version,
                 new_app_desc.date, new_app_desc.time, new_app_desc.idf_ver);
    } else {
        ESP_LOGI(TAG, "OTA upgrade SUCCESSFUL! Next boot will launch from partition '%s'. Rebooting in 1.5s...", update_partition->label);
    }
    status_led_set_mode(STATUS_LED_MODE_SOLID);

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, "{\"status\":\"ok\",\"message\":\"Firmware update successful! Device restarting...\"}");

    schedule_delayed_restart(1500);
    return ESP_OK;
}

// --------------------------------------------------------------------------
// HTTP SERVER INITIALIZATION
// --------------------------------------------------------------------------
esp_err_t web_server_start(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Configure max_uri_handlers to 25 to support all endpoints and captive portal
    config.max_uri_handlers = 25;

    // Critical: Configure buffer limits per Knowledge Item to prevent "Header fields are too long"
    config.max_req_hdr_len = 2048;
    config.max_uri_len = 1024;
    config.lru_purge_enable = true;
    config.stack_size = 8192; // Ensure sufficient stack for JSON parsing & WS async
    config.max_open_sockets = 7;
    config.recv_wait_timeout = 3;
    config.send_wait_timeout = 3;

    ESP_LOGI(TAG, "Starting HTTP & WebSocket server on port %d...", config.server_port);
    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(err));
        return err;
    }

    // Register Static Web Asset Handlers
    httpd_uri_t uri_root = { .uri = "/", .method = HTTP_GET, .handler = index_html_handler };
    httpd_register_uri_handler(s_server, &uri_root);

    httpd_uri_t uri_index = { .uri = "/index.html", .method = HTTP_GET, .handler = index_html_handler };
    httpd_register_uri_handler(s_server, &uri_index);

    httpd_uri_t uri_style = { .uri = "/style.css", .method = HTTP_GET, .handler = style_css_handler };
    httpd_register_uri_handler(s_server, &uri_style);

    httpd_uri_t uri_app = { .uri = "/app.js", .method = HTTP_GET, .handler = app_js_handler };
    httpd_register_uri_handler(s_server, &uri_app);

    httpd_uri_t uri_chart = { .uri = "/chart.js", .method = HTTP_GET, .handler = chart_js_handler };
    httpd_register_uri_handler(s_server, &uri_chart);

    httpd_uri_t uri_skeleton3d = { .uri = "/skeleton3d.js", .method = HTTP_GET, .handler = skeleton3d_js_handler };
    httpd_register_uri_handler(s_server, &uri_skeleton3d);

    // Register WebSocket Handler (/ws)
    httpd_uri_t uri_ws = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .is_websocket = true
    };
    httpd_register_uri_handler(s_server, &uri_ws);

    // Register REST API Handlers
    httpd_uri_t uri_api_status = { .uri = "/api/status", .method = HTTP_GET, .handler = api_status_handler };
    httpd_register_uri_handler(s_server, &uri_api_status);

    httpd_uri_t uri_api_config_get = { .uri = "/api/config", .method = HTTP_GET, .handler = api_config_get_handler };
    httpd_register_uri_handler(s_server, &uri_api_config_get);

    httpd_uri_t uri_api_config_post = { .uri = "/api/config", .method = HTTP_POST, .handler = api_config_post_handler };
    httpd_register_uri_handler(s_server, &uri_api_config_post);

    httpd_uri_t uri_api_calib = { .uri = "/api/calibrate", .method = HTTP_POST, .handler = api_calibrate_handler };
    httpd_register_uri_handler(s_server, &uri_api_calib);

    httpd_uri_t uri_api_snooze = { .uri = "/api/snooze", .method = HTTP_POST, .handler = api_snooze_handler };
    httpd_register_uri_handler(s_server, &uri_api_snooze);

    httpd_uri_t uri_api_hist = { .uri = "/api/history", .method = HTTP_GET, .handler = api_history_handler };
    httpd_register_uri_handler(s_server, &uri_api_hist);

    // Register OTA Firmware Update Handler (/api/ota)
    httpd_uri_t uri_api_ota = {
        .uri = "/api/ota",
        .method = HTTP_POST,
        .handler = api_ota_handler
    };
    httpd_register_uri_handler(s_server, &uri_api_ota);

    // Register Captive Portal Auto-Detect Handlers (iOS & Android)
    httpd_uri_t uri_apple_detect = { .uri = "/hotspot-detect.html", .method = HTTP_GET, .handler = captive_redirect_handler };
    httpd_register_uri_handler(s_server, &uri_apple_detect);

    httpd_uri_t uri_android_204 = { .uri = "/generate_204", .method = HTTP_GET, .handler = captive_redirect_handler };
    httpd_register_uri_handler(s_server, &uri_android_204);

    httpd_uri_t uri_android_gen = { .uri = "/gen_204", .method = HTTP_GET, .handler = captive_redirect_handler };
    httpd_register_uri_handler(s_server, &uri_android_gen);

    // Register 404 Error Handler to redirect all other unknown domains/paths to root
    httpd_register_err_handler(s_server, HTTPD_404_NOT_FOUND, http_404_error_handler);

    ESP_LOGI(TAG, "Web server running. Static files, /ws, /api/*, and Captive Portal ready.");
    return ESP_OK;
}

esp_err_t web_server_stop(void) {
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
    }
    return ESP_OK;
}
