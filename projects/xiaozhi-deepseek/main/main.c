// projects/xiaozhi-deepseek/main/main.c
// Xiaozhi AI Assistant with Built-in DeepSeek for FoloToy AI Passport.
#include "xiaozhi_config.h"
#include "xiaozhi_state.h"
#include "xiaozhi_ui.h"
#include "xiaozhi_audio.h"
#include "xiaozhi_wifi.h"
#include "deepseek_client.h"
#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "xiaozhi_main";

static xiaozhi_state_machine_t s_sm;
static QueueHandle_t s_event_queue = NULL;

typedef struct {
    xiaozhi_event_t evt;
    char text[XIAOZHI_PROMPT_MAX_LEN];
} app_event_msg_t;

// DeepSeek Token Callbacks
static void on_deepseek_token(const char *token, size_t len, bool is_reasoning, void *user_data)
{
    (void)user_data;
    xiaozhi_ui_append_response_token(token, len, is_reasoning);
    xiaozhi_audio_play_speech_token(token, len);

    app_event_msg_t msg = {
        .evt = { .type = XIAOZHI_EVT_QUERY_TOKEN, .str_payload = NULL, .int_payload = 0 }
    };
    xQueueSend(s_event_queue, &msg, 0);
}

static void on_deepseek_complete(void *user_data)
{
    (void)user_data;
    app_event_msg_t msg = {
        .evt = { .type = XIAOZHI_EVT_QUERY_DONE, .str_payload = NULL, .int_payload = 0 }
    };
    xQueueSend(s_event_queue, &msg, portMAX_DELAY);
}

static void on_deepseek_error(const char *err_msg, void *user_data)
{
    (void)user_data;
    app_event_msg_t msg = {
        .evt = { .type = XIAOZHI_EVT_QUERY_ERROR, .str_payload = err_msg, .int_payload = 0 }
    };
    if (err_msg) {
        strncpy(msg.text, err_msg, sizeof(msg.text) - 1);
        msg.evt.str_payload = msg.text;
    }
    xQueueSend(s_event_queue, &msg, portMAX_DELAY);
}

// Background Task for running DeepSeek HTTPS Request
static void query_worker_task(void *pvParameters)
{
    char prompt[XIAOZHI_PROMPT_MAX_LEN];
    strncpy(prompt, (const char *)pvParameters, sizeof(prompt) - 1);
    prompt[sizeof(prompt) - 1] = '\0';

    ESP_LOGI(TAG, "Starting DeepSeek query: \"%s\"", prompt);
    xiaozhi_ui_clear_response();

    bool ok = deepseek_client_chat(prompt,
                                   on_deepseek_token,
                                   on_deepseek_complete,
                                   on_deepseek_error,
                                   NULL);
    if (!ok) {
        ESP_LOGW(TAG, "DeepSeek chat request ended with failure status");
    }

    vTaskDelete(NULL);
}

// State Machine Change Callback
static void on_state_changed(xiaozhi_state_t old_state, xiaozhi_state_t new_state, void *user_data)
{
    (void)user_data;
    ESP_LOGI(TAG, "State transition: %s -> %s", xiaozhi_state_to_str(old_state), xiaozhi_state_to_str(new_state));

    xiaozhi_ui_set_state(new_state);

    switch (new_state) {
        case XIAOZHI_STATE_LISTENING:
            xiaozhi_audio_play_chime(XIAOZHI_CHIME_WAKE);
            xiaozhi_audio_start_record();
            xiaozhi_ui_set_prompt("正在录音... (松开OK键发送)");
            break;

        case XIAOZHI_STATE_THINKING:
            {
                size_t rec_bytes = 0;
                uint32_t rec_dur = xiaozhi_audio_stop_record(&rec_bytes);
                xiaozhi_audio_play_chime(XIAOZHI_CHIME_THINK);

                const char *cur_prompt = xiaozhi_get_preset_prompt(s_sm.prompt_index);
                xiaozhi_ui_set_prompt(cur_prompt);
                xTaskCreate(query_worker_task, "query_task", 8192, (void *)cur_prompt, 5, NULL);
                (void)rec_dur;
            }
            break;

        case XIAOZHI_STATE_IDLE:
            if (old_state == XIAOZHI_STATE_SPEAKING || old_state == XIAOZHI_STATE_THINKING) {
                xiaozhi_audio_play_chime(XIAOZHI_CHIME_DONE);
            }
            break;

        case XIAOZHI_STATE_ERROR:
            xiaozhi_audio_play_chime(XIAOZHI_CHIME_ERROR);
            xiaozhi_ui_set_error(s_sm.last_error);
            break;

        default:
            break;
    }
}

// Wi-Fi Event Callback
static void on_wifi_status(bool connected, const char *ip_str, void *user_data)
{
    (void)user_data;
    xiaozhi_ui_set_wifi_status(connected, ip_str);

    app_event_msg_t msg = {
        .evt = {
            .type = connected ? XIAOZHI_EVT_WIFI_CONNECTED : XIAOZHI_EVT_WIFI_DISCONNECTED,
            .str_payload = ip_str,
            .int_payload = 0
        }
    };
    xQueueSend(s_event_queue, &msg, portMAX_DELAY);
}

// Button Callback (Non-blocking ISR/Timer context)
static void on_bsp_button(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    (void)user;
    app_event_msg_t msg = {0};
    if (ev == BSP_BTN_PRESS) {
        if (btn == BSP_BTN_OK) msg.evt.type = XIAOZHI_EVT_BTN_OK_PRESS;
    } else if (ev == BSP_BTN_RELEASE) {
        if (btn == BSP_BTN_OK) msg.evt.type = XIAOZHI_EVT_BTN_OK_RELEASE;
    } else if (ev == BSP_BTN_CLICK) {
        if (btn == BSP_BTN_UP) msg.evt.type = XIAOZHI_EVT_BTN_UP_CLICK;
        else if (btn == BSP_BTN_DOWN) msg.evt.type = XIAOZHI_EVT_BTN_DOWN_CLICK;
        else if (btn == BSP_BTN_OK) msg.evt.type = XIAOZHI_EVT_BTN_OK_CLICK;
    }
    if (msg.evt.type != XIAOZHI_EVT_NONE && s_event_queue) {
        xQueueSend(s_event_queue, &msg, 0);
    }
}

// Periodic UI / Battery animation task
static void ui_timer_task(void *pvParameters)
{
    (void)pvParameters;
    uint32_t count = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
        xiaozhi_ui_tick_anim();

        count++;
        if ((count % 100) == 0) { // Every 5 seconds
            int pct = bsp_battery_soc();
            if (pct >= 0) {
                xiaozhi_ui_set_battery((uint8_t)pct, false);
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "  Xiaozhi AI Assistant (Built-in DeepSeek)");
    ESP_LOGI(TAG, "  Target: FoloToy AI Passport (ESP32-C3)");
    ESP_LOGI(TAG, "==================================================");

    // 1. Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Initialize BSP Hardware
    ESP_ERROR_CHECK(bsp_i2c_init());
    ESP_ERROR_CHECK(bsp_display_init());
    bsp_lvgl_init();
    bsp_display_backlight(100);

    bsp_battery_init();
    xiaozhi_audio_init();
    // 3. Initialize Event Queue & Configurations
    s_event_queue = xQueueCreate(16, sizeof(app_event_msg_t));
    xiaozhi_config_init();
    deepseek_client_init();

    // 4. Initialize UI
    xiaozhi_ui_init();
    xiaozhi_ui_set_prompt(xiaozhi_get_preset_prompt(0));

    int batt = bsp_battery_soc();
    if (batt >= 0) {
        xiaozhi_ui_set_battery((uint8_t)batt, false);
    }

    // 5. Initialize State Machine
    const xiaozhi_config_t *cfg = xiaozhi_config_get();
    bool has_key = xiaozhi_config_has_valid_api_key();
    xiaozhi_sm_init(&s_sm, false, has_key, on_state_changed, NULL);
    xiaozhi_ui_set_state(s_sm.current_state);

    if (!has_key) {
        xiaozhi_ui_set_error("Built-in API Key is empty.\nPlease configure CONFIG_XIAOZHI_DEEPSEEK_API_KEY in sdkconfig or menuconfig.");
    }

    // 6. Initialize Buttons
    ESP_ERROR_CHECK(bsp_button_init(on_bsp_button, NULL));

    // 7. Initialize and Connect Wi-Fi
    xiaozhi_wifi_init(on_wifi_status, NULL);
    if (xiaozhi_config_has_wifi()) {
        xiaozhi_wifi_connect(cfg->wifi_ssid, cfg->wifi_password);
    } else {
        ESP_LOGW(TAG, "Default Wi-Fi SSID is not configured. Please set CONFIG_XIAOZHI_WIFI_SSID in sdkconfig.");
    }

    // 8. Start Background UI Task
    xTaskCreate(ui_timer_task, "ui_timer", 3072, NULL, 3, NULL);

    // 9. Main Event Loop
    app_event_msg_t msg;
    while (1) {
        if (xQueueReceive(s_event_queue, &msg, portMAX_DELAY)) {
            if (msg.evt.type == XIAOZHI_EVT_BTN_UP_CLICK || msg.evt.type == XIAOZHI_EVT_BTN_DOWN_CLICK) {
                xiaozhi_audio_play_chime(XIAOZHI_CHIME_CLICK);
            }

            xiaozhi_sm_step(&s_sm, &msg.evt);

            if (msg.evt.type == XIAOZHI_EVT_BTN_UP_CLICK || msg.evt.type == XIAOZHI_EVT_BTN_DOWN_CLICK) {
                const char *p = xiaozhi_get_preset_prompt(s_sm.prompt_index);
                xiaozhi_ui_set_prompt(p);
            }
        }
    }
}
