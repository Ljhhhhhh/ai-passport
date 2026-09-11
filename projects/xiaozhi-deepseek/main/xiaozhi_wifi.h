// main/xiaozhi_wifi.h
// Wi-Fi station management for Xiaozhi DeepSeek AI Assistant.
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*xiaozhi_wifi_event_cb_t)(bool connected, const char *ip_str, void *user_data);

/**
 * @brief Initialize Wi-Fi subsystem and register event callbacks.
 */
bool xiaozhi_wifi_init(xiaozhi_wifi_event_cb_t cb, void *user_data);

/**
 * @brief Connect to Wi-Fi access point with given SSID and password.
 */
bool xiaozhi_wifi_connect(const char *ssid, const char *password);

/**
 * @brief Check if Wi-Fi is currently connected and has an IP.
 */
bool xiaozhi_wifi_is_connected(void);

/**
 * @brief Get current IP address string.
 */
const char *xiaozhi_wifi_get_ip(void);

#ifdef __cplusplus
}
#endif
