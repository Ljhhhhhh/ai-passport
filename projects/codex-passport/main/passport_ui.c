// passport_ui.c - Five-page LVGL UI for Codex Passport (240x320)
#include "passport_ui.h"
#include "passport_storage.h"
#include "passport_alert.h"
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>

#ifdef ESP_PLATFORM
#include "bsp_display.h"
#include "lvgl.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#if LV_USE_QRCODE
#include "src/libs/qrcode/lv_qrcode.h"
#endif

extern const lv_font_t font_passport_16;

static const char *TAG = "passport_ui";

#define COL_INK        0x0B1018
#define COL_GOLD       0xD4B45A
#define COL_GOLD_DEEP  0xA68532
#define COL_BURGUNDY   0x8E2433
#define COL_IVORY      0xF4EFE4
#define COL_IVORY_DIM  0xC4BDB0
#define COL_MUTED      0x8A93A6
#define COL_HAIR       0x2C3850
#define COL_STATUS     0x0E1420
#define COL_TODAY_BG   0x100E0A
#define COL_WAIT       0xE8A54B
#define COL_DONE       0x3D9B8F
#define COL_ERR        0xD4524A
#define COL_IDLE       0x6B7385

static lv_obj_t *s_pages[PAGE_COUNT];
static lv_obj_t *s_pager = NULL;
static lv_obj_t *s_ticks[PAGE_CYCLE];
static lv_obj_t *s_live = NULL;
static lv_obj_t *s_lbl_live_proj = NULL;
static lv_obj_t *s_lbl_live_meta = NULL;
static lv_obj_t *s_lbl_ble = NULL;
static lv_obj_t *s_stamp = NULL;
static lv_obj_t *s_lbl_status = NULL;
static lv_obj_t *s_lbl_battery = NULL;
static lv_obj_t *s_batt_level = NULL;

static lv_obj_t *s_lbl_name = NULL;
static lv_obj_t *s_lbl_issue = NULL;
static lv_obj_t *s_lbl_today_num = NULL;
static lv_obj_t *s_lbl_today_unit = NULL;
static lv_obj_t *s_lbl_sync = NULL;

static lv_obj_t *s_lbl_m_total = NULL;
static lv_obj_t *s_lbl_m_week = NULL;
static lv_obj_t *s_lbl_m_streak = NULL;

static lv_obj_t *s_lbl_q_window = NULL;
static lv_obj_t *s_lbl_q_name[PASSPORT_QUOTA_ACCOUNTS];
static lv_obj_t *s_lbl_q_short[PASSPORT_QUOTA_ACCOUNTS];
static lv_obj_t *s_lbl_q_week[PASSPORT_QUOTA_ACCOUNTS];
static lv_obj_t *s_bar_q_short[PASSPORT_QUOTA_ACCOUNTS];
static lv_obj_t *s_bar_q_week[PASSPORT_QUOTA_ACCOUNTS];
static lv_obj_t *s_lbl_q_reset[PASSPORT_QUOTA_ACCOUNTS];
static lv_obj_t *s_lbl_q_reset_w[PASSPORT_QUOTA_ACCOUNTS];

static lv_obj_t *s_qr_widget = NULL;
static lv_obj_t *s_lbl_qr_url = NULL;

static lv_obj_t *s_box_cfg_voice = NULL;
static lv_obj_t *s_lbl_cfg_voice_val = NULL;
static lv_obj_t *s_box_cfg_vol = NULL;
static lv_obj_t *s_lbl_cfg_vol_val = NULL;
static lv_obj_t *s_bar_cfg_vol = NULL;

static atomic_bool s_project_updated;
static uint8_t s_project_page;
static uint8_t s_project_pages = 1;
static int s_current_page = PAGE_PROFILE;
static int s_prev_page_before_qr = PAGE_PROFILE;
static bool s_qr_active = false;
static passport_voice_messages_page_t s_voice_messages;
static bool s_voice_targets_valid;
static uint8_t s_selected_message;
static bool s_prefer_last_message;
static bool s_message_page_pending;
static bool s_ble_connected;
static uint8_t s_message_count;
static uint8_t s_rendered_message_page;
static lv_obj_t *s_voice_overlay, *s_voice_phase, *s_voice_body, *s_voice_scroll, *s_voice_hint;
static void highlight_message(void);

static void strip_obj(lv_obj_t *obj)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
}

static void format_scaled(uint64_t tokens, char *num, size_t nlen, const char **scale)
{
    if (tokens >= 100000000ULL) {
        double val = (double)tokens / 100000000.0;
        snprintf(num, nlen, "%.1f", val);
        char *dot = strchr(num, '.');
        if (dot && strcmp(dot, ".0") == 0) *dot = '\0';
        *scale = "亿";
    } else if (tokens >= 10000ULL) {
        double val = (double)tokens / 10000.0;
        snprintf(num, nlen, "%.1f", val);
        char *dot = strchr(num, '.');
        if (dot && strcmp(dot, ".0") == 0) *dot = '\0';
        *scale = "万";
    } else {
        snprintf(num, nlen, "%llu", (unsigned long long)tokens);
        *scale = "";
    }
}

static void format_issue_short(const char *iso, char *out, size_t n)
{
    static const char *MON[] = {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };
    int y = 0, m = 0, d = 0;
    if (iso && sscanf(iso, "%d-%d-%d", &y, &m, &d) == 3 && m >= 1 && m <= 12) {
        snprintf(out, n, "%02d %s %02d", d, MON[m - 1], y % 100);
    } else {
        snprintf(out, n, "%s", iso ? iso : "");
    }
}

static void style_kicker(lv_obj_t *lbl)
{
    lv_obj_set_style_text_color(lbl, lv_color_hex(COL_GOLD), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_letter_space(lbl, 2, 0);
}

static void style_micro(lv_obj_t *lbl)
{
    lv_obj_set_style_text_color(lbl, lv_color_hex(COL_MUTED), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
}

static void show_page(int page_idx)
{
    if (page_idx < 0 || page_idx >= PAGE_COUNT) {
        return;
    }
    for (int i = 0; i < PAGE_COUNT; i++) {
        if (!s_pages[i]) {
            continue;
        }
        if (i == page_idx) {
            lv_obj_remove_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    bool qr = (page_idx == PAGE_QR_CODE);
    if (s_pager) {
        if (qr) {
            lv_obj_add_flag(s_pager, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(s_pager, LV_OBJ_FLAG_HIDDEN);
        }
    }
    for (int i = 0; i < PAGE_CYCLE; i++) {
        if (!s_ticks[i]) {
            continue;
        }
        bool on = (!qr && i == page_idx);
        lv_obj_set_style_bg_color(s_ticks[i], lv_color_hex(on ? COL_GOLD : 0x2A3346), 0);
        lv_obj_set_width(s_ticks[i], on ? 16 : 10);
    }
    s_current_page = page_idx;
}

static void create_page_home(lv_obj_t *parent)
{
    lv_obj_t *kicker = lv_label_create(parent);
    lv_label_set_text(kicker, "CODEX");
    style_kicker(kicker);
    lv_obj_align(kicker, LV_ALIGN_TOP_LEFT, 12, 12);

    s_lbl_sync = lv_label_create(parent);
    lv_label_set_text(s_lbl_sync, "SYNC  --");
    style_micro(s_lbl_sync);
    lv_obj_align(s_lbl_sync, LV_ALIGN_TOP_RIGHT, -12, 12);


    s_lbl_name = lv_label_create(parent);
    lv_label_set_text(s_lbl_name, "GuanMo");
    lv_obj_set_style_text_color(s_lbl_name, lv_color_hex(COL_IVORY), 0);
    lv_obj_set_style_text_font(s_lbl_name, &font_passport_16, 0);
    lv_obj_align(s_lbl_name, LV_ALIGN_TOP_LEFT, 12, 32);


    lv_obj_t *today = lv_obj_create(parent);
    strip_obj(today);
    lv_obj_set_size(today, 234, 86);
    lv_obj_align(today, LV_ALIGN_TOP_MID, 0, 72);
    lv_obj_set_style_bg_color(today, lv_color_hex(COL_TODAY_BG), 0);
    lv_obj_set_style_border_width(today, 1, 0);
    lv_obj_set_style_border_side(today, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(today, lv_color_hex(COL_GOLD_DEEP), 0);

    lv_obj_t *today_k = lv_label_create(today);
    lv_label_set_text(today_k, "TODAY");
    style_kicker(today_k);
    lv_obj_align(today_k, LV_ALIGN_TOP_LEFT, 12, 8);

    s_lbl_today_num = lv_label_create(today);
    lv_label_set_text(s_lbl_today_num, "0");
    lv_obj_set_style_text_color(s_lbl_today_num, lv_color_hex(COL_IVORY), 0);
    lv_obj_set_style_text_font(s_lbl_today_num, &lv_font_montserrat_28, 0);
    lv_obj_align(s_lbl_today_num, LV_ALIGN_TOP_LEFT, 12, 28);

    s_lbl_today_unit = lv_label_create(today);
    lv_label_set_text(s_lbl_today_unit, "万 Token");
    style_kicker(s_lbl_today_unit);
    lv_obj_set_style_text_font(s_lbl_today_unit, &font_passport_16, 0);
    lv_obj_align(s_lbl_today_unit, LV_ALIGN_TOP_RIGHT, -12, 36);

    lv_obj_t *strip = lv_obj_create(parent);
    strip_obj(strip);
    lv_obj_set_size(strip, 234, 54);
    lv_obj_align(strip, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(strip, lv_color_hex(COL_INK), 0);
    lv_obj_set_style_border_width(strip, 1, 0);
    lv_obj_set_style_border_side(strip, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(strip, lv_color_hex(COL_HAIR), 0);

    const char *titles[3] = {"TOTAL", "WEEK", "DAYS"};
    lv_obj_t **vals[3] = {&s_lbl_m_total, &s_lbl_m_week, &s_lbl_m_streak};
    const char *seed[3] = {"--", "--", "--"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *cell = lv_obj_create(strip);
        strip_obj(cell);
        lv_obj_set_size(cell, 76, 54);
        lv_obj_set_pos(cell, 2 + i * 77, 0);
        lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
        if (i < 2) {
            lv_obj_set_style_border_width(cell, 1, 0);
            lv_obj_set_style_border_side(cell, LV_BORDER_SIDE_RIGHT, 0);
            lv_obj_set_style_border_color(cell, lv_color_hex(COL_HAIR), 0);
        }
        lv_obj_t *t = lv_label_create(cell);
        lv_label_set_text(t, titles[i]);
        style_micro(t);
        lv_obj_set_width(t, 64);
        lv_label_set_long_mode(t, LV_LABEL_LONG_CLIP);
        lv_obj_align(t, LV_ALIGN_TOP_LEFT, 6, 8);
        *vals[i] = lv_label_create(cell);
        lv_label_set_text(*vals[i], seed[i]);
        lv_obj_set_style_text_color(*vals[i], lv_color_hex(COL_IVORY), 0);
        lv_obj_set_style_text_font(*vals[i], &font_passport_16, 0);
        lv_obj_set_width(*vals[i], 68);
        lv_label_set_long_mode(*vals[i], LV_LABEL_LONG_CLIP);
        lv_obj_align(*vals[i], LV_ALIGN_TOP_LEFT, 6, 26);
    }
}

static void create_page_quota(lv_obj_t *parent)
{
    lv_obj_t *kicker = lv_label_create(parent);
    lv_label_set_text(kicker, "QUOTA");
    style_kicker(kicker);
    lv_obj_align(kicker, LV_ALIGN_TOP_LEFT, 12, 10);

    s_lbl_q_window = lv_label_create(parent);
    lv_label_set_text(s_lbl_q_window, "CODEX  5H / WEEK");
    style_micro(s_lbl_q_window);
    lv_obj_align(s_lbl_q_window, LV_ALIGN_TOP_RIGHT, -12, 12);

    for (int i = 0; i < PASSPORT_QUOTA_ACCOUNTS; i++) {
        int y = 36 + i * 62;

        s_lbl_q_name[i] = lv_label_create(parent);
        lv_label_set_text(s_lbl_q_name[i], "--");
        lv_obj_set_style_text_color(s_lbl_q_name[i], lv_color_hex(COL_IVORY), 0);
        lv_obj_set_style_text_font(s_lbl_q_name[i], &font_passport_16, 0);
        lv_obj_set_width(s_lbl_q_name[i], 70);
        lv_label_set_long_mode(s_lbl_q_name[i], LV_LABEL_LONG_CLIP);
        lv_obj_align(s_lbl_q_name[i], LV_ALIGN_TOP_LEFT, 12, y);

        s_lbl_q_short[i] = lv_label_create(parent);
        lv_label_set_text(s_lbl_q_short[i], "0%");
        lv_obj_set_style_text_color(s_lbl_q_short[i], lv_color_hex(COL_GOLD), 0);
        lv_obj_set_style_text_font(s_lbl_q_short[i], &lv_font_montserrat_14, 0);
        lv_obj_set_width(s_lbl_q_short[i], 100);
        lv_obj_set_style_text_align(s_lbl_q_short[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(s_lbl_q_short[i], LV_ALIGN_TOP_LEFT, 12, y);

        s_lbl_q_week[i] = lv_label_create(parent);
        lv_label_set_text(s_lbl_q_week[i], "0%");
        lv_obj_set_style_text_color(s_lbl_q_week[i], lv_color_hex(COL_GOLD), 0);
        lv_obj_set_style_text_font(s_lbl_q_week[i], &lv_font_montserrat_14, 0);
        lv_obj_align(s_lbl_q_week[i], LV_ALIGN_TOP_RIGHT, -12, y);

        lv_obj_t *track_s = lv_obj_create(parent);
        strip_obj(track_s);
        lv_obj_set_size(track_s, 100, 6);
        lv_obj_align(track_s, LV_ALIGN_TOP_LEFT, 12, y + 22);
        lv_obj_set_style_bg_color(track_s, lv_color_hex(0x1E2738), 0);
        s_bar_q_short[i] = lv_obj_create(track_s);
        strip_obj(s_bar_q_short[i]);
        lv_obj_set_size(s_bar_q_short[i], 4, 6);
        lv_obj_align(s_bar_q_short[i], LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_bg_color(s_bar_q_short[i], lv_color_hex(COL_GOLD), 0);

        lv_obj_t *track_w = lv_obj_create(parent);
        strip_obj(track_w);
        lv_obj_set_size(track_w, 100, 6);
        lv_obj_align(track_w, LV_ALIGN_TOP_RIGHT, -12, y + 22);
        lv_obj_set_style_bg_color(track_w, lv_color_hex(0x1E2738), 0);
        s_bar_q_week[i] = lv_obj_create(track_w);
        strip_obj(s_bar_q_week[i]);
        lv_obj_set_size(s_bar_q_week[i], 4, 6);
        lv_obj_align(s_bar_q_week[i], LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_bg_color(s_bar_q_week[i], lv_color_hex(COL_GOLD), 0);

        s_lbl_q_reset[i] = lv_label_create(parent);
        lv_label_set_text(s_lbl_q_reset[i], "5H  --");
        style_micro(s_lbl_q_reset[i]);
        lv_obj_set_width(s_lbl_q_reset[i], 100);
        lv_obj_align(s_lbl_q_reset[i], LV_ALIGN_TOP_LEFT, 12, y + 34);

        s_lbl_q_reset_w[i] = lv_label_create(parent);
        lv_label_set_text(s_lbl_q_reset_w[i], "W  --");
        style_micro(s_lbl_q_reset_w[i]);
        lv_obj_align(s_lbl_q_reset_w[i], LV_ALIGN_TOP_RIGHT, -12, y + 34);
    }
}

static void create_page_qr(lv_obj_t *parent)
{
    lv_obj_t *kicker = lv_label_create(parent);
    lv_label_set_text(kicker, "HOMEPAGE");
    style_kicker(kicker);
    lv_obj_align(kicker, LV_ALIGN_TOP_MID, 0, 16);

#if LV_USE_QRCODE
    lv_obj_t *well = lv_obj_create(parent);
    strip_obj(well);
    lv_obj_set_size(well, 156, 156);
    lv_obj_align(well, LV_ALIGN_TOP_MID, 0, 42);
    lv_obj_set_style_bg_color(well, lv_color_hex(0xF4EFE4), 0);
    lv_obj_set_style_pad_all(well, 8, 0);
    lv_obj_set_style_border_width(well, 1, 0);
    lv_obj_set_style_border_color(well, lv_color_hex(COL_GOLD_DEEP), 0);

    s_qr_widget = lv_qrcode_create(well);
    lv_qrcode_set_size(s_qr_widget, 140);
    lv_qrcode_set_dark_color(s_qr_widget, lv_color_hex(COL_INK));
    lv_qrcode_set_light_color(s_qr_widget, lv_color_hex(0xF4EFE4));
    const char *url = "https://github.com/Ljhhhhhh";
    lv_qrcode_update(s_qr_widget, url, strlen(url));
    lv_obj_center(s_qr_widget);
#endif

    s_lbl_qr_url = lv_label_create(parent);
    lv_label_set_text(s_lbl_qr_url, "github.com/Ljhhhhhh");
    lv_obj_set_style_text_color(s_lbl_qr_url, lv_color_hex(COL_IVORY_DIM), 0);
    lv_obj_set_style_text_font(s_lbl_qr_url, &lv_font_montserrat_14, 0);
    lv_obj_align(s_lbl_qr_url, LV_ALIGN_TOP_MID, 0, 210);
}

static void create_page_settings(lv_obj_t *parent)
{
    lv_obj_t *kicker = lv_label_create(parent);
    lv_label_set_text(kicker, "SETTINGS");
    style_kicker(kicker);
    lv_obj_align(kicker, LV_ALIGN_TOP_LEFT, 12, 12);

    lv_obj_t *tip = lv_label_create(parent);
    lv_label_set_text(tip, "OK:切换开关/音量");
    style_micro(tip);
    lv_obj_set_style_text_font(tip, &font_passport_16, 0);
    lv_obj_align(tip, LV_ALIGN_TOP_RIGHT, -12, 12);

    // Option 1: Voice Alert Switch
    s_box_cfg_voice = lv_obj_create(parent);
    strip_obj(s_box_cfg_voice);
    lv_obj_set_size(s_box_cfg_voice, 210, 68);
    lv_obj_align(s_box_cfg_voice, LV_ALIGN_TOP_MID, 0, 42);
    lv_obj_set_style_bg_color(s_box_cfg_voice, lv_color_hex(COL_STATUS), 0);
    lv_obj_set_style_border_width(s_box_cfg_voice, 1, 0);
    lv_obj_set_style_border_color(s_box_cfg_voice, lv_color_hex(COL_HAIR), 0);
    lv_obj_set_style_pad_all(s_box_cfg_voice, 8, 0);

    lv_obj_t *lbl_v_title = lv_label_create(s_box_cfg_voice);
    lv_label_set_text(lbl_v_title, "语音播报");
    lv_obj_set_style_text_color(lbl_v_title, lv_color_hex(COL_IVORY), 0);
    lv_obj_set_style_text_font(lbl_v_title, &font_passport_16, 0);
    lv_obj_align(lbl_v_title, LV_ALIGN_TOP_LEFT, 4, 4);

    lv_obj_t *lbl_v_sub = lv_label_create(s_box_cfg_voice);
    lv_label_set_text(lbl_v_sub, "完成/等待/失败提示音");
    style_micro(lbl_v_sub);
    lv_obj_set_style_text_font(lbl_v_sub, &font_passport_16, 0);
    lv_obj_align(lbl_v_sub, LV_ALIGN_BOTTOM_LEFT, 4, -4);

    s_lbl_cfg_voice_val = lv_label_create(s_box_cfg_voice);
    lv_label_set_text(s_lbl_cfg_voice_val, "开启");
    lv_obj_set_style_text_color(s_lbl_cfg_voice_val, lv_color_hex(COL_DONE), 0);
    lv_obj_set_style_text_font(s_lbl_cfg_voice_val, &font_passport_16, 0);
    lv_obj_align(s_lbl_cfg_voice_val, LV_ALIGN_RIGHT_MID, -8, 0);

    // Option 2: Volume Setting
    s_box_cfg_vol = lv_obj_create(parent);
    strip_obj(s_box_cfg_vol);
    lv_obj_set_size(s_box_cfg_vol, 210, 84);
    lv_obj_align(s_box_cfg_vol, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_set_style_bg_color(s_box_cfg_vol, lv_color_hex(COL_STATUS), 0);
    lv_obj_set_style_border_width(s_box_cfg_vol, 1, 0);
    lv_obj_set_style_border_color(s_box_cfg_vol, lv_color_hex(COL_HAIR), 0);
    lv_obj_set_style_pad_all(s_box_cfg_vol, 8, 0);

    lv_obj_t *lbl_vol_title = lv_label_create(s_box_cfg_vol);
    lv_label_set_text(lbl_vol_title, "播报音量");
    lv_obj_set_style_text_color(lbl_vol_title, lv_color_hex(COL_IVORY), 0);
    lv_obj_set_style_text_font(lbl_vol_title, &font_passport_16, 0);
    lv_obj_align(lbl_vol_title, LV_ALIGN_TOP_LEFT, 4, 4);

    s_lbl_cfg_vol_val = lv_label_create(s_box_cfg_vol);
    lv_label_set_text(s_lbl_cfg_vol_val, "80%");
    lv_obj_set_style_text_color(s_lbl_cfg_vol_val, lv_color_hex(COL_GOLD), 0);
    lv_obj_set_style_text_font(s_lbl_cfg_vol_val, &lv_font_montserrat_14, 0);
    lv_obj_align(s_lbl_cfg_vol_val, LV_ALIGN_TOP_RIGHT, -8, 4);

    s_bar_cfg_vol = lv_bar_create(s_box_cfg_vol);
    lv_obj_set_size(s_bar_cfg_vol, 190, 8);
    lv_obj_align(s_bar_cfg_vol, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_bar_set_range(s_bar_cfg_vol, 0, 100);
    lv_bar_set_value(s_bar_cfg_vol, 80, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar_cfg_vol, lv_color_hex(0x1B212D), 0);
    lv_obj_set_style_bg_color(s_bar_cfg_vol, lv_color_hex(COL_GOLD), LV_PART_INDICATOR);
    lv_obj_set_style_anim_time(s_bar_cfg_vol, 0, 0);
}

static void set_today_labels(uint64_t tokens)
{
    char num[24];
    const char *scale = "";
    format_scaled(tokens, num, sizeof(num), &scale);
    if (s_lbl_today_num) {
        lv_label_set_text(s_lbl_today_num, num);
    }
    if (s_lbl_today_unit) {
        if (scale[0] != '\0') {
            lv_label_set_text_fmt(s_lbl_today_unit, "%s Token", scale);
        } else {
            lv_label_set_text(s_lbl_today_unit, "Token");
        }
    }
}
static void format_remaining(uint32_t sec, char *out, size_t n)
{
    if (sec == 0) {
        snprintf(out, n, "READY");
        return;
    }
    unsigned days = (unsigned)(sec / 86400U);
    unsigned hours = (unsigned)((sec % 86400U) / 3600U);
    unsigned mins = (unsigned)((sec % 3600U) / 60U);
    if (days > 0) {
        snprintf(out, n, "%uD %uH", days, hours);
    } else if (hours > 0) {
        snprintf(out, n, "%uH %uM", hours, mins);
    } else {
        snprintf(out, n, "%uM", mins > 0U ? mins : 1U);
    }
}

static void set_quota_bar(lv_obj_t *bar, lv_obj_t *pct_lbl, uint8_t percent, int track_w)
{
    if (percent > 100) {
        percent = 100;
    }
    if (pct_lbl) {
        lv_label_set_text_fmt(pct_lbl, "%u%%", percent);
        lv_obj_set_style_text_color(pct_lbl, lv_color_hex(percent >= 90 ? COL_ERR : COL_GOLD), 0);
    }
    if (bar) {
        int w = (track_w * percent) / 100;
        if (w < 4) {
            w = 4;
        }
        if (w > track_w) {
            w = track_w;
        }
        lv_obj_set_width(bar, w);
        lv_obj_set_style_bg_color(bar, lv_color_hex(percent >= 90 ? COL_ERR : COL_GOLD), 0);
    }
}



static void style_stamp(uint8_t state)
{
    uint32_t color = COL_IDLE;
    const char *text = "IDLE";
    switch (state) {
    case CODEX_STATE_RUNNING:
        color = COL_GOLD;
        text = "RUN";
        break;
    case CODEX_STATE_WAITING_INPUT:
        color = COL_WAIT;
        text = "WAIT";
        break;
    case CODEX_STATE_COMPLETED:
        color = COL_DONE;
        text = "DONE";
        break;
    case CODEX_STATE_ERROR:
        color = COL_ERR;
        text = "ERR";
        break;
    default:
        break;
    }
    if (s_stamp) {
        lv_obj_set_style_border_color(s_stamp, lv_color_hex(color), 0);
        lv_obj_set_style_bg_color(s_stamp, lv_color_hex(color), 0);
        lv_obj_set_style_bg_opa(s_stamp, (state == CODEX_STATE_IDLE) ? LV_OPA_TRANSP : LV_OPA_20, 0);
    }
    if (s_lbl_status) {
        lv_label_set_text(s_lbl_status, text);
        lv_obj_set_style_text_color(s_lbl_status, lv_color_hex(color), 0);
    }
}

static void create_page_projects(lv_obj_t *parent);
esp_err_t passport_ui_init(void)
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(1000))) {
        return ESP_FAIL;
    }

    lv_obj_t *scr = lv_obj_create(NULL);
    strip_obj(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(COL_INK), 0);

    lv_obj_t *spine = lv_obj_create(scr);
    strip_obj(spine);
    lv_obj_set_size(spine, 6, 320);
    lv_obj_align(spine, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(spine, lv_color_hex(COL_BURGUNDY), 0);

    lv_obj_t *col = lv_obj_create(scr);
    strip_obj(col);
    lv_obj_set_size(col, 234, 320);
    lv_obj_align(col, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(col, lv_color_hex(COL_INK), 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *status = lv_obj_create(col);
    strip_obj(status);
    lv_obj_set_size(status, 234, 22);
    lv_obj_set_style_bg_color(status, lv_color_hex(COL_STATUS), 0);
    lv_obj_set_style_border_width(status, 1, 0);
    lv_obj_set_style_border_side(status, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(status, lv_color_hex(0x1C2536), 0);

    s_lbl_ble = lv_label_create(status);
    lv_label_set_text(s_lbl_ble, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(s_lbl_ble, lv_color_hex(COL_IDLE), 0);
    lv_obj_set_style_text_font(s_lbl_ble, &lv_font_montserrat_14, 0);
    lv_obj_align(s_lbl_ble, LV_ALIGN_LEFT_MID, 8, 0);

    s_stamp = lv_obj_create(status);
    strip_obj(s_stamp);
    lv_obj_set_size(s_stamp, 48, 14);
    lv_obj_align(s_stamp, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(s_stamp, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_stamp, 1, 0);
    lv_obj_set_style_border_color(s_stamp, lv_color_hex(COL_IDLE), 0);
    s_lbl_status = lv_label_create(s_stamp);
    lv_label_set_text(s_lbl_status, "IDLE");
    lv_obj_set_style_text_color(s_lbl_status, lv_color_hex(COL_IDLE), 0);
    lv_obj_set_style_text_font(s_lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_center(s_lbl_status);

    lv_obj_t *batt_wrap = lv_obj_create(status);
    strip_obj(batt_wrap);
    lv_obj_set_size(batt_wrap, 52, 22);
    lv_obj_align(batt_wrap, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(batt_wrap, LV_OPA_TRANSP, 0);

    s_lbl_battery = lv_label_create(batt_wrap);
    lv_label_set_text(s_lbl_battery, "100");
    lv_obj_set_style_text_color(s_lbl_battery, lv_color_hex(COL_IVORY_DIM), 0);
    lv_obj_set_style_text_font(s_lbl_battery, &lv_font_montserrat_14, 0);
    lv_obj_align(s_lbl_battery, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *shell = lv_obj_create(batt_wrap);
    strip_obj(shell);
    lv_obj_set_size(shell, 16, 8);
    lv_obj_align(shell, LV_ALIGN_RIGHT_MID, -6, 0);
    lv_obj_set_style_bg_opa(shell, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(shell, 1, 0);
    lv_obj_set_style_border_color(shell, lv_color_hex(COL_IVORY_DIM), 0);
    lv_obj_set_style_pad_all(shell, 1, 0);
    s_batt_level = lv_obj_create(shell);
    strip_obj(s_batt_level);
    lv_obj_set_size(s_batt_level, 12, 4);
    lv_obj_align(s_batt_level, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(s_batt_level, lv_color_hex(COL_DONE), 0);

    s_live = lv_obj_create(col);
    strip_obj(s_live);
    lv_obj_set_size(s_live, 234, 16);
    lv_obj_set_style_bg_color(s_live, lv_color_hex(COL_TODAY_BG), 0);
    lv_obj_set_style_border_width(s_live, 1, 0);
    lv_obj_set_style_border_side(s_live, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(s_live, lv_color_hex(0x47381A), 0);
    lv_obj_add_flag(s_live, LV_OBJ_FLAG_HIDDEN);
    s_lbl_live_proj = lv_label_create(s_live);
    lv_label_set_text(s_lbl_live_proj, "");
    lv_obj_set_style_text_color(s_lbl_live_proj, lv_color_hex(COL_GOLD), 0);
    lv_obj_set_style_text_font(s_lbl_live_proj, &font_passport_16, 0);
    lv_label_set_long_mode(s_lbl_live_proj, LV_LABEL_LONG_DOT);
    lv_obj_set_width(s_lbl_live_proj, 130);
    lv_obj_align(s_lbl_live_proj, LV_ALIGN_LEFT_MID, 10, 0);
    s_lbl_live_meta = lv_label_create(s_live);
    lv_label_set_text(s_lbl_live_meta, "");
    style_micro(s_lbl_live_meta);
    lv_obj_set_style_text_font(s_lbl_live_meta, &font_passport_16, 0);
    lv_obj_align(s_lbl_live_meta, LV_ALIGN_RIGHT_MID, -10, 0);

    lv_obj_t *content = lv_obj_create(col);
    strip_obj(content);
    lv_obj_set_width(content, 234);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_color(content, lv_color_hex(COL_INK), 0);

    for (int i = 0; i < PAGE_COUNT; i++) {
        s_pages[i] = lv_obj_create(content);
        strip_obj(s_pages[i]);
        lv_obj_set_size(s_pages[i], 234, LV_PCT(100));
        lv_obj_align(s_pages[i], LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(s_pages[i], lv_color_hex(COL_INK), 0);
        lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
    }

    create_page_home(s_pages[PAGE_PROFILE]);
    create_page_quota(s_pages[PAGE_QUOTA]);
    create_page_qr(s_pages[PAGE_QR_CODE]);
    create_page_projects(s_pages[PAGE_PROJECTS]);
    create_page_settings(s_pages[PAGE_SETTINGS]);

    s_pager = lv_obj_create(col);
    strip_obj(s_pager);
    lv_obj_set_size(s_pager, 234, 16);
    lv_obj_set_style_bg_color(s_pager, lv_color_hex(COL_STATUS), 0);
    lv_obj_set_flex_flow(s_pager, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_pager, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(s_pager, 4, 0);
    for (int i = 0; i < PAGE_CYCLE; i++) {
        s_ticks[i] = lv_obj_create(s_pager);
        strip_obj(s_ticks[i]);
        lv_obj_set_size(s_ticks[i], 10, 2);
        lv_obj_set_style_bg_color(s_ticks[i], lv_color_hex(0x2A3346), 0);
    }

    show_page(PAGE_PROJECTS);
    lv_screen_load(scr);
    bsp_lvgl_unlock();

    passport_profile_t p;
    passport_stats_t s;
    passport_heatmap_t h;
    passport_footprints_t f;
    passport_directions_t d;
    passport_quota_t q;
    if (passport_storage_load_all(&p, &s, &h, &f, &d) == ESP_OK) {
        passport_ui_update_profile(&p);
        passport_ui_update_stats(&s);
    }
    if (passport_storage_load_quota(&q) == ESP_OK) {
        passport_ui_update_quota(&q);
    }
    passport_settings_t cfg;
    if (passport_storage_load_settings(&cfg) == ESP_OK) {
        passport_alert_set_settings(cfg.voice_enabled != 0, cfg.volume);
        passport_ui_update_settings(&cfg);
    }

    ESP_LOGI(TAG, "Passport UI ready (4 pages + QR, Settings enabled).");
    return ESP_OK;
}

void passport_ui_next_page(void)
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(100))) {
        return;
    }
    if (s_qr_active) {
        s_qr_active = false;
        show_page(s_prev_page_before_qr);
    } else {
        show_page((s_current_page + 1) % PAGE_CYCLE);
    }
    bsp_lvgl_unlock();
}

void passport_ui_prev_page(void)
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(100))) {
        return;
    }
    if (s_qr_active) {
        s_qr_active = false;
        show_page(s_prev_page_before_qr);
    } else {
        show_page((s_current_page + PAGE_CYCLE - 1) % PAGE_CYCLE);
    }
    bsp_lvgl_unlock();
}

bool passport_ui_take_project_update(void)
{
    return atomic_exchange(&s_project_updated, false);
}

void passport_ui_show_projects(void)
{
    if (bsp_lvgl_lock(pdMS_TO_TICKS(100))) {
        s_qr_active = false;
        show_page(PAGE_PROJECTS);
        bsp_lvgl_unlock();
    }
}

uint8_t passport_ui_project_page(void)
{
    uint8_t page = 0;
    if (bsp_lvgl_lock(pdMS_TO_TICKS(100))) {
        page = s_project_page;
        bsp_lvgl_unlock();
    }
    return page;
}

void passport_ui_toggle_qr(void)
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(100))) {
        return;
    }
    if (s_qr_active) {
        s_qr_active = false;
        show_page(s_prev_page_before_qr);
    } else {
        s_prev_page_before_qr = s_current_page;
        s_qr_active = true;
        show_page(PAGE_QR_CODE);
    }
    bsp_lvgl_unlock();
}

static bool move_message(int delta)
{
    if (s_qr_active || s_current_page != PAGE_MESSAGES || s_project_pages == 0) return false;
    /* Consume repeats until the requested page arrives, including reverse presses. */
    if (s_message_page_pending) return true;
    int count = s_message_count;
    int idx = (int)s_selected_message + delta;
    if (idx >= 0 && idx < count) {
        s_selected_message = (uint8_t)idx;
        highlight_message();
        return true;
    }
    if (!s_ble_connected) return false;
    if (delta > 0) {
        if (s_project_page + 1U >= s_project_pages) return false;
        s_prefer_last_message = false;
        s_selected_message = 0;
        s_voice_targets_valid = false;
        s_project_page++;
        s_message_page_pending = true;
        highlight_message();
        return true;
    }
    if (s_project_page == 0) return false;
    s_prefer_last_message = true;
    s_voice_targets_valid = false;
    s_project_page--;
    s_message_page_pending = true;
    highlight_message();
    return true;
}

bool passport_ui_is_settings_page(void)
{
    bool is_settings = false;
    if (bsp_lvgl_lock(pdMS_TO_TICKS(100))) {
        is_settings = (!s_qr_active && s_current_page == PAGE_SETTINGS);
        bsp_lvgl_unlock();
    }
    return is_settings;
}

bool passport_ui_is_messages_page(void)
{
    bool is_messages = false;
    if (bsp_lvgl_lock(pdMS_TO_TICKS(100))) {
        is_messages = (!s_qr_active && s_current_page == PAGE_MESSAGES);
        bsp_lvgl_unlock();
    }
    return is_messages;
}

void passport_ui_settings_next(void)
{
}

void passport_ui_settings_prev(void)
{
}

void passport_ui_settings_toggle(void)
{
    bool voice = true;
    uint8_t vol = 80;
    passport_alert_get_settings(&voice, &vol);
    // Cycle through: 80% (ON) -> 100% (ON) -> MUTED (OFF) -> 20% (ON) -> 40% (ON) -> 60% (ON) -> 80% (ON)
    if (!voice || vol == 0) {
        voice = true;
        vol = 20;
    } else if (vol >= 100) {
        voice = false;
        vol = 0;
    } else {
        vol += 20;
    }
    passport_alert_set_settings(voice, vol);
    passport_settings_t cfg = { .voice_enabled = voice ? 1 : 0, .volume = vol };
    passport_storage_save_settings(&cfg);
    passport_ui_update_settings(&cfg);
}

void passport_ui_next_item(void)
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(100))) return;
    bool moved = move_message(1);
    if (!moved) {
        if (s_qr_active) {
            s_qr_active = false;
            show_page(s_prev_page_before_qr);
        } else {
            show_page((s_current_page + 1) % PAGE_CYCLE);
        }
    }
    bsp_lvgl_unlock();
}

void passport_ui_prev_item(void)
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(100))) return;
    bool moved = move_message(-1);
    if (!moved) {
        if (s_qr_active) {
            s_qr_active = false;
            show_page(s_prev_page_before_qr);
        } else {
            show_page((s_current_page + PAGE_CYCLE - 1) % PAGE_CYCLE);
        }
    }
    bsp_lvgl_unlock();
}

void passport_ui_update_profile(const passport_profile_t *profile)
{
    if (!profile) {
        return;
    }
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) {
        return;
    }
    if (s_lbl_name) {
        lv_label_set_text(s_lbl_name, profile->name);
    }
    if (s_lbl_issue) {
        char issued[24];
        format_issue_short(profile->issue_date, issued, sizeof(issued));
        lv_label_set_text(s_lbl_issue, issued);
    }
#if LV_USE_QRCODE
    if (s_qr_widget && profile->homepage[0] != '\0') {
        lv_qrcode_update(s_qr_widget, profile->homepage, strlen(profile->homepage));
    }
#endif
    if (s_lbl_qr_url && profile->homepage[0] != '\0') {
        const char *p = profile->homepage;
        if (strncmp(p, "https://", 8) == 0) {
            p += 8;
        } else if (strncmp(p, "http://", 7) == 0) {
            p += 7;
        }
        lv_label_set_text(s_lbl_qr_url, p);
    }
    bsp_lvgl_unlock();
}

void passport_ui_update_stats(const passport_stats_t *stats)
{
    if (!stats) {
        return;
    }
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) {
        return;
    }
    char num[24];
    const char *scale = "";
    format_scaled(stats->total_tokens, num, sizeof(num), &scale);
    if (s_lbl_m_total) {
        if (scale[0] != '\0') {
            lv_label_set_text_fmt(s_lbl_m_total, "%s%s", num, scale);
        } else {
            lv_label_set_text(s_lbl_m_total, num);
        }
    }
    format_scaled(stats->week_tokens, num, sizeof(num), &scale);
    if (s_lbl_m_week) {
        if (scale[0] != '\0') {
            lv_label_set_text_fmt(s_lbl_m_week, "%s%s", num, scale);
        } else {
            lv_label_set_text(s_lbl_m_week, num);
        }
    }
    if (s_lbl_m_streak) {
        lv_label_set_text_fmt(s_lbl_m_streak, "%d天", stats->streak_days);
    }
    if (s_lbl_sync) {
        if (stats->synced_at[0] != '\0') {
            lv_label_set_text_fmt(s_lbl_sync, "SYNC  %s", stats->synced_at);
        } else {
            lv_label_set_text(s_lbl_sync, "SYNC  --");
        }
    }
    set_today_labels(stats->today_tokens);
    bsp_lvgl_unlock();
}

void passport_ui_update_quota(const passport_quota_t *quota)
{
    if (!quota) {
        return;
    }
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) {
        return;
    }
    uint8_t hours = quota->short_window_hours ? quota->short_window_hours : 5;
    if (s_lbl_q_window) {
        lv_label_set_text_fmt(s_lbl_q_window, "CODEX  %uH / WEEK", hours);
    }
    for (int i = 0; i < PASSPORT_QUOTA_ACCOUNTS; i++) {
        if (i >= quota->count) {
            if (s_lbl_q_name[i]) {
                lv_label_set_text(s_lbl_q_name[i], "");
            }
            if (s_lbl_q_short[i]) {
                lv_label_set_text(s_lbl_q_short[i], "");
            }
            if (s_lbl_q_week[i]) {
                lv_label_set_text(s_lbl_q_week[i], "");
            }
            if (s_lbl_q_reset[i]) {
                lv_label_set_text(s_lbl_q_reset[i], "");
            }
            if (s_lbl_q_reset_w[i]) {
                lv_label_set_text(s_lbl_q_reset_w[i], "");
            }
            if (s_bar_q_short[i]) {
                lv_obj_set_width(s_bar_q_short[i], 4);
            }
            if (s_bar_q_week[i]) {
                lv_obj_set_width(s_bar_q_week[i], 4);
            }
            continue;
        }
        const passport_quota_account_t *a = &quota->items[i];
        if (s_lbl_q_name[i]) {
            lv_label_set_text(s_lbl_q_name[i], a->name);
        }
        set_quota_bar(s_bar_q_short[i], s_lbl_q_short[i], a->short_percent, 100);
        set_quota_bar(s_bar_q_week[i], s_lbl_q_week[i], a->weekly_percent, 100);
        char rs[16];
        char rw[16];
        format_remaining(a->short_remaining_sec, rs, sizeof(rs));
        format_remaining(a->weekly_remaining_sec, rw, sizeof(rw));
        if (s_lbl_q_reset[i]) {
            lv_label_set_text_fmt(s_lbl_q_reset[i], "5H  %s", rs);
        }
        if (s_lbl_q_reset_w[i]) {
            lv_label_set_text_fmt(s_lbl_q_reset_w[i], "W  %s", rw);
        }
    }
    bsp_lvgl_unlock();
}



void passport_ui_update_heatmap(const passport_heatmap_t *heatmap)
{
    (void)heatmap;
}

void passport_ui_update_footprints(const passport_footprints_t *footprints)
{
    (void)footprints;
}

void passport_ui_update_directions(const passport_directions_t *directions)
{
    (void)directions;
}

void passport_ui_update_realtime(const passport_realtime_t *rt)
{
    if (!rt) {
        return;
    }
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) {
        return;
    }
    style_stamp(rt->state);
    if (s_live) {
        if (rt->state == CODEX_STATE_IDLE) {
            lv_obj_add_flag(s_live, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(s_live, LV_OBJ_FLAG_HIDDEN);
            if (s_lbl_live_proj) {
                lv_label_set_text(s_lbl_live_proj, rt->project_name);
            }
            if (s_lbl_live_meta) {
                unsigned min = rt->duration_sec / 60;
                unsigned sec = rt->duration_sec % 60;
                char num[24];
                const char *scale = "";
                format_scaled(rt->turn_tokens, num, sizeof(num), &scale);
                lv_label_set_text_fmt(s_lbl_live_meta, "%u:%02u  %s%s", min, sec, num, scale);
            }
        }
    }
    bsp_lvgl_unlock();
}


void passport_ui_set_ble_connected(bool connected)
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) {
        return;
    }
    s_ble_connected = connected;
    if (!connected) {
        s_project_page = s_rendered_message_page;
        s_message_page_pending = false;
        s_prefer_last_message = false;
        s_voice_targets_valid = false;
        if (s_selected_message >= s_message_count)
            s_selected_message = s_message_count ? s_message_count - 1 : 0;
    }
    highlight_message();
    if (s_lbl_ble) {
        lv_obj_set_style_text_color(s_lbl_ble, lv_color_hex(connected ? COL_GOLD : COL_IDLE), 0);
    }
    bsp_lvgl_unlock();
}

void passport_ui_update_battery(int percent, bool is_charging)
{
    if (percent < 0) {
        percent = 0;
    }
    if (percent > 100) {
        percent = 100;
    }
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) {
        return;
    }
    if (s_lbl_battery) {
        lv_label_set_text_fmt(s_lbl_battery, "%d", percent);
        lv_obj_set_style_text_color(s_lbl_battery, lv_color_hex(is_charging ? COL_GOLD : COL_IVORY_DIM), 0);
    }
    if (s_batt_level) {
        int w = (12 * percent) / 100;
        if (w < 1) {
            w = 1;
        }
        lv_obj_set_width(s_batt_level, w);
        uint32_t c = COL_DONE;
        if (is_charging) {
            c = COL_GOLD;
        } else if (percent < 20) {
            c = COL_ERR;
        }
        lv_obj_set_style_bg_color(s_batt_level, lv_color_hex(c), 0);
    }
    bsp_lvgl_unlock();
}

void passport_ui_update_settings(const passport_settings_t *settings)
{
    if (!settings) return;
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) return;
    if (s_lbl_cfg_voice_val) {
        lv_label_set_text(s_lbl_cfg_voice_val, settings->voice_enabled ? "开启" : "静音");
        lv_obj_set_style_text_color(s_lbl_cfg_voice_val, lv_color_hex(settings->voice_enabled ? COL_DONE : COL_MUTED), 0);
    }
    if (s_lbl_cfg_vol_val) {
        if (settings->voice_enabled && settings->volume > 0) {
            lv_label_set_text_fmt(s_lbl_cfg_vol_val, "%u%%", (unsigned)settings->volume);
            lv_obj_set_style_text_color(s_lbl_cfg_vol_val, lv_color_hex(COL_GOLD), 0);
        } else {
            lv_label_set_text(s_lbl_cfg_vol_val, "静音");
            lv_obj_set_style_text_color(s_lbl_cfg_vol_val, lv_color_hex(COL_MUTED), 0);
        }
    }
    if (s_bar_cfg_vol) {
        lv_bar_set_value(s_bar_cfg_vol, settings->voice_enabled ? settings->volume : 0, LV_ANIM_OFF);
    }
    bsp_lvgl_unlock();
}

static lv_obj_t *s_lbl_proj_title = NULL;
static lv_obj_t *s_lbl_msg_title[3] = {NULL};
static lv_obj_t *s_lbl_msg_proj[3] = {NULL};
static lv_obj_t *s_lbl_msg_status[3] = {NULL};
static lv_obj_t *s_box_proj[3] = {NULL};
static bool s_sync_error = false;

static void create_page_projects(lv_obj_t *parent)
{
    lv_obj_t *kicker = lv_label_create(parent);
    lv_label_set_text(kicker, "MESSAGES / UP DOWN");
    style_kicker(kicker);
    lv_obj_align(kicker, LV_ALIGN_TOP_LEFT, 10, 8);

    s_lbl_proj_title = lv_label_create(parent);
    lv_label_set_text(s_lbl_proj_title, "Waiting for Mac sync");
    style_micro(s_lbl_proj_title);
    lv_obj_set_style_text_font(s_lbl_proj_title, &font_passport_16, 0);
    lv_obj_set_width(s_lbl_proj_title, 214);
    lv_label_set_long_mode(s_lbl_proj_title, LV_LABEL_LONG_DOT);
    lv_obj_align(s_lbl_proj_title, LV_ALIGN_TOP_LEFT, 10, 24);

    for (int i = 0; i < 3; i++) {
        int y = 44 + i * 66;
        s_box_proj[i] = lv_obj_create(parent);
        strip_obj(s_box_proj[i]);
        lv_obj_set_size(s_box_proj[i], 214, 60);
        lv_obj_align(s_box_proj[i], LV_ALIGN_TOP_MID, 0, y);
        lv_obj_set_style_bg_color(s_box_proj[i], lv_color_hex(COL_STATUS), 0);
        lv_obj_set_style_border_width(s_box_proj[i], 1, 0);
        lv_obj_set_style_border_color(s_box_proj[i], lv_color_hex(COL_HAIR), 0);

        s_lbl_msg_title[i] = lv_label_create(s_box_proj[i]);
        lv_label_set_text(s_lbl_msg_title[i], "--");
        lv_obj_set_style_text_color(s_lbl_msg_title[i], lv_color_hex(COL_IVORY), 0);
        lv_obj_set_style_text_font(s_lbl_msg_title[i], &font_passport_16, 0);
        lv_obj_set_width(s_lbl_msg_title[i], 198);
        lv_obj_set_height(s_lbl_msg_title[i], 20);
        lv_label_set_long_mode(s_lbl_msg_title[i], LV_LABEL_LONG_DOT);
        lv_obj_align(s_lbl_msg_title[i], LV_ALIGN_TOP_LEFT, 8, 8);

        s_lbl_msg_status[i] = lv_label_create(s_box_proj[i]);
        lv_label_set_text(s_lbl_msg_status[i], "");
        lv_obj_set_style_text_font(s_lbl_msg_status[i], &font_passport_16, 0);
        lv_obj_align(s_lbl_msg_status[i], LV_ALIGN_BOTTOM_LEFT, 8, -8);

        s_lbl_msg_proj[i] = lv_label_create(s_box_proj[i]);
        lv_label_set_text(s_lbl_msg_proj[i], "");
        lv_obj_set_style_text_color(s_lbl_msg_proj[i], lv_color_hex(COL_MUTED), 0);
        lv_obj_set_style_text_font(s_lbl_msg_proj[i], &font_passport_16, 0);
        lv_obj_set_width(s_lbl_msg_proj[i], 130);
        lv_label_set_long_mode(s_lbl_msg_proj[i], LV_LABEL_LONG_DOT);
        lv_obj_align(s_lbl_msg_proj[i], LV_ALIGN_BOTTOM_RIGHT, -8, -8);
    }
}

void passport_ui_set_sync_error(bool error)
{
    s_sync_error = error;
}

static void render_projects_locked(const passport_projects_page_t *projects)
{
    atomic_store(&s_project_updated, true);
    s_project_pages = projects->total_pages ? projects->total_pages : 1;
    s_project_page = projects->page_index;
    s_message_page_pending = false;
    s_rendered_message_page = projects->page_index;
    s_message_count = projects->count;
    if (s_prefer_last_message || s_selected_message >= s_message_count)
        s_selected_message = s_message_count ? s_message_count - 1 : 0;
    s_prefer_last_message = false;
    for (int i = 0; i < 3; i++) {
        if (i < projects->count) {
            const passport_message_item_t *it = &projects->items[i];
            if (s_lbl_msg_title[i]) lv_label_set_text(s_lbl_msg_title[i], it->title);
            if (s_lbl_msg_proj[i]) lv_label_set_text(s_lbl_msg_proj[i], it->project);
            const char *st_text = "";
            uint32_t st_color = COL_MUTED;
            if (it->status == 1) {
                st_text = "待回复";
                st_color = COL_WAIT;
            } else if (it->status == 2) {
                st_text = "已完成";
                st_color = COL_DONE;
            } else if (it->status == 3) {
                st_text = "失败";
                st_color = COL_ERR;
            } else if (it->status == 4) {
                st_text = "进行中";
                st_color = COL_GOLD;
            }
            if (s_lbl_msg_status[i]) {
                lv_label_set_text(s_lbl_msg_status[i], st_text);
                lv_obj_set_style_text_color(s_lbl_msg_status[i], lv_color_hex(st_color), 0);
            }
            if (s_box_proj[i]) {
                lv_obj_set_style_border_color(s_box_proj[i], lv_color_hex(st_color != COL_MUTED ? st_color : COL_HAIR), 0);
            }
        } else {
            if (s_lbl_msg_title[i]) lv_label_set_text(s_lbl_msg_title[i], "");
            if (s_lbl_msg_proj[i]) lv_label_set_text(s_lbl_msg_proj[i], "");
            if (s_lbl_msg_status[i]) lv_label_set_text(s_lbl_msg_status[i], "");
            if (s_box_proj[i]) lv_obj_set_style_border_color(s_box_proj[i], lv_color_hex(COL_HAIR), 0);
        }
    }
    if (s_lbl_proj_title) {
        if (s_sync_error) {
            lv_label_set_text(s_lbl_proj_title, "同步异常");
            lv_obj_set_style_text_color(s_lbl_proj_title, lv_color_hex(COL_ERR), 0);
        } else {
            lv_label_set_text_fmt(s_lbl_proj_title, "PAGE %d/%d", projects->page_index + 1, projects->total_pages > 0 ? projects->total_pages : 1);
            lv_obj_set_style_text_color(s_lbl_proj_title, lv_color_hex(COL_MUTED), 0);
        }
    }
    if (projects->count == 0) {
        if (s_sync_error) {
            lv_label_set_text(s_lbl_msg_title[0], "同步异常");
            lv_label_set_text(s_lbl_msg_proj[0], "无法获取未读状态");
        } else {
            lv_label_set_text(s_lbl_msg_title[0], "暂无消息");
            lv_label_set_text(s_lbl_msg_proj[0], "");
        }
        if (s_lbl_msg_status[0]) lv_label_set_text(s_lbl_msg_status[0], "");
    }
}

static void highlight_message(void)
{
    for (int i = 0; i < PASSPORT_MESSAGES_PER_PAGE; ++i) {
        if (s_box_proj[i]) {
            bool selected = !s_message_page_pending && i == s_selected_message &&
                            i < s_message_count;
            lv_obj_set_style_border_width(s_box_proj[i], selected ? 2 : 1, 0);
            lv_obj_set_style_bg_color(s_box_proj[i], lv_color_hex(selected ? 0x282315 : COL_STATUS), 0);
        }
    }
    if (s_lbl_proj_title) {
        if (s_message_page_pending)
            lv_label_set_text_fmt(s_lbl_proj_title, "Loading page %u", s_project_page + 1);
        else if (!s_ble_connected)
            lv_label_set_text(s_lbl_proj_title, "Disconnected");
        else if (s_sync_error)
            lv_label_set_text(s_lbl_proj_title, "同步异常");
        else
            lv_label_set_text_fmt(s_lbl_proj_title, "%u/%u%s", s_project_page + 1,
                                 s_project_pages, s_voice_targets_valid && s_message_count ? "  2xOK talk" : "");
    }
}

static bool message_page_matches(const passport_messages_page_t *page)
{
    /* The host clamps an out-of-range request when messages disappear. */
    uint8_t pages = page->total_pages ? page->total_pages : 1;
    uint8_t expected = s_project_page < pages ? s_project_page : pages - 1;
    return page->page_index == expected;
}

bool passport_ui_update_projects(const passport_projects_page_t *projects)
{
    if (!projects || !bsp_lvgl_lock(pdMS_TO_TICKS(500))) return false;
    if (!message_page_matches(projects)) {
        bsp_lvgl_unlock();
        return true;
    }
    s_voice_targets_valid = false;
    render_projects_locked(projects);
    highlight_message();
    bsp_lvgl_unlock();
    return true;
}

bool passport_ui_update_voice_messages(const passport_voice_messages_page_t *snapshot)
{
    if (!snapshot || !bsp_lvgl_lock(pdMS_TO_TICKS(500))) return false;
    if (!message_page_matches(&snapshot->messages)) {
        bsp_lvgl_unlock();
        return true;
    }
    uint8_t previous_id[16] = {0};
    if (s_voice_targets_valid) memcpy(previous_id, s_voice_messages.thread_ids[s_selected_message], 16);
    s_selected_message = 0;
    for (int i = 0; i < snapshot->messages.count; ++i) {
        if (!memcmp(previous_id, snapshot->thread_ids[i], 16)) s_selected_message = i;
    }
    if (snapshot->messages.count && s_selected_message >= snapshot->messages.count)
        s_selected_message = snapshot->messages.count - 1;
    s_voice_messages = *snapshot;
    s_voice_targets_valid = s_ble_connected;
    render_projects_locked(&snapshot->messages);
    highlight_message();
    bsp_lvgl_unlock();
    return true;
}

bool passport_ui_voice_target(uint8_t thread_id[16], char title[64])
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(100))) return false;
    static const uint8_t empty[16] = {0};
    bool valid = s_current_page == PAGE_MESSAGES && !s_qr_active && s_voice_targets_valid &&
                 s_selected_message < s_voice_messages.messages.count &&
                 memcmp(s_voice_messages.thread_ids[s_selected_message], empty, 16);
    if (valid) {
        memcpy(thread_id, s_voice_messages.thread_ids[s_selected_message], 16);
        memcpy(title, s_voice_messages.messages.items[s_selected_message].title, 64);
    }
    bsp_lvgl_unlock();
    return valid;
}

bool passport_ui_voice_show(const char *phase, const char *body, const char *hint)
{
    if (!bsp_lvgl_lock(500)) return false;
    if (!s_voice_overlay) {
        s_voice_overlay = lv_obj_create(lv_layer_top());
        strip_obj(s_voice_overlay);
        lv_obj_set_size(s_voice_overlay, 234, 286);
        lv_obj_align(s_voice_overlay, LV_ALIGN_BOTTOM_MID, 0, -3);
        lv_obj_set_style_bg_color(s_voice_overlay, lv_color_hex(COL_INK), 0);
        s_voice_phase = lv_label_create(s_voice_overlay);
        lv_obj_set_size(s_voice_phase, 214, 42);
        lv_obj_set_pos(s_voice_phase, 10, 8);
        lv_obj_set_style_text_font(s_voice_phase, &font_passport_16, 0);
        lv_obj_set_style_text_color(s_voice_phase, lv_color_hex(COL_GOLD), 0);
        s_voice_scroll = lv_obj_create(s_voice_overlay);
        lv_obj_set_pos(s_voice_scroll, 6, 52);
        lv_obj_set_size(s_voice_scroll, 222, 182);
        lv_obj_set_style_pad_all(s_voice_scroll, 4, 0);
        lv_obj_set_style_border_width(s_voice_scroll, 0, 0);
        lv_obj_set_style_bg_color(s_voice_scroll, lv_color_hex(COL_STATUS), 0);
        s_voice_body = lv_label_create(s_voice_scroll);
        lv_obj_set_width(s_voice_body, 210);
        lv_obj_set_style_text_font(s_voice_body, &font_passport_16, 0);
        lv_obj_set_style_text_color(s_voice_body, lv_color_hex(COL_IVORY), 0);
        s_voice_hint = lv_label_create(s_voice_overlay);
        lv_obj_set_size(s_voice_hint, 214, 44);
        lv_obj_set_pos(s_voice_hint, 10, 240);
        lv_obj_set_style_text_font(s_voice_hint, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(s_voice_hint, lv_color_hex(COL_IVORY_DIM), 0);
    }
    lv_obj_remove_flag(s_voice_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(s_voice_phase, phase ? phase : "");
    lv_label_set_text(s_voice_body, body ? body : "");
    lv_label_set_text(s_voice_hint, hint ? hint : "");
    lv_obj_scroll_to_y(s_voice_scroll, 0, LV_ANIM_OFF);
    bsp_lvgl_unlock();
    return true;
}

bool passport_ui_voice_hide(void)
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) return false;
    if (s_voice_overlay) lv_obj_add_flag(s_voice_overlay, LV_OBJ_FLAG_HIDDEN);
    bsp_lvgl_unlock();
    return true;
}

void passport_ui_voice_scroll(int direction)
{
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(100))) return;
    if (s_voice_scroll) lv_obj_scroll_by(s_voice_scroll, 0, direction > 0 ? -72 : 72, LV_ANIM_OFF);
    bsp_lvgl_unlock();
}

bool passport_ui_update_messages(const passport_messages_page_t *messages)
{
    return passport_ui_update_projects(messages);
}
void passport_ui_update_tasks(const passport_tasks_page_t *tasks)
{
    (void)tasks;
}


#else

bool passport_ui_update_voice_messages(const passport_voice_messages_page_t *p) { (void)p; return false; }
bool passport_ui_voice_target(uint8_t id[16], char title[64]) { (void)id; (void)title; return false; }
bool passport_ui_voice_show(const char *p, const char *b, const char *h) { (void)p; (void)b; (void)h; return false; }
bool passport_ui_voice_hide(void) { return true; }
void passport_ui_voice_scroll(int direction) { (void)direction; }

bool passport_ui_take_project_update(void) { return false; }
void passport_ui_show_projects(void) {}
uint8_t passport_ui_project_page(void) { return 0; }
bool passport_ui_update_projects(const passport_projects_page_t *p) { (void)p; return false; }
bool passport_ui_update_messages(const passport_messages_page_t *m) { (void)m; return false; }
void passport_ui_set_sync_error(bool error) { (void)error; }
void passport_ui_update_tasks(const passport_tasks_page_t *p) { (void)p; }
esp_err_t passport_ui_init(void) { return 0; }
void passport_ui_next_page(void) {}
void passport_ui_prev_page(void) {}
void passport_ui_toggle_qr(void) {}
void passport_ui_next_item(void) {}
void passport_ui_prev_item(void) {}
void passport_ui_update_profile(const passport_profile_t *profile) { (void)profile; }
void passport_ui_update_stats(const passport_stats_t *stats) { (void)stats; }
void passport_ui_update_heatmap(const passport_heatmap_t *heatmap) { (void)heatmap; }
void passport_ui_update_footprints(const passport_footprints_t *footprints) { (void)footprints; }
void passport_ui_update_directions(const passport_directions_t *directions) { (void)directions; }
void passport_ui_update_quota(const passport_quota_t *quota) { (void)quota; }
void passport_ui_update_realtime(const passport_realtime_t *realtime) { (void)realtime; }
void passport_ui_set_ble_connected(bool connected) { (void)connected; }
void passport_ui_update_battery(int percent, bool is_charging) { (void)percent; (void)is_charging; }
void passport_ui_update_settings(const passport_settings_t *settings) { (void)settings; }
bool passport_ui_is_settings_page(void) { return false; }
bool passport_ui_is_messages_page(void) { return false; }
void passport_ui_settings_next(void) {}
void passport_ui_settings_prev(void) {}
void passport_ui_settings_toggle(void) {}

#endif
