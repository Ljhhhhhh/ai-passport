#include "hanzi_ui.h"

#include "lvgl.h"

#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(font_hanzi_80);
LV_FONT_DECLARE(font_hanzi_24);

#define COLOR_SKY       0x7EC8E3
#define COLOR_GROUND    0x8FCB5A
#define COLOR_PAPER     0xFFF8EE
#define COLOR_NIGHT     0x1A2238
#define COLOR_INK       0x2B241C
#define COLOR_WHITE     0xFFFFFF
#define COLOR_SUN       0xFFD166
#define COLOR_MOUNTAIN  0x5A7D5A
#define COLOR_RIVER     0x2980B9
#define COLOR_CHAR_BODY 0xE67E22
#define COLOR_CHAR_FACE 0xF5CBA7
#define COLOR_CARD_BG   0xFFFFFF
#define COLOR_CARD_SEL  0xFFEAA7
#define COLOR_CARD_BOR  0xD35400

static lv_obj_t *s_screen;
static lv_obj_t *s_title_label;
static lv_obj_t *s_stage;
static lv_obj_t *s_focus_label;
static lv_obj_t *s_prompt_label;

/* Direct 2-choice card objects */
static lv_obj_t *s_card_top;
static lv_obj_t *s_card_top_badge;
static lv_obj_t *s_card_top_badge_label;
static lv_obj_t *s_card_top_label;

static lv_obj_t *s_card_bot;
static lv_obj_t *s_card_bot_badge;
static lv_obj_t *s_card_bot_badge_label;
static lv_obj_t *s_card_bot_label;

/* Bottom hints */
static lv_obj_t *s_hint_bar;
static lv_obj_t *s_hint_up;
static lv_obj_t *s_hint_down;
static lv_obj_t *s_hint_ok;

static lv_obj_t *block(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, int radius)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    return obj;
}

static lv_obj_t *label_create(lv_obj_t *parent, const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_label_set_text(label, "");
    return label;
}

static void draw_world_scene(lv_obj_t *stage, const hanzi_view_t *view)
{
    lv_obj_clean(stage);

    if (view->detached_mode) {
        return; /* Detached mode uses clean cards */
    }

    if (view->state == HANZI_STATE_DAY_COMPLETE && !view->world_has_river && view->current_day == 1) {
        /* Restful bedtime scene */
        block(stage, 180, 20, 32, 32, COLOR_SUN, 16);
        return;
    }

    /* Sun */
    block(stage, 184, 16, 28, 28, COLOR_SUN, 14);

    /* Mountain (大 / 小 / 正常) */
    if (view->world_mountain_size == 1) {
        /* Big Mountain */
        block(stage, 80, 28, 120, 100, COLOR_MOUNTAIN, 8);
        block(stage, 100, 16, 80, 30, COLOR_MOUNTAIN, 6);
    } else if (view->world_mountain_size == 2) {
        /* Small Mountain */
        block(stage, 130, 72, 60, 56, COLOR_MOUNTAIN, 6);
    } else {
        /* Normal Mountain */
        block(stage, 110, 50, 90, 78, COLOR_MOUNTAIN, 6);
        block(stage, 130, 38, 50, 24, COLOR_MOUNTAIN, 4);
    }

    /* Ground */
    block(stage, 0, 120, 240, 50, COLOR_GROUND, 0);

    /* River (when unlocked) */
    if (view->world_has_river) {
        block(stage, 0, 136, 240, 26, COLOR_RIVER, 4);
        /* Water ripples */
        block(stage, 40, 142, 36, 4, COLOR_WHITE, 2);
        block(stage, 130, 148, 48, 4, COLOR_WHITE, 2);
    }

    /* Little Guy Character (正常 / 大 / 小 / 跳) */
    int char_x = 36;
    int char_y = 74;

    if (view->character_state == 1) {
        /* Big Character */
        char_x = 24;
        char_y = 52;
        block(stage, char_x + 12, char_y, 24, 24, COLOR_CHAR_FACE, 12); /* Head */
        block(stage, char_x, char_y + 26, 48, 44, COLOR_CHAR_BODY, 6);  /* Body */
        block(stage, char_x + 8, char_y + 70, 12, 18, COLOR_INK, 2);    /* Leg L */
        block(stage, char_x + 28, char_y + 70, 12, 18, COLOR_INK, 2);   /* Leg R */
    } else if (view->character_state == 2) {
        /* Small Character */
        char_x = 44;
        char_y = 96;
        block(stage, char_x + 6, char_y, 14, 14, COLOR_CHAR_FACE, 7);  /* Head */
        block(stage, char_x, char_y + 15, 26, 24, COLOR_CHAR_BODY, 4);  /* Body */
        block(stage, char_x + 4, char_y + 39, 6, 10, COLOR_INK, 1);    /* Leg L */
        block(stage, char_x + 16, char_y + 39, 6, 10, COLOR_INK, 1);   /* Leg R */
    } else if (view->character_state == 3) {
        /* Jumping Character */
        char_x = 36;
        char_y = 42; /* Air */
        block(stage, char_x + 8, char_y, 18, 18, COLOR_CHAR_FACE, 9);
        block(stage, char_x + 2, char_y + 20, 30, 32, COLOR_CHAR_BODY, 5);
        block(stage, char_x + 6, char_y + 52, 8, 12, COLOR_INK, 2);
        block(stage, char_x + 20, char_y + 52, 8, 12, COLOR_INK, 2);
    } else {
        /* Normal Character */
        block(stage, char_x + 8, char_y, 18, 18, COLOR_CHAR_FACE, 9);   /* Head */
        block(stage, char_x + 2, char_y + 20, 30, 32, COLOR_CHAR_BODY, 5); /* Body */
        block(stage, char_x + 6, char_y + 52, 8, 14, COLOR_INK, 2);    /* Leg L */
        block(stage, char_x + 20, char_y + 52, 8, 14, COLOR_INK, 2);   /* Leg R */
    }
}

void hanzi_ui_create(void)
{
    s_screen = lv_obj_create(NULL);
    lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(COLOR_SKY), 0);
    lv_obj_set_style_border_width(s_screen, 0, 0);
    lv_obj_set_style_pad_all(s_screen, 0, 0);

    /* Header title */
    s_title_label = label_create(s_screen, &font_hanzi_24, COLOR_INK);
    lv_obj_set_pos(s_title_label, 16, 12);

    /* World stage */
    s_stage = lv_obj_create(s_screen);
    lv_obj_remove_flag(s_stage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_stage, 0, 44);
    lv_obj_set_size(s_stage, 240, 170);
    lv_obj_set_style_bg_opa(s_stage, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_stage, 0, 0);
    lv_obj_set_style_pad_all(s_stage, 0, 0);

    /* Focus large Hanzi */
    s_focus_label = label_create(s_screen, &font_hanzi_80, COLOR_INK);
    lv_obj_set_pos(s_focus_label, 80, 72);

    /* Prompt label */
    s_prompt_label = label_create(s_screen, &font_hanzi_24, COLOR_INK);
    lv_obj_set_pos(s_prompt_label, 16, 230);

    /* Direct 2-Choice Cards */
    /* Top Card */
    s_card_top = lv_obj_create(s_screen);
    lv_obj_set_pos(s_card_top, 20, 50);
    lv_obj_set_size(s_card_top, 200, 96);
    lv_obj_set_style_radius(s_card_top, 12, 0);
    lv_obj_set_style_bg_color(s_card_top, lv_color_hex(COLOR_CARD_BG), 0);
    lv_obj_set_style_border_width(s_card_top, 3, 0);
    lv_obj_set_style_border_color(s_card_top, lv_color_hex(COLOR_SKY), 0);
    lv_obj_set_style_pad_all(s_card_top, 0, 0);
    lv_obj_remove_flag(s_card_top, LV_OBJ_FLAG_SCROLLABLE);

    s_card_top_badge = block(s_card_top, 8, 8, 48, 24, COLOR_SKY, 6);
    s_card_top_badge_label = label_create(s_card_top_badge, &font_hanzi_24, COLOR_WHITE);
    lv_label_set_text(s_card_top_badge_label, "▲上");
    lv_obj_center(s_card_top_badge_label);

    s_card_top_label = label_create(s_card_top, &font_hanzi_80, COLOR_INK);
    lv_obj_center(s_card_top_label);

    /* Bottom Card */
    s_card_bot = lv_obj_create(s_screen);
    lv_obj_set_pos(s_card_bot, 20, 156);
    lv_obj_set_size(s_card_bot, 200, 96);
    lv_obj_set_style_radius(s_card_bot, 12, 0);
    lv_obj_set_style_bg_color(s_card_bot, lv_color_hex(COLOR_CARD_BG), 0);
    lv_obj_set_style_border_width(s_card_bot, 3, 0);
    lv_obj_set_style_border_color(s_card_bot, lv_color_hex(COLOR_GROUND), 0);
    lv_obj_set_style_pad_all(s_card_bot, 0, 0);
    lv_obj_remove_flag(s_card_bot, LV_OBJ_FLAG_SCROLLABLE);

    s_card_bot_badge = block(s_card_bot, 8, 8, 48, 24, COLOR_GROUND, 6);
    s_card_bot_badge_label = label_create(s_card_bot_badge, &font_hanzi_24, COLOR_WHITE);
    lv_label_set_text(s_card_bot_badge_label, "▼下");
    lv_obj_center(s_card_bot_badge_label);

    s_card_bot_label = label_create(s_card_bot, &font_hanzi_80, COLOR_INK);
    lv_obj_center(s_card_bot_label);

    /* Bottom hint bar */
    s_hint_bar = lv_obj_create(s_screen);
    lv_obj_set_pos(s_hint_bar, 0, 280);
    lv_obj_set_size(s_hint_bar, 240, 40);
    lv_obj_set_style_bg_opa(s_hint_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_hint_bar, 0, 0);
    lv_obj_set_style_pad_all(s_hint_bar, 0, 0);

    s_hint_up = block(s_hint_bar, 16, 6, 44, 28, COLOR_INK, 6);
    lv_obj_t *l_up = label_create(s_hint_up, &font_hanzi_24, COLOR_WHITE);
    lv_label_set_text(l_up, "▲");
    lv_obj_center(l_up);

    s_hint_down = block(s_hint_bar, 68, 6, 44, 28, COLOR_INK, 6);
    lv_obj_t *l_dn = label_create(s_hint_down, &font_hanzi_24, COLOR_WHITE);
    lv_label_set_text(l_dn, "▼");
    lv_obj_center(l_dn);

    s_hint_ok = block(s_hint_bar, 160, 6, 64, 28, COLOR_INK, 6);
    lv_obj_t *l_ok = label_create(s_hint_ok, &font_hanzi_24, COLOR_WHITE);
    lv_label_set_text(l_ok, "OK");
    lv_obj_center(l_ok);

    lv_screen_load(s_screen);
}

void hanzi_ui_render(const hanzi_view_t *view)
{
    if (!view) {
        return;
    }

    /* Screen background style */
    if (view->detached_mode) {
        lv_obj_set_style_bg_color(s_screen, lv_color_hex(COLOR_PAPER), 0);
    } else if (view->state == HANZI_STATE_DAY_COMPLETE && !view->world_has_river && view->current_day == 1) {
        lv_obj_set_style_bg_color(s_screen, lv_color_hex(COLOR_NIGHT), 0);
    } else {
        lv_obj_set_style_bg_color(s_screen, lv_color_hex(COLOR_SKY), 0);
    }

    /* Title */
    char title_buf[64];
    if (view->sandbox_active) {
        snprintf(title_buf, sizeof(title_buf), "自由小世界 (%ds)", (int)view->sandbox_seconds_left);
        lv_label_set_text(s_title_label, title_buf);
    } else {
        lv_label_set_text(s_title_label, view->title ? view->title : "");
    }

    /* Prompt */
    lv_label_set_text(s_prompt_label, view->prompt ? view->prompt : "");

    /* Draw World Scene */
    draw_world_scene(s_stage, view);

    /* Focus Hanzi */
    if (view->focus_hanzi && !view->show_choices) {
        lv_label_set_text(s_focus_label, view->focus_hanzi);
        lv_obj_remove_flag(s_focus_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_center(s_focus_label);
    } else {
        lv_obj_add_flag(s_focus_label, LV_OBJ_FLAG_HIDDEN);
    }

    /* Direct 2-Choice Cards */
    if (view->show_choices) {
        lv_obj_remove_flag(s_card_top, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_card_bot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_stage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_prompt_label, LV_OBJ_FLAG_HIDDEN);

        lv_label_set_text(s_card_top_label, view->choice_top ? view->choice_top : "");
        lv_label_set_text(s_card_bot_label, view->choice_bottom ? view->choice_bottom : "");

        /* Selected feedback */
        if (view->selected_choice == 1) {
            lv_obj_set_style_bg_color(s_card_top, lv_color_hex(COLOR_CARD_SEL), 0);
            lv_obj_set_style_border_color(s_card_top, lv_color_hex(COLOR_CARD_BOR), 0);
        } else {
            lv_obj_set_style_bg_color(s_card_top, lv_color_hex(COLOR_CARD_BG), 0);
            lv_obj_set_style_border_color(s_card_top, lv_color_hex(COLOR_SKY), 0);
        }

        if (view->selected_choice == 2) {
            lv_obj_set_style_bg_color(s_card_bot, lv_color_hex(COLOR_CARD_SEL), 0);
            lv_obj_set_style_border_color(s_card_bot, lv_color_hex(COLOR_CARD_BOR), 0);
        } else {
            lv_obj_set_style_bg_color(s_card_bot, lv_color_hex(COLOR_CARD_BG), 0);
            lv_obj_set_style_border_color(s_card_bot, lv_color_hex(COLOR_GROUND), 0);
        }
    } else {
        lv_obj_add_flag(s_card_top, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_card_bot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_stage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_prompt_label, LV_OBJ_FLAG_HIDDEN);
    }

    /* Button Hints Bar */
    if (view->show_choice_hint) {
        lv_obj_set_style_opa(s_hint_up, LV_OPA_COVER, 0);
        lv_obj_set_style_opa(s_hint_down, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_opa(s_hint_up, LV_OPA_40, 0);
        lv_obj_set_style_opa(s_hint_down, LV_OPA_40, 0);
    }

    if (view->show_ok_hint) {
        lv_obj_set_style_opa(s_hint_ok, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_opa(s_hint_ok, LV_OPA_40, 0);
    }
}
