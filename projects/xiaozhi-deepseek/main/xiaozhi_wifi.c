// main/xiaozhi_wifi.c
#include "xiaozhi_wifi.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "xiaozhi_wifi";
static bool s_connected = false;
static char s_ip_str[32] = "0.0.0.0";
static xiaozhi_wifi_event_cb_t s_cb = NULL;
static void *s_user_data = NULL;
static esp_netif_t *s_sta_netif = NULL;
static bool s_inited = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    (void)arg;
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            ESP_LOGI(TAG, "Wi-Fi STA started, connecting...");
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGW(TAG, "Wi-Fi disconnected, retrying connection...");
            s_connected = false;
            snprintf(s_ip_str, sizeof(s_ip_str), "0.0.0.0");
            if (s_cb) s_cb(false, s_ip_str, s_user_data);
            esp_wifi_connect();
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            esp_ip4addr_ntoa(&event->ip_info.ip, s_ip_str, sizeof(s_ip_str));
            ESP_LOGI(TAG, "Wi-Fi connected! Got IP: %s", s_ip_str);
            s_connected = true;
            if (s_cb) s_cb(true, s_ip_str, s_user_data);
        }
    }
}

bool xiaozhi_wifi_init(xiaozhi_wifi_event_cb_t cb, void *user_data)
{
    if (s_inited) return true;
    s_cb = cb;
    s_user_data = user_data;

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_netif_init failed");
        return false;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed");
        return false;
    }

    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (!s_sta_netif) {
        ESP_LOGE(TAG, "Failed to create default wifi STA netif");
        return false;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed");
        return false;
    }

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) return false;

    err = esp_wifi_start();
    if (err != ESP_OK) return false;

    s_inited = true;
    ESP_LOGI(TAG, "Wi-Fi subsystem initialized");
    return true;
}

bool xiaozhi_wifi_connect(const char *ssid, const char *password)
{
    if (!ssid || strlen(ssid) == 0) {
        ESP_LOGW(TAG, "Cannot connect: SSID is empty");
        return false;
    }

    wifi_config_t wifi_cfg = {0};
    strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    if (password) {
        strncpy((char *)wifi_cfg.sta.password, password, sizeof(wifi_cfg.sta.password) - 1);
    }
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    esp_wifi_disconnect();
    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);
    return (esp_wifi_connect() == ESP_OK);
}

bool xiaozhi_wifi_is_connected(void)
{
    return s_connected;
}

const char *xiaozhi_wifi_get_ip(void)
{
    return s_ip_str;
}
