/**
 * @file wifi_manager.c
 * @brief Wi-Fi SoftAP controller implementation.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#include "wifi_manager.h"
#include "dns_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include <string.h>

static const char *TAG = "WIFI_MGR";
static dns_server_handle_t s_dns_handle = NULL;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Client joined SoftAP: MAC=" MACSTR " AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Client left SoftAP: MAC=" MACSTR " AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

esp_err_t wifi_manager_init_softap(void) {
    ESP_LOGI(TAG, "Initializing Wi-Fi SoftAP...");

    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    if (!ap_netif) {
        ESP_LOGE(TAG, "Failed to create default Wi-Fi AP netif");
        return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = POSTURE_WIFI_AP_SSID,
            .ssid_len = strlen(POSTURE_WIFI_AP_SSID),
            .channel = POSTURE_WIFI_AP_CHANNEL,
            .password = POSTURE_WIFI_AP_PASS,
            .max_connection = POSTURE_WIFI_AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    if (strlen(POSTURE_WIFI_AP_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Start captive portal DNS server resolving all queries to SoftAP
    dns_server_config_t dns_cfg = DNS_SERVER_CONFIG_SINGLE("*", "WIFI_AP_DEF");
    s_dns_handle = dns_server_start(&dns_cfg);
    if (s_dns_handle) {
        ESP_LOGI(TAG, "Captive portal DNS server active (redirecting all DNS queries to 192.168.4.1)");
    } else {
        ESP_LOGW(TAG, "Failed to start captive portal DNS server");
    }

    ESP_LOGI(TAG, "SoftAP active! SSID: '%s', Password: '%s'", POSTURE_WIFI_AP_SSID, POSTURE_WIFI_AP_PASS);
    ESP_LOGI(TAG, "Connect to Wi-Fi and navigate to http://192.168.4.1");
    return ESP_OK;
}

int8_t wifi_manager_get_rssi(void) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    return -50; // AP default
}
