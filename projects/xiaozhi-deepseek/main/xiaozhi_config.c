// main/xiaozhi_config.c
#include "xiaozhi_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "xiaozhi_config";
static xiaozhi_config_t s_config;

void xiaozhi_config_init(void)
{
    // 1. Populate compile-time defaults
    memset(&s_config, 0, sizeof(s_config));
    snprintf(s_config.model_provider, sizeof(s_config.model_provider), "%s", CONFIG_XIAOZHI_MODEL_PROVIDER);
    snprintf(s_config.api_key, sizeof(s_config.api_key), "%s", CONFIG_XIAOZHI_DEEPSEEK_API_KEY);
    snprintf(s_config.base_url, sizeof(s_config.base_url), "%s", CONFIG_XIAOZHI_DEEPSEEK_BASE_URL);
    snprintf(s_config.model, sizeof(s_config.model), "%s", CONFIG_XIAOZHI_DEEPSEEK_MODEL);
    snprintf(s_config.system_prompt, sizeof(s_config.system_prompt), "%s", CONFIG_XIAOZHI_DEEPSEEK_SYSTEM_PROMPT);
    s_config.temperature = (float)CONFIG_XIAOZHI_DEEPSEEK_TEMPERATURE / 10.0f;
    s_config.max_tokens = CONFIG_XIAOZHI_DEEPSEEK_MAX_TOKENS;
    snprintf(s_config.wifi_ssid, sizeof(s_config.wifi_ssid), "%s", CONFIG_XIAOZHI_WIFI_SSID);
    snprintf(s_config.wifi_password, sizeof(s_config.wifi_password), "%s", CONFIG_XIAOZHI_WIFI_PASSWORD);
    s_config.volume = CONFIG_XIAOZHI_AUDIO_DEFAULT_VOLUME;
    s_config.sample_rate = CONFIG_XIAOZHI_AUDIO_SAMPLE_RATE;

    // 2. Attempt to load NVS overrides
    nvs_handle_t nvs_h;
    esp_err_t err = nvs_open("xiaozhi", NVS_READONLY, &nvs_h);
    if (err == ESP_OK) {
        size_t len = sizeof(s_config.api_key);
        char nvs_api_key[XIAOZHI_STR_MAX_LEN] = {0};
        if (nvs_get_str(nvs_h, "api_key", nvs_api_key, &len) == ESP_OK && strlen(nvs_api_key) > 0) {
            snprintf(s_config.api_key, sizeof(s_config.api_key), "%s", nvs_api_key);
            ESP_LOGI(TAG, "Loaded API key from NVS override");
        }

        len = sizeof(s_config.wifi_ssid);
        char nvs_ssid[64] = {0};
        if (nvs_get_str(nvs_h, "wifi_ssid", nvs_ssid, &len) == ESP_OK && strlen(nvs_ssid) > 0) {
            snprintf(s_config.wifi_ssid, sizeof(s_config.wifi_ssid), "%s", nvs_ssid);
        }

        len = sizeof(s_config.wifi_password);
        char nvs_pass[64] = {0};
        if (nvs_get_str(nvs_h, "wifi_pass", nvs_pass, &len) == ESP_OK) {
            snprintf(s_config.wifi_password, sizeof(s_config.wifi_password), "%s", nvs_pass);
        }

        uint8_t vol = 0;
        if (nvs_get_u8(nvs_h, "volume", &vol) == ESP_OK) {
            s_config.volume = vol;
        }

        nvs_close(nvs_h);
    }

    ESP_LOGI(TAG, "Xiaozhi Config ready: Provider=%s, Model=%s, HasKey=%s, SSID=%s",
             s_config.model_provider,
             s_config.model,
             strlen(s_config.api_key) > 0 ? "YES" : "NO (configure in sdkconfig)",
             strlen(s_config.wifi_ssid) > 0 ? s_config.wifi_ssid : "(none)");
}

const xiaozhi_config_t *xiaozhi_config_get(void)
{
    return &s_config;
}

bool xiaozhi_config_has_valid_api_key(void)
{
    return strlen(s_config.api_key) >= 5;
}

bool xiaozhi_config_has_wifi(void)
{
    return strlen(s_config.wifi_ssid) > 0;
}

bool xiaozhi_config_save_wifi(const char *ssid, const char *password)
{
    if (!ssid) return false;
    nvs_handle_t nvs_h;
    if (nvs_open("xiaozhi", NVS_READWRITE, &nvs_h) != ESP_OK) return false;

    nvs_set_str(nvs_h, "wifi_ssid", ssid);
    if (password) {
        nvs_set_str(nvs_h, "wifi_pass", password);
    } else {
        nvs_set_str(nvs_h, "wifi_pass", "");
    }
    nvs_commit(nvs_h);
    nvs_close(nvs_h);

    snprintf(s_config.wifi_ssid, sizeof(s_config.wifi_ssid), "%s", ssid);
    if (password) {
        snprintf(s_config.wifi_password, sizeof(s_config.wifi_password), "%s", password);
    } else {
        s_config.wifi_password[0] = '\0';
    }
    return true;
}

bool xiaozhi_config_save_api_key(const char *api_key)
{
    if (!api_key) return false;
    nvs_handle_t nvs_h;
    if (nvs_open("xiaozhi", NVS_READWRITE, &nvs_h) != ESP_OK) return false;

    nvs_set_str(nvs_h, "api_key", api_key);
    nvs_commit(nvs_h);
    nvs_close(nvs_h);

    snprintf(s_config.api_key, sizeof(s_config.api_key), "%s", api_key);
    return true;
}

bool xiaozhi_config_save_volume(uint8_t volume)
{
    if (volume > 100) volume = 100;
    nvs_handle_t nvs_h;
    if (nvs_open("xiaozhi", NVS_READWRITE, &nvs_h) != ESP_OK) return false;

    nvs_set_u8(nvs_h, "volume", volume);
    nvs_commit(nvs_h);
    nvs_close(nvs_h);

    s_config.volume = volume;
    return true;
}
