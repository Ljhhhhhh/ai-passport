// main/xiaozhi_ui.c
#include "xiaozhi_ui.h"
#include "xiaozhi_config.h"
#include "bsp_display.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

LV_FONT_DECLARE(font_xiaozhi_16);
static const char *TAG = "xiaozhi_ui";

// Color Palette
#define COLOR_BG          0x0A0F1D  // Deep cosmic navy
#define COLOR_CARD_BG     0x151D2E  // Elevated card background
#define COLOR_PRIMARY     0x0066FF  // DeepSeek brand blue
#define COLOR_ACCENT      0x38BDF8  // Cyan accent
#define COLOR_TEXT_MAIN   0xF8FAFC  // Crisp white
#define COLOR_TEXT_MUTED  0x94A3B8  // Slate gray
#define COLOR_SUCCESS     0x10B981  // Emerald green
#define COLOR_WARNING     0xF59E0B  // Amber
#define COLOR_ERROR       0xEF4444  // Coral red

// UI Object Handles
static lv_obj_t *s_scr = NULL;
static lv_obj_t *s_label_wifi = NULL;
static lv_obj_t *s_label_model = NULL;
static lv_obj_t *s_label_battery = NULL;
static lv_obj_t *s_avatar_box = NULL;
static lv_obj_t *s_eye_left = NULL;
static lv_obj_t *s_eye_right = NULL;
static lv_obj_t *s_mouth_bar = NULL;
static lv_obj_t *s_wave_bars[5] = {NULL};
static lv_obj_t *s_label_status = NULL;
static lv_obj_t *s_card_dialog = NULL;
static lv_obj_t *s_label_prompt = NULL;
static lv_obj_t *s_label_response = NULL;
static lv_obj_t *s_label_footer = NULL;

// Internal Text Buffer for streaming
static char s_response_buf[2048];
static size_t s_response_len = 0;
static xiaozhi_state_t s_current_ui_state = XIAOZHI_STATE_BOOT;
static uint32_t s_anim_tick = 0;

void xiaozhi_ui_init(void)
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(1000))) {
        ESP_LOGE(TAG, "Failed to acquire LVGL lock in ui_init");
        return;
    }

    s_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(s_scr, LV_SCROLLBAR_MODE_OFF);

    // ==========================================
    // 1. Top Status Bar (y: 0 ~ 28)
    // ==========================================
    lv_obj_t *top_bar = lv_obj_create(s_scr);
    lv_obj_set_size(top_bar, 240, 28);
    lv_obj_set_pos(top_bar, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x0E1726), 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 4, 0);
    lv_obj_set_scrollbar_mode(top_bar, LV_SCROLLBAR_MODE_OFF);

    s_label_wifi = lv_label_create(top_bar);
    lv_label_set_text(s_label_wifi, "WiFi --");
    lv_obj_set_style_text_color(s_label_wifi, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(s_label_wifi, &lv_font_montserrat_14, 0);
    lv_obj_align(s_label_wifi, LV_ALIGN_LEFT_MID, 4, 0);

    s_label_model = lv_label_create(top_bar);
    lv_label_set_text(s_label_model, "DeepSeek");
    lv_obj_set_style_text_color(s_label_model, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_text_font(s_label_model, &lv_font_montserrat_14, 0);
    lv_obj_align(s_label_model, LV_ALIGN_CENTER, 0, 0);

    s_label_battery = lv_label_create(top_bar);
    lv_label_set_text(s_label_battery, "100%");
    lv_obj_set_style_text_color(s_label_battery, lv_color_hex(COLOR_SUCCESS), 0);
    lv_obj_set_style_text_font(s_label_battery, &lv_font_montserrat_14, 0);
    lv_obj_align(s_label_battery, LV_ALIGN_RIGHT_MID, -4, 0);

    // ==========================================
    // 2. Avatar / Face Box (y: 32 ~ 125)
    // ==========================================
    s_avatar_box = lv_obj_create(s_scr);
    lv_obj_set_size(s_avatar_box, 224, 88);
    lv_obj_set_pos(s_avatar_box, 8, 32);
    lv_obj_set_style_bg_color(s_avatar_box, lv_color_hex(COLOR_CARD_BG), 0);
    lv_obj_set_style_radius(s_avatar_box, 12, 0);
    lv_obj_set_style_border_color(s_avatar_box, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_width(s_avatar_box, 1, 0);
    lv_obj_set_scrollbar_mode(s_avatar_box, LV_SCROLLBAR_MODE_OFF);

    // Eyes
    s_eye_left = lv_obj_create(s_avatar_box);
    lv_obj_set_size(s_eye_left, 16, 22);
    lv_obj_set_pos(s_eye_left, 55, 12);
    lv_obj_set_style_bg_color(s_eye_left, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_radius(s_eye_left, 8, 0);
    lv_obj_set_style_border_width(s_eye_left, 0, 0);

    s_eye_right = lv_obj_create(s_avatar_box);
    lv_obj_set_size(s_eye_right, 16, 22);
    lv_obj_set_pos(s_eye_right, 135, 12);
    lv_obj_set_style_bg_color(s_eye_right, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_radius(s_eye_right, 8, 0);
    lv_obj_set_style_border_width(s_eye_right, 0, 0);

    // Mouth / Wave Indicator
    s_mouth_bar = lv_obj_create(s_avatar_box);
    lv_obj_set_size(s_mouth_bar, 36, 6);
    lv_obj_align(s_mouth_bar, LV_ALIGN_CENTER, 0, 14);
    lv_obj_set_style_bg_color(s_mouth_bar, lv_color_hex(COLOR_PRIMARY), 0);
    lv_obj_set_style_radius(s_mouth_bar, 3, 0);
    lv_obj_set_style_border_width(s_mouth_bar, 0, 0);

    // 5 soundwave animation bars
    for (int i = 0; i < 5; i++) {
        s_wave_bars[i] = lv_obj_create(s_avatar_box);
        lv_obj_set_size(s_wave_bars[i], 4, 12);
        lv_obj_set_pos(s_wave_bars[i], 82 + i * 9, 36);
        lv_obj_set_style_bg_color(s_wave_bars[i], lv_color_hex(COLOR_ACCENT), 0);
        lv_obj_set_style_radius(s_wave_bars[i], 2, 0);
        lv_obj_set_style_border_width(s_wave_bars[i], 0, 0);
        lv_obj_add_flag(s_wave_bars[i], LV_OBJ_FLAG_HIDDEN);
    }

    // Status message label below avatar
    s_label_status = lv_label_create(s_avatar_box);
    lv_label_set_text(s_label_status, "Ready");
    lv_obj_set_style_text_color(s_label_status, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(s_label_status, &lv_font_montserrat_14, 0);
    lv_obj_align(s_label_status, LV_ALIGN_BOTTOM_MID, 0, -2);

    // ==========================================
    // 3. Dialog & Streaming Area (y: 124 ~ 276)
    // ==========================================
    s_card_dialog = lv_obj_create(s_scr);
    lv_obj_set_size(s_card_dialog, 224, 150);
    lv_obj_set_pos(s_card_dialog, 8, 126);
    lv_obj_set_style_bg_color(s_card_dialog, lv_color_hex(COLOR_CARD_BG), 0);
    lv_obj_set_style_radius(s_card_dialog, 10, 0);
    lv_obj_set_style_border_color(s_card_dialog, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_width(s_card_dialog, 1, 0);
    lv_obj_set_style_pad_all(s_card_dialog, 8, 0);
    lv_obj_set_scrollbar_mode(s_card_dialog, LV_SCROLLBAR_MODE_AUTO);

    s_label_prompt = lv_label_create(s_card_dialog);
    lv_label_set_long_mode(s_label_prompt, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_label_prompt, 206);
    lv_label_set_text(s_label_prompt, "Q: Press OK to ask DeepSeek");
    lv_obj_set_style_text_color(s_label_prompt, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_text_font(s_label_prompt, &font_xiaozhi_16, 0);
    lv_obj_align(s_label_prompt, LV_ALIGN_TOP_LEFT, 0, 0);

    s_label_response = lv_label_create(s_card_dialog);
    lv_label_set_long_mode(s_label_response, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_label_response, 206);
    lv_label_set_text(s_label_response, "DeepSeek AI is ready. Press OK or UP/DOWN to start conversation.");
    lv_obj_set_style_text_color(s_label_response, lv_color_hex(COLOR_TEXT_MAIN), 0);
    lv_obj_set_style_text_font(s_label_response, &font_xiaozhi_16, 0);
    lv_obj_align(s_label_response, LV_ALIGN_TOP_LEFT, 0, 24);

    // ==========================================
    // 4. Bottom Footer Guidance (y: 282 ~ 318)
    // ==========================================
    s_label_footer = lv_label_create(s_scr);
    lv_label_set_text(s_label_footer, "OK: Talk | UP/DN: Topic");
    lv_obj_set_style_text_color(s_label_footer, lv_color_hex(COLOR_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(s_label_footer, &lv_font_montserrat_14, 0);
    lv_obj_align(s_label_footer, LV_ALIGN_BOTTOM_MID, 0, -6);

    lv_screen_load(s_scr);
    bsp_lvgl_unlock();
}

void xiaozhi_ui_set_state(xiaozhi_state_t state)
{
    s_current_ui_state = state;
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) return;

    if (s_label_status) {
        switch (state) {
            case XIAOZHI_STATE_BOOT:
                lv_label_set_text(s_label_status, "Booting...");
                lv_obj_set_style_text_color(s_label_status, lv_color_hex(COLOR_TEXT_MUTED), 0);
                break;
            case XIAOZHI_STATE_WIFI_CONNECTING:
                lv_label_set_text(s_label_status, "Connecting WiFi...");
                lv_obj_set_style_text_color(s_label_status, lv_color_hex(COLOR_WARNING), 0);
                break;
            case XIAOZHI_STATE_IDLE:
                lv_label_set_text(s_label_status, "Ready (IDLE)");
                lv_obj_set_style_text_color(s_label_status, lv_color_hex(COLOR_SUCCESS), 0);
                // Reset face
                lv_obj_clear_flag(s_mouth_bar, LV_OBJ_FLAG_HIDDEN);
                for (int i = 0; i < 5; i++) lv_obj_add_flag(s_wave_bars[i], LV_OBJ_FLAG_HIDDEN);
                break;
            case XIAOZHI_STATE_LISTENING:
                lv_label_set_text(s_label_status, "Listening...");
                lv_obj_set_style_text_color(s_label_status, lv_color_hex(COLOR_ACCENT), 0);
                lv_obj_add_flag(s_mouth_bar, LV_OBJ_FLAG_HIDDEN);
                for (int i = 0; i < 5; i++) lv_obj_clear_flag(s_wave_bars[i], LV_OBJ_FLAG_HIDDEN);
                break;
            case XIAOZHI_STATE_THINKING:
                lv_label_set_text(s_label_status, "DeepSeek Thinking...");
                lv_obj_set_style_text_color(s_label_status, lv_color_hex(COLOR_PRIMARY), 0);
                break;
            case XIAOZHI_STATE_SPEAKING:
                lv_label_set_text(s_label_status, "DeepSeek Replying...");
                lv_obj_set_style_text_color(s_label_status, lv_color_hex(COLOR_ACCENT), 0);
                lv_obj_add_flag(s_mouth_bar, LV_OBJ_FLAG_HIDDEN);
                for (int i = 0; i < 5; i++) lv_obj_clear_flag(s_wave_bars[i], LV_OBJ_FLAG_HIDDEN);
                break;
            case XIAOZHI_STATE_ERROR:
                lv_label_set_text(s_label_status, "Notice / Error");
                lv_obj_set_style_text_color(s_label_status, lv_color_hex(COLOR_ERROR), 0);
                break;
            default:
                break;
        }
    }

    bsp_lvgl_unlock();
}

void xiaozhi_ui_set_wifi_status(bool connected, const char *ip_str)
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) return;
    if (s_label_wifi) {
        if (connected) {
            lv_label_set_text(s_label_wifi, "WiFi OK");
            lv_obj_set_style_text_color(s_label_wifi, lv_color_hex(COLOR_SUCCESS), 0);
        } else {
            lv_label_set_text(s_label_wifi, "WiFi --");
            lv_obj_set_style_text_color(s_label_wifi, lv_color_hex(COLOR_ERROR), 0);
        }
    }
    (void)ip_str;
    bsp_lvgl_unlock();
}

void xiaozhi_ui_set_battery(uint8_t percent, bool charging)
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) return;
    if (s_label_battery) {
        lv_label_set_text_fmt(s_label_battery, "%s%d%%", charging ? "+" : "", percent);
        if (percent > 20) {
            lv_obj_set_style_text_color(s_label_battery, lv_color_hex(COLOR_SUCCESS), 0);
        } else {
            lv_obj_set_style_text_color(s_label_battery, lv_color_hex(COLOR_WARNING), 0);
        }
    }
    bsp_lvgl_unlock();
}

void xiaozhi_ui_set_prompt(const char *prompt)
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) return;
    if (s_label_prompt && prompt) {
        lv_label_set_text_fmt(s_label_prompt, "Q: %s", prompt);
    }
    bsp_lvgl_unlock();
}

void xiaozhi_ui_clear_response(void)
{
    s_response_len = 0;
    s_response_buf[0] = '\0';
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) return;
    if (s_label_response) {
        lv_label_set_text(s_label_response, "");
    }
    bsp_lvgl_unlock();
}

void xiaozhi_ui_append_response_token(const char *token, size_t len, bool is_reasoning)
{
    if (!token || len == 0) return;
    (void)is_reasoning;

    if (s_response_len + len + 1 < sizeof(s_response_buf)) {
        memcpy(&s_response_buf[s_response_len], token, len);
        s_response_len += len;
        s_response_buf[s_response_len] = '\0';
    }

    if (bsp_lvgl_lock(pdMS_TO_TICKS(500))) {
        if (s_label_response) {
            lv_label_set_text(s_label_response, s_response_buf);
            if (s_card_dialog) {
                lv_obj_scroll_to_view(s_label_response, LV_ANIM_OFF);
            }
        }
        bsp_lvgl_unlock();
    }
}

void xiaozhi_ui_set_error(const char *error_msg)
{
    if (!error_msg) return;
    if (bsp_lvgl_lock(pdMS_TO_TICKS(500))) {
        if (s_label_response) {
            lv_label_set_text_fmt(s_label_response, "[Error] %s\n\nPlease check API Key and Wi-Fi.", error_msg);
        }
        bsp_lvgl_unlock();
    }
}

void xiaozhi_ui_tick_anim(void)
{
    s_anim_tick++;
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(50))) return;

    if (s_current_ui_state == XIAOZHI_STATE_IDLE) {
        // Natural blinking every 60 ticks (approx 3 seconds)
        if ((s_anim_tick % 60) == 0) {
            lv_obj_set_height(s_eye_left, 4);
            lv_obj_set_height(s_eye_right, 4);
        } else if ((s_anim_tick % 60) == 3) {
            lv_obj_set_height(s_eye_left, 22);
            lv_obj_set_height(s_eye_right, 22);
        }
    } else if (s_current_ui_state == XIAOZHI_STATE_LISTENING || s_current_ui_state == XIAOZHI_STATE_SPEAKING) {
        // Animate wave bars
        static const int8_t wave_offsets[5] = {0, 2, 4, 1, 3};
        for (int i = 0; i < 5; i++) {
            if (s_wave_bars[i]) {
                int h = 8 + (((s_anim_tick + wave_offsets[i]) % 6) * 4);
                lv_obj_set_height(s_wave_bars[i], h);
            }
        }
    }

    bsp_lvgl_unlock();
}
