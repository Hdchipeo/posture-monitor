/**
 * @file dns_server.h
 * @brief Captive portal DNS server header for ESP32 SoftAP.
 *
 * Intercepts DNS queries on UDP port 53 and redirects domain lookups to the SoftAP IP,
 * enabling automatic captive portal detection and seamless mobile browser navigation.
 *
 * @copyright (c) 2026 Posture Monitor Project. Licensed under Apache-2.0.
 */

#pragma once

#include "esp_netif.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DNS_SERVER_MAX_ITEMS
#define DNS_SERVER_MAX_ITEMS 1
#endif

#define DNS_SERVER_CONFIG_SINGLE(queried_name, netif_key) {          \
        .num_of_entries = 1,                                         \
        .item = { { .name = queried_name, .if_key = netif_key } }    \
    }

/**
 * @brief Definition of one DNS entry: NAME - IP (or the netif whose IP to answer).
 */
typedef struct dns_entry_pair {
    const char *name;       /**< Exact match of the name field of the DNS query or "*" for wildcard */
    const char *if_key;     /**< Use this network interface IP to answer, if NULL use static IP below */
    esp_ip4_addr_t ip;      /**< Constant IP address if if_key is NULL */
} dns_entry_pair_t;

/**
 * @brief DNS server configuration structure.
 */
typedef struct dns_server_config {
    int num_of_entries;
    dns_entry_pair_t item[DNS_SERVER_MAX_ITEMS];
} dns_server_config_t;

typedef struct dns_server_handle *dns_server_handle_t;

/**
 * @brief Start the captive portal DNS server task.
 *
 * @param config Pointer to dns_server_config_t.
 * @return Handle to the DNS server, or NULL on failure.
 */
dns_server_handle_t dns_server_start(const dns_server_config_t *config);

/**
 * @brief Stop and free the captive portal DNS server.
 *
 * @param handle Handle returned from dns_server_start.
 */
void dns_server_stop(dns_server_handle_t handle);

#ifdef __cplusplus
}
#endif
