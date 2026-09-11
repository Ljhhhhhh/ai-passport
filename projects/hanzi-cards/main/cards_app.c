#include "cards_app.h"

#include "bsp_button.h"
#include "bsp_display.h"
#include "cards_audio.h"
#include "cards_catalog.h"
#include "cards_model.h"
#include "cards_ui.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

typedef enum {
    APP_EVT_BUTTON = 0,
    APP_EVT_AUDIO
} app_evt_type_t;

typedef struct {
    app_evt_type_t type;
    uint8_t a;
    uint32_t gen;
} app_evt_t;

static const char *TAG = "cards_app";
static QueueHandle_t s_queue;
static cards_model_t s_model;
static uint8_t s_audio_ok;

static void post(app_evt_type_t type, uint8_t a, uint32_t gen)
{
    app_evt_t evt = {.type = type, .a = a, .gen = gen};
    if (s_queue) {
        xQueueSend(s_queue, &evt, 0);
    }
}

static void on_button(bsp_btn_t button, bsp_btn_ev_t event, void *user)
{
    (void)user;
    if (event != BSP_BTN_PRESS) {
        return;
    }
    post(APP_EVT_BUTTON, (uint8_t)button, 0);
}

static void on_audio_done(int success, uint32_t play_gen, void *user)
{
    (void)success;
    (void)user;
    post(APP_EVT_AUDIO, 0, play_gen);
}

static void apply_result(cards_result_t res)
{
    const cards_entry_t *entry = NULL;

    if (res.act == CARDS_ACT_NONE) {
        return;
    }

    if (res.act == CARDS_ACT_SLEEP) {
        cards_audio_cancel();
        bsp_display_backlight(0);
        return;
    }

    cards_catalog_get(res.index, &entry);
    bsp_display_backlight(100);
    if (bsp_lvgl_lock(200)) {
        cards_ui_render(entry);
        bsp_lvgl_unlock();
    }

    cards_audio_cancel();
    if (entry && entry->voice && entry->voice_len > 0U) {
        if (!cards_audio_play(entry->voice, entry->voice_len, res.play_gen)) {
            ESP_LOGW(TAG, "voice play failed; card remains browsable");
        }
    }
}

static void app_task(void *arg)
{
    app_evt_t evt;
    TickType_t last = xTaskGetTickCount();

    (void)arg;
    apply_result((cards_result_t){
        .act = CARDS_ACT_SHOW_PLAY,
        .index = s_model.index,
        .play_gen = s_model.play_gen
    });

    for (;;) {
        TickType_t now = xTaskGetTickCount();
        uint32_t elapsed = (uint32_t)((now - last) * portTICK_PERIOD_MS);
        last = now;
        if (elapsed > 0U) {
            apply_result(cards_model_on_tick(&s_model, elapsed));
        }

        if (xQueueReceive(s_queue, &evt, pdMS_TO_TICKS(50)) != pdTRUE) {
            continue;
        }

        if (evt.type == APP_EVT_BUTTON) {
            apply_result(cards_model_on_button(&s_model, (cards_btn_t)evt.a));
        } else if (evt.type == APP_EVT_AUDIO) {
            if (!cards_model_play_current(&s_model, evt.gen)) {
                ESP_LOGD(TAG, "ignore stale play-complete gen=%u", (unsigned)evt.gen);
            }
        }
    }
}

void cards_app_start(void)
{
    s_queue = xQueueCreate(8, sizeof(app_evt_t));
    cards_model_init(&s_model);
    s_audio_ok = (uint8_t)cards_audio_init(on_audio_done, NULL);

    if (bsp_button_init(on_button, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "button init failed");
        return;
    }

    if (xTaskCreate(app_task, "cards_app", 4096, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "app task creation failed");
        return;
    }

    ESP_LOGI(TAG,
             "hanzi cards ready (audio=%u heap=%u min=%u largest=%u)",
             (unsigned)s_audio_ok,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}
