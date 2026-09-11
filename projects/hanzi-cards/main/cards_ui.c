#include "cards_ui.h"

#include "lvgl.h"

#include <string.h>

LV_FONT_DECLARE(font_cards_88);
LV_FONT_DECLARE(font_cards_22);

#define COLOR_CREAM 0xF6EBD8
#define COLOR_INK   0x3C2F23
#define COLOR_PHRASE 0x6B5344
#define IMAGE_W 200
#define IMAGE_H 164
#define IMAGE_STRIDE 100
#define IMAGE_PALETTE 64U

static lv_obj_t *s_image;
static lv_obj_t *s_hanzi;
static lv_obj_t *s_phrase;
static lv_image_dsc_t s_dsc;

static void apply_image(const cards_entry_t *entry)
{
    memset(&s_dsc, 0, sizeof(s_dsc));
    if (!entry || !entry->image || entry->image_len < IMAGE_PALETTE + (IMAGE_STRIDE * IMAGE_H)) {
        lv_image_set_src(s_image, NULL);
        return;
    }
    s_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_dsc.header.cf = LV_COLOR_FORMAT_I4;
    s_dsc.header.w = IMAGE_W;
    s_dsc.header.h = IMAGE_H;
    s_dsc.header.stride = IMAGE_STRIDE;
    s_dsc.data_size = (uint32_t)entry->image_len;
    s_dsc.data = entry->image;
    lv_image_set_src(s_image, &s_dsc);
}

void cards_ui_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(COLOR_CREAM), 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_radius(screen, 0, 0);

    s_image = lv_image_create(screen);
    lv_obj_set_pos(s_image, 20, 16);
    lv_obj_set_size(s_image, IMAGE_W, IMAGE_H);
    lv_obj_set_style_border_width(s_image, 0, 0);
    lv_obj_set_style_pad_all(s_image, 0, 0);
    lv_obj_set_style_bg_opa(s_image, LV_OPA_TRANSP, 0);

    s_hanzi = lv_label_create(screen);
    lv_obj_set_style_text_font(s_hanzi, &font_cards_88, 0);
    lv_obj_set_style_text_color(s_hanzi, lv_color_hex(COLOR_INK), 0);
    lv_obj_set_style_text_align(s_hanzi, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_hanzi, 240);
    lv_obj_set_pos(s_hanzi, 0, 188);

    s_phrase = lv_label_create(screen);
    lv_obj_set_style_text_font(s_phrase, &font_cards_22, 0);
    lv_obj_set_style_text_color(s_phrase, lv_color_hex(COLOR_PHRASE), 0);
    lv_obj_set_style_text_align(s_phrase, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_phrase, 240);
    lv_obj_set_pos(s_phrase, 0, 284);

    lv_screen_load(screen);
}

void cards_ui_render(const cards_entry_t *entry)
{
    apply_image(entry);
    lv_label_set_text(s_hanzi, (entry && entry->hanzi) ? entry->hanzi : "");
    lv_label_set_text(s_phrase, (entry && entry->phrase) ? entry->phrase : "");
}
