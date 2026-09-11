// main/xiaozhi_config.h
// Configuration management for Xiaozhi DeepSeek built-in AI assistant.
#pragma once

#if __has_include("sdkconfig.h")
#include "sdkconfig.h"
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XIAOZHI_STR_MAX_LEN 256
#define XIAOZHI_PROMPT_MAX_LEN 512

#ifndef CONFIG_XIAOZHI_MODEL_PROVIDER
#define CONFIG_XIAOZHI_MODEL_PROVIDER "deepseek"
#endif

#ifndef CONFIG_XIAOZHI_DEEPSEEK_API_KEY
#define CONFIG_XIAOZHI_DEEPSEEK_API_KEY ""
#endif

#ifndef CONFIG_XIAOZHI_DEEPSEEK_BASE_URL
#define CONFIG_XIAOZHI_DEEPSEEK_BASE_URL "https://api.deepseek.com/v1"
#endif

#ifndef CONFIG_XIAOZHI_DEEPSEEK_MODEL
#define CONFIG_XIAOZHI_DEEPSEEK_MODEL "deepseek-chat"
#endif

#ifndef CONFIG_XIAOZHI_DEEPSEEK_SYSTEM_PROMPT
#define CONFIG_XIAOZHI_DEEPSEEK_SYSTEM_PROMPT "你是小智，一个运行在 FoloToy AI Passport 随身卡片机上的智能语音伴侣助手。你的回答需要简明、亲切、生动，适合小屏幕显示和语音交流。"
#endif

#ifndef CONFIG_XIAOZHI_DEEPSEEK_TEMPERATURE
#define CONFIG_XIAOZHI_DEEPSEEK_TEMPERATURE 7
#endif

#ifndef CONFIG_XIAOZHI_DEEPSEEK_MAX_TOKENS
#define CONFIG_XIAOZHI_DEEPSEEK_MAX_TOKENS 512
#endif

#ifndef CONFIG_XIAOZHI_WIFI_SSID
#define CONFIG_XIAOZHI_WIFI_SSID ""
#endif

#ifndef CONFIG_XIAOZHI_WIFI_PASSWORD
#define CONFIG_XIAOZHI_WIFI_PASSWORD ""
#endif

#ifndef CONFIG_XIAOZHI_AUDIO_DEFAULT_VOLUME
#define CONFIG_XIAOZHI_AUDIO_DEFAULT_VOLUME 75
#endif

#ifndef CONFIG_XIAOZHI_AUDIO_SAMPLE_RATE
#define CONFIG_XIAOZHI_AUDIO_SAMPLE_RATE 16000
#endif

typedef struct {
    char model_provider[32];
    char api_key[XIAOZHI_STR_MAX_LEN];
    char base_url[XIAOZHI_STR_MAX_LEN];
    char model[64];
    char system_prompt[XIAOZHI_PROMPT_MAX_LEN];
    float temperature;
    int max_tokens;
    char wifi_ssid[64];
    char wifi_password[64];
    uint8_t volume;
    uint32_t sample_rate;
} xiaozhi_config_t;

/**
 * @brief Initialize configuration system with compile-time defaults + NVS overrides.
 */
void xiaozhi_config_init(void);

/**
 * @brief Get read-only pointer to current active configuration.
 */
const xiaozhi_config_t *xiaozhi_config_get(void);

/**
 * @brief Check if a valid API key is present (built-in or from NVS).
 */
bool xiaozhi_config_has_valid_api_key(void);

/**
 * @brief Check if Wi-Fi SSID is configured.
 */
bool xiaozhi_config_has_wifi(void);

/**
 * @brief Save Wi-Fi credentials to NVS.
 */
bool xiaozhi_config_save_wifi(const char *ssid, const char *password);

/**
 * @brief Save API key to NVS.
 */
bool xiaozhi_config_save_api_key(const char *api_key);

/**
 * @brief Save volume to NVS.
 */
bool xiaozhi_config_save_volume(uint8_t volume);

#ifdef __cplusplus
}
#endif
