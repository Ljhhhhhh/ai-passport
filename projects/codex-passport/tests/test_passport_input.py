"""Exercise the actual input task with queued events and mocked voice/UI services."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

MAIN = Path(__file__).resolve().parents[1] / "main"


class InputTests(unittest.TestCase):
    def test_wake_notice_and_voice_routing(self):
        source = (MAIN / "main.c").read_text()
        task = re.search(r"^static void ui_input_task\(void \*arg\)\n\{.*?^\}",
                         source, re.M | re.S)
        self.assertIsNotNone(task)
        message = re.search(r"typedef struct \{[^}]+\} btn_msg_t;", source)
        self.assertIsNotNone(message)
        program = r'''
#include "passport_idle.h"
#include "passport_voice.h"
#include <assert.h>
#include <setjmp.h>
#include <string.h>
typedef enum { BSP_BTN_UP, BSP_BTN_DOWN, BSP_BTN_OK } bsp_btn_t;
typedef enum { BSP_BTN_PRESS, BSP_BTN_CLICK, BSP_BTN_DOUBLE, BSP_BTN_LONG,
               BSP_BTN_RELEASE } bsp_btn_ev_t;
typedef unsigned TickType_t;
#define pdMS_TO_TICKS(x) (x)
#define portTICK_PERIOD_MS 1
#define pdTRUE 1
''' + message.group() + r'''
static passport_idle_t s_idle;
static int s_buttons;
static jmp_buf finished;
static unsigned step, starts, notices, hides, stops, confirms, cancels;
static passport_voice_state_t state;
static bool start_ok, hide_ok;
static uint32_t confirmed_token;
static TickType_t xTaskGetTickCount(void) { return step; }
static void apply_idle(passport_idle_act_t a) { (void)a; }
static uint32_t passport_ble_unread_count(void) { return 0; }
static bool passport_ui_take_project_update(void) { return false; }
static void passport_ui_show_projects(void) { assert(0); }
bool passport_voice_busy(void) { return state != PASSPORT_VOICE_IDLE; }
passport_voice_state_t passport_voice_get_state(void) { return state; }
void passport_voice_stop(void) { ++stops; }
void passport_voice_cancel(void) { ++cancels; }
void passport_voice_confirm(uint32_t token) { ++confirms; confirmed_token = token; }
bool passport_voice_start(const uint8_t id[16], const char *title) {
    assert(id[0] == 42 && !strcmp(title, "Pinned task"));
    ++starts;
    if (start_ok) state = PASSPORT_VOICE_PREPARING;
    return start_ok;
}
static bool passport_ui_is_messages_page(void) { return true; }
static bool passport_ui_is_settings_page(void) { return false; }
static bool passport_ui_voice_target(uint8_t id[16], char title[64]) {
    memset(id, 0, 16); id[0] = 42; strcpy(title, "Pinned task"); return true;
}
bool passport_ui_voice_show(const char *phase, const char *body, const char *hint) {
    assert(!strcmp(phase, "Voice unavailable"));
    assert(body[0] && !strcmp(hint, "OK: close")); ++notices; return true;
}
bool passport_ui_voice_hide(void) { ++hides; return hide_ok; }
static void passport_ui_voice_scroll(int d) { (void)d; assert(0); }
static void passport_ui_toggle_qr(void) { assert(0); }
static void passport_ui_settings_toggle(void) { assert(0); }
static void passport_ui_prev_item(void) { assert(0); }
static void passport_ui_next_item(void) { assert(0); }
static int xQueueReceive(int queue, btn_msg_t *msg, unsigned timeout) {
    (void)queue; (void)timeout;
    *msg = (btn_msg_t){.btn = BSP_BTN_OK, .ev = BSP_BTN_PRESS};
    switch (step++) {
    case 0: assert(!s_idle.awake); break;
    case 1: assert(s_idle.awake); break;
    case 2: msg->ev = BSP_BTN_DOUBLE; break;
    case 3: assert(starts == 0); break; /* Entire wake double was consumed. */
    case 4: break;
    case 5: start_ok = true; msg->ev = BSP_BTN_DOUBLE; break;
    case 6: assert(starts == 1 && state == PASSPORT_VOICE_PREPARING);
            state = PASSPORT_VOICE_IDLE; start_ok = false; break;
    case 7: break;
    case 8: msg->ev = BSP_BTN_DOUBLE; break;
    case 9: assert(starts == 2 && notices == 1); break;
    case 10: hide_ok = false; msg->ev = BSP_BTN_CLICK; break;
    case 11: assert(hides == 1); break;
    case 12: break;
    case 13: msg->ev = BSP_BTN_DOUBLE; break;
    case 14: assert(starts == 2 && notices == 1); break; /* Failed hide retains ownership. */
    case 15: hide_ok = true; msg->ev = BSP_BTN_CLICK; break;
    case 16: assert(hides == 2); break;
    case 17: break;
    case 18: start_ok = true; msg->ev = BSP_BTN_DOUBLE; break;
    case 19: assert(starts == 3); state = PASSPORT_VOICE_RECORDING; break;
    case 20: msg->ev = BSP_BTN_CLICK; break;
    case 21: assert(stops == 1 && confirms == 0); break;
    case 22: state = PASSPORT_VOICE_REVIEW; msg->ev = BSP_BTN_CLICK;
             msg->pressed_voice_rid = 0; break; /* Stop press crosses into review. */
    case 23: assert(stops == 1 && confirms == 1 && confirmed_token == 0); break;
    case 24: msg->ev = BSP_BTN_CLICK; msg->pressed_voice_rid = 42; break;
    case 25: assert(confirms == 2 && confirmed_token == 42); break;
    case 26: msg->ev = BSP_BTN_LONG; break;
    default: assert(cancels == 1 && starts == 3 && notices == 1 && hides == 2);
             longjmp(finished, 1);
    }
    return pdTRUE;
}
''' + task.group() + r'''
int main(void) {
    s_idle = (passport_idle_t){.awake = 0, .unread_count = 0};
    if (!setjmp(finished)) ui_input_task(NULL);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as folder:
            binary = str(Path(folder) / "input")
            subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
                            "-Werror", "-I", str(MAIN), "-x", "c", "-",
                            str(MAIN / "passport_idle.c"), "-o", binary],
                           input=program, text=True, check=True)
            subprocess.run([binary], check=True, timeout=5)


if __name__ == "__main__":
    unittest.main()
