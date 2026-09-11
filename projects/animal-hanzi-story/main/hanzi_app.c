#include "hanzi_app.h"

#include "bsp_display.h"
#include "bsp_button.h"
#include "hanzi_audio.h"
#include "hanzi_store.h"
#include "hanzi_story.h"
#include "hanzi_ui.h"

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "lvgl.h"

typedef enum {
    APP_EVT_BUTTON = 0,
    APP_EVT_AUDIO,
    APP_EVT_TICK
} app_evt_type_t;

typedef struct {
    app_evt_type_t type;
    uint8_t a;
    uint8_t b;
} app_evt_t;

static const char *TAG = "hanzi_app";
static QueueHandle_t s_queue;
static hanzi_story_t s_story;
static uint8_t s_last_audio_kind;
static uint8_t s_audio_ok;

static void post(app_evt_type_t type, uint8_t a, uint8_t b)
{
    app_evt_t evt = {.type = type, .a = a, .b = b};
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

static void on_audio_done(int success, void *user)
{
    (void)user;
    post(APP_EVT_AUDIO, success ? 1 : 0, 0);
}

static void persist_if_needed(void)
{
    hanzi_save_t save;
    if (hanzi_story_consume_save(&s_story, &save)) {
        hanzi_store_save(&save);
    }
}

static void render_and_maybe_play(int play_audio)
{
    hanzi_view_t view;

    hanzi_story_get_view(&s_story, &view);
    if (!bsp_lvgl_lock(200)) {
        return;
    }
    hanzi_ui_render(&view);
    bsp_lvgl_unlock();

    persist_if_needed();

    if (!play_audio) {
        s_last_audio_kind = (uint8_t)view.audio_kind;
        return;
    }

    if (view.audio_kind != HANZI_AUDIO_NONE &&
        view.audio_kind != (hanzi_audio_kind_t)s_last_audio_kind) {
        if (!hanzi_audio_play_view(&view)) {
            hanzi_story_on_audio_done(&s_story, 0);
            hanzi_story_get_view(&s_story, &view);
            if (bsp_lvgl_lock(200)) {
                hanzi_ui_render(&view);
                bsp_lvgl_unlock();
            }
            ESP_LOGW(TAG, "audio degraded; screen flow continues");
        }
    } else if (view.audio_kind == HANZI_AUDIO_NONE) {
        hanzi_audio_cancel();
    }
    s_last_audio_kind = (uint8_t)view.audio_kind;
}

static void app_task(void *arg)
{
    app_evt_t evt;
    TickType_t last = xTaskGetTickCount();

    (void)arg;
    render_and_maybe_play(1);

    for (;;) {
        TickType_t now = xTaskGetTickCount();
        uint32_t elapsed = (uint32_t)((now - last) * portTICK_PERIOD_MS);
        last = now;
        if (elapsed > 0U) {
            hanzi_audio_kind_t before = s_story.audio_kind;
            hanzi_story_on_tick(&s_story, elapsed);
            if (s_story.audio_kind != before) {
                render_and_maybe_play(1);
            }
        }

        if (xQueueReceive(s_queue, &evt, pdMS_TO_TICKS(50)) != pdTRUE) {
            continue;
        }

        if (evt.type == APP_EVT_BUTTON) {
            hanzi_audio_cancel();
            hanzi_story_on_button(&s_story, (hanzi_btn_t)evt.a);
            render_and_maybe_play(1);
        } else if (evt.type == APP_EVT_AUDIO) {
            hanzi_story_on_audio_done(&s_story, evt.a);
            render_and_maybe_play(1);
        }
    }
}

void hanzi_app_start(void)
{
    hanzi_save_t save;

    s_queue = xQueueCreate(8, sizeof(app_evt_t));
    hanzi_store_init(&save);
    hanzi_story_init(&s_story, &save);
    s_audio_ok = (uint8_t)hanzi_audio_init(on_audio_done, NULL);

    if (bsp_button_init(on_button, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "button init failed");
        return;
    }

    if (xTaskCreate(app_task, "hanzi_app", 4096, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "app task creation failed");
        return;
    }

    ESP_LOGI(TAG,
             "animal hanzi story ready (audio=%u heap=%u min=%u largest=%u)",
             (unsigned)s_audio_ok,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}
