"""Run the firmware navigation functions on the host, without LVGL rendering."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

MAIN = Path(__file__).resolve().parents[1] / "main"


class NavigationTests(unittest.TestCase):
    def test_cards_pages_repeats_and_shrinking_snapshots(self):
        source = (MAIN / "passport_ui.c").read_text()

        def function(name):
            match = re.search(r"^(?:static )?(?:bool|void) " + name +
                              r"\([^\n]*\)\n\{.*?^\}", source, re.M | re.S)
            self.assertIsNotNone(match, name)
            return match.group()

        # Keep the real render state updates; only omit LVGL card painting.
        render = function("render_projects_locked").split("    for (")[0] + "}\n"
        state = source[source.index("static atomic_bool s_project_updated;"):
                       source.index("static lv_obj_t *s_voice_overlay")]
        program = '''#include "passport_ui.h"
#include <assert.h>
#include <string.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdarg.h>
#define pdMS_TO_TICKS(x) (x)
#define COL_STATUS 0
#define COL_GOLD 1
#define COL_IDLE 2
typedef struct { char text[128]; int border; } lv_obj_t;
static lv_obj_t subtitle, cards[3];
static lv_obj_t *s_lbl_proj_title = &subtitle, *s_lbl_ble;
static lv_obj_t *s_box_proj[] = {&cards[0], &cards[1], &cards[2]};
static bool s_sync_error;
static int lv_color_hex(int color) { return color; }
static void lv_obj_set_style_border_width(lv_obj_t *o, int w, int s) { (void)s; o->border = w; }
static void lv_obj_set_style_bg_color(lv_obj_t *o, int c, int s) { (void)o; (void)c; (void)s; }
static void lv_obj_set_style_text_color(lv_obj_t *o, int c, int s) { (void)o; (void)c; (void)s; }
static void lv_label_set_text(lv_obj_t *o, const char *s) { snprintf(o->text, sizeof(o->text), "%s", s); }
static void lv_label_set_text_fmt(lv_obj_t *o, const char *fmt, ...) {
    va_list args; va_start(args, fmt); vsnprintf(o->text, sizeof(o->text), fmt, args); va_end(args);
}
static int locked;
static bool bsp_lvgl_lock(int ticks) { (void)ticks; assert(!locked); locked = 1; return true; }
static void bsp_lvgl_unlock(void) { assert(locked); locked = 0; }
static void highlight_message(void);
'''+ state + '''
static void show_page(int page) { s_current_page = page; }
''' + render + "\n".join(function(name) for name in (
            "highlight_message", "message_page_matches", "move_message",
            "passport_ui_next_item", "passport_ui_prev_item", "passport_ui_set_ble_connected",
            "passport_ui_update_projects", "passport_ui_update_voice_messages", "passport_ui_voice_target")) + '''
static void receive(unsigned page, unsigned pages, unsigned count) {
    passport_voice_messages_page_t p = {0};
    p.messages.page_index = page;
    p.messages.total_pages = pages;
    p.messages.count = count;
    for (unsigned i = 0; i < count; ++i) p.thread_ids[i][0] = page * 3 + i + 1;
    assert(passport_ui_update_voice_messages(&p));
    assert(!locked);
}
static void selected(unsigned page, unsigned card) {
    uint8_t id[16]; char title[64];
    assert(s_current_page == PAGE_MESSAGES);
    assert(s_project_page == page && s_selected_message == card);
    assert(passport_ui_voice_target(id, title));
    assert(id[0] == page * 3 + card + 1);
}
int main(void) {
    s_current_page = PAGE_MESSAGES;
    passport_ui_set_ble_connected(true);
    receive(0, 3, 3);
    selected(0, 0);
    assert(!strcmp(subtitle.text, "1/3  2xOK talk"));
    /* Visit every card, including a partial final page. */
    for (unsigned n = 1; n < 8; ++n) {
        passport_ui_next_item();
        if (n % 3 == 0) {
            char loading[32]; snprintf(loading, sizeof(loading), "Loading page %u", n / 3 + 1);
            assert(!strcmp(subtitle.text, loading));
            uint8_t id[16]; char title[64];
            assert(!passport_ui_voice_target(id, title));
            for (int j = 0; j < 10; ++j) {
                passport_ui_next_item(); passport_ui_prev_item();
            }
            assert(s_current_page == PAGE_MESSAGES && s_project_page == n / 3);
            receive(n / 3 - 1, 3, 3); /* Old in-flight snapshot. */
            assert(s_message_page_pending && !s_voice_targets_valid);
            receive(n / 3, 3, n < 6 ? 3 : 2);
        }
        selected(n / 3, n % 3);
    }
    passport_ui_next_item(); assert(s_current_page == PAGE_SETTINGS);
    passport_ui_prev_item(); selected(2, 1);
    for (int n = 6; n >= 0; --n) {
        passport_ui_prev_item();
        if (n % 3 == 2) {
            passport_ui_prev_item(); passport_ui_next_item();
            assert(s_project_page == n / 3);
            receive(n / 3 + 1, 3, 2);
            assert(s_message_page_pending);
            receive(n / 3, 3, 3);
        }
        selected(n / 3, n % 3);
    }
    passport_ui_prev_item(); assert(s_current_page == PAGE_QUOTA);
    passport_ui_next_item(); selected(0, 0);
    /* Requested last page disappears: accept exactly the host's clamp. */
    s_project_page = 2; s_message_page_pending = true; s_voice_targets_valid = false;
    receive(0, 2, 3); assert(s_message_page_pending && s_project_page == 2);
    receive(1, 2, 1); selected(1, 0); assert(!s_message_page_pending);
    passport_ui_prev_item(); receive(0, 1, 2); selected(0, 1);
    passport_ui_prev_item(); selected(0, 0);
    /* Empty list also releases a pending request and permits leaving Messages. */
    s_project_page = 1; s_message_page_pending = true; s_voice_targets_valid = false;
    receive(0, 1, 0);
    assert(!s_message_page_pending && s_project_page == 0);
    passport_ui_prev_item(); assert(s_current_page == PAGE_QUOTA);
    /* Disconnect cancels a pending request; cached cards remain navigable. */
    s_current_page = PAGE_MESSAGES;
    receive(0, 3, 3);
    passport_ui_next_item(); passport_ui_next_item(); passport_ui_next_item();
    assert(s_message_page_pending);
    passport_ui_set_ble_connected(false);
    assert(!s_message_page_pending && s_project_page == 0);
    assert(!strcmp(subtitle.text, "Disconnected"));
    uint8_t id[16]; char title[64];
    assert(!passport_ui_voice_target(id, title));
    passport_ui_next_item(); passport_ui_next_item();
    assert(s_selected_message == 2);
    passport_ui_next_item(); assert(s_current_page == PAGE_SETTINGS);
    passport_ui_prev_item(); passport_ui_prev_item(); passport_ui_prev_item();
    assert(s_selected_message == 0);
    passport_ui_prev_item(); assert(s_current_page == PAGE_QUOTA);
    passport_ui_set_ble_connected(true);
    assert(!passport_ui_voice_target(id, title));
    passport_ui_next_item(); receive(0, 3, 3); selected(0, 0);
    /* Display-only legacy pages navigate cards without granting voice targets. */
    passport_projects_page_t legacy = {.count = 3, .total_pages = 2};
    assert(passport_ui_update_projects(&legacy));
    assert(!passport_ui_voice_target(id, title));
    assert(!strcmp(subtitle.text, "1/2"));
    passport_ui_next_item(); assert(s_selected_message == 1 && cards[1].border == 2);
    passport_ui_next_item(); assert(s_selected_message == 2);
    passport_ui_next_item(); assert(s_message_page_pending && s_project_page == 1);
    assert(passport_ui_update_projects(&legacy)); assert(s_message_page_pending);
    legacy.page_index = 1; legacy.count = 1;
    assert(passport_ui_update_projects(&legacy)); assert(!s_message_page_pending);
    assert(s_selected_message == 0 && !passport_ui_voice_target(id, title));
    passport_ui_prev_item(); assert(s_message_page_pending && s_project_page == 0);
    legacy.page_index = 0; legacy.count = 3;
    assert(passport_ui_update_projects(&legacy)); assert(s_selected_message == 2);
    passport_ui_prev_item(); passport_ui_prev_item(); assert(s_selected_message == 0);
    passport_ui_prev_item(); assert(s_current_page == PAGE_QUOTA);
    /* Cancel an UP page request too; reconnect must not retain prefer-last. */
    s_current_page = PAGE_MESSAGES;
    s_project_page = 1;
    receive(1, 2, 1);
    passport_ui_prev_item(); assert(s_prefer_last_message && s_message_page_pending);
    passport_ui_set_ble_connected(false);
    assert(s_project_page == 1 && !s_prefer_last_message && !s_message_page_pending);
    passport_ui_prev_item(); assert(s_current_page == PAGE_QUOTA);
    passport_ui_set_ble_connected(true);
    passport_ui_next_item(); receive(1, 2, 3); selected(1, 0);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as folder:
            binary = str(Path(folder) / "navigation")
            subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
                            "-Werror", "-I", str(MAIN), "-x", "c", "-", "-o", binary],
                           input=program, text=True, check=True)
            subprocess.run([binary], check=True)


if __name__ == "__main__":
    unittest.main()
