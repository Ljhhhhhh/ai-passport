#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "passport_protocol.h"
#include "passport_storage.h"
#include "passport_ble.h"
#include "passport_ui.h"
#include "passport_idle.h"
#include "passport_alert.h"
#include "passport_voice.h"

static const char *TAG = "codex-passport";
static passport_idle_t s_idle;
typedef struct {
    bsp_btn_t btn;
    bsp_btn_ev_t ev;
    uint32_t pressed_voice_rid;
} btn_msg_t;

static QueueHandle_t s_buttons;

static void apply_idle(passport_idle_act_t act)
{
    if (act == PASSPORT_IDLE_SLEEP) {
        bsp_display_backlight(0);
        ESP_LOGI(TAG, "Backlight off: no unread tasks, idle timeout");
    } else if (act == PASSPORT_IDLE_WAKE) {
        bsp_display_backlight(100);
        ESP_LOGI(TAG, "Backlight on");
    }
}

static void on_button_event(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    (void)user;
    static uint32_t pressed_voice_rid; /* Button callbacks run in one task. */
    if (btn == BSP_BTN_OK && ev == BSP_BTN_PRESS) pressed_voice_rid = passport_voice_press();
    if (btn == BSP_BTN_OK && ev == BSP_BTN_RELEASE) passport_voice_release();
    if ((ev == BSP_BTN_PRESS || ev == BSP_BTN_CLICK || ev == BSP_BTN_DOUBLE || ev == BSP_BTN_LONG) && s_buttons) {
        btn_msg_t msg = { .btn = btn, .ev = ev, .pressed_voice_rid = pressed_voice_rid };
        xQueueSend(s_buttons, &msg, 0);
    }
}

static void ui_input_task(void *arg)
{
    (void)arg;
    TickType_t previous = xTaskGetTickCount();
    bool suppress_gesture = false;
    bool voice_notice = false;
    while (1) {
        btn_msg_t msg;
        bool pressed = xQueueReceive(s_buttons, &msg, pdMS_TO_TICKS(250)) == pdTRUE;
        TickType_t now = xTaskGetTickCount();
        uint32_t dt = (now - previous) * portTICK_PERIOD_MS;
        uint32_t unread = passport_ble_unread_count();
        apply_idle(passport_idle_set_unread(&s_idle, unread));
        if (passport_voice_busy() || voice_notice) s_idle.idle_ms = 0;
        apply_idle(passport_idle_on_tick(&s_idle, dt));
        previous = now;
        if (passport_ui_take_project_update()) {
            bool was_asleep = !s_idle.awake;
            s_idle.awake = 1;
            s_idle.idle_ms = 0;
            if (was_asleep) {
                passport_ui_show_projects();
                apply_idle(PASSPORT_IDLE_WAKE);
            }
        }
        if (!pressed) continue;
        if (msg.ev == BSP_BTN_PRESS) {
            suppress_gesture = suppress_gesture || !s_idle.awake;
            apply_idle(passport_idle_on_button(&s_idle, msg.btn == BSP_BTN_OK));
            continue;
        }
        if (suppress_gesture) {
            suppress_gesture = false;
            continue;
        }
        passport_idle_act_t act = passport_idle_on_button(&s_idle, msg.btn == BSP_BTN_OK);
        apply_idle(act);
        if (act != PASSPORT_IDLE_PASS) continue;

        if (voice_notice) {
            if (msg.btn == BSP_BTN_OK && (msg.ev == BSP_BTN_CLICK || msg.ev == BSP_BTN_LONG)) {
                voice_notice = !passport_ui_voice_hide();
            }
            continue;
        }

        if (passport_voice_busy()) {
            if (msg.ev == BSP_BTN_LONG && msg.btn == BSP_BTN_OK) passport_voice_cancel();
            else if (msg.ev == BSP_BTN_CLICK && msg.btn == BSP_BTN_OK) {
                if (passport_voice_get_state() == PASSPORT_VOICE_RECORDING) {
                    passport_voice_stop();
                } else {
                    passport_voice_confirm(msg.pressed_voice_rid);
                }
            } else if (msg.ev == BSP_BTN_CLICK) passport_ui_voice_scroll(msg.btn == BSP_BTN_DOWN ? 1 : -1);
            continue;
        }

        if (msg.ev == BSP_BTN_DOUBLE && msg.btn == BSP_BTN_OK && passport_ui_is_messages_page()) {
            uint8_t thread_id[16];
            char title[64];
            if (!passport_ui_voice_target(thread_id, title) || !passport_voice_start(thread_id, title)) {
                voice_notice = passport_ui_voice_show("Voice unavailable",
                    "Keep the Mac connected. Wait for a highlighted message, then try again.", "OK: close");
            }
            continue;
        }

        if (msg.ev == BSP_BTN_LONG) {
            if (msg.btn == BSP_BTN_OK && !passport_ui_is_messages_page()) passport_ui_toggle_qr();
        } else if (msg.ev == BSP_BTN_CLICK) {
            if (msg.btn == BSP_BTN_UP) {
                passport_ui_prev_item();
            } else if (msg.btn == BSP_BTN_DOWN) {
                passport_ui_next_item();
            } else if (msg.btn == BSP_BTN_OK) {
                if (passport_ui_is_settings_page()) {
                    passport_ui_settings_toggle();
                } else if (passport_ui_is_messages_page()) {
                    /* Keep OK free for double-click talk; UP/DOWN already move cards. */
                } else {
                    passport_ui_toggle_qr();
                }
            }
        }
    }
}

static void battery_task(void *pvParameters)
{
    bool has_batt = (pvParameters != NULL);

    while (1) {
        if (has_batt) {
            int pct = bsp_battery_soc();
            int mv = bsp_battery_mv();
            bool is_chg = (mv > 4250);
            passport_ui_update_battery(pct >= 0 ? pct : 100, is_chg);
        } else {
            passport_ui_update_battery(100, false);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Codex Passport Companion...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(bsp_i2c_init());

    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "Display / LVGL initialization failed!");
        return;
    }
    bsp_display_backlight(100);
    passport_idle_init(&s_idle);

    bool has_batt = (bsp_battery_init() == ESP_OK);
    ESP_LOGI(TAG, "Battery sensor CW2017: %s", has_batt ? "detected" : "not fitted (using default)");

    ESP_ERROR_CHECK(passport_storage_init());
    ESP_ERROR_CHECK(passport_ui_init());
    ESP_ERROR_CHECK(passport_voice_init());
    s_buttons = xQueueCreate(8, sizeof(btn_msg_t));
    configASSERT(s_buttons);
    configASSERT(xTaskCreate(ui_input_task, "passport_input", 3072, NULL, 4, NULL) == pdPASS);
    ESP_ERROR_CHECK(bsp_button_init(on_button_event, NULL));
    ESP_ERROR_CHECK(passport_ble_init());

    xTaskCreate(battery_task, "battery_task", 3072, has_batt ? (void *)1 : NULL, 4, NULL);

    ESP_LOGI(TAG, "Codex Passport initialized successfully.");
}
