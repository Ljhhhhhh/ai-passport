// tests/test_xiaozhi_state.c
// Host unit test for Xiaozhi Companion State Machine.
#include "../main/xiaozhi_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    xiaozhi_state_t last_old_state;
    xiaozhi_state_t last_new_state;
    int state_change_count;
} state_test_ctx_t;

static void on_test_state_change(xiaozhi_state_t old_state, xiaozhi_state_t new_state, void *user_data)
{
    state_test_ctx_t *ctx = (state_test_ctx_t *)user_data;
    ctx->last_old_state = old_state;
    ctx->last_new_state = new_state;
    ctx->state_change_count++;
}

static void test_initialization(void)
{
    printf("[TEST] Running test_initialization...\n");
    xiaozhi_state_machine_t sm;
    state_test_ctx_t ctx = {0};

    // Case 1: Wi-Fi connected and API key present -> IDLE
    xiaozhi_sm_init(&sm, true, true, on_test_state_change, &ctx);
    assert(sm.current_state == XIAOZHI_STATE_IDLE);

    // Case 2: Wi-Fi disconnected and API key present -> WIFI_CONNECTING
    xiaozhi_sm_init(&sm, false, true, on_test_state_change, &ctx);
    assert(sm.current_state == XIAOZHI_STATE_WIFI_CONNECTING);

    // Case 3: No API key -> ERROR
    xiaozhi_sm_init(&sm, true, false, on_test_state_change, &ctx);
    assert(sm.current_state == XIAOZHI_STATE_ERROR);

    printf("[TEST] test_initialization PASSED\n");
}

static void test_wifi_transitions(void)
{
    printf("[TEST] Running test_wifi_transitions...\n");
    xiaozhi_state_machine_t sm;
    state_test_ctx_t ctx = {0};

    xiaozhi_sm_init(&sm, false, true, on_test_state_change, &ctx);
    assert(sm.current_state == XIAOZHI_STATE_WIFI_CONNECTING);

    // Connect Wi-Fi
    xiaozhi_event_t evt_connect = { .type = XIAOZHI_EVT_WIFI_CONNECTED };
    xiaozhi_sm_step(&sm, &evt_connect);
    assert(sm.current_state == XIAOZHI_STATE_IDLE);
    assert(ctx.last_new_state == XIAOZHI_STATE_IDLE);

    // Disconnect Wi-Fi
    xiaozhi_event_t evt_disconnect = { .type = XIAOZHI_EVT_WIFI_DISCONNECTED };
    xiaozhi_sm_step(&sm, &evt_disconnect);
    assert(sm.current_state == XIAOZHI_STATE_WIFI_CONNECTING);

    printf("[TEST] test_wifi_transitions PASSED\n");
}

static void test_prompt_cycling(void)
{
    printf("[TEST] Running test_prompt_cycling...\n");
    xiaozhi_state_machine_t sm;
    state_test_ctx_t ctx = {0};

    xiaozhi_sm_init(&sm, true, true, on_test_state_change, &ctx);
    assert(sm.prompt_index == 0);
    uint32_t count = xiaozhi_get_preset_prompt_count();
    assert(count > 0);

    // DOWN button cycles forward
    xiaozhi_event_t evt_down = { .type = XIAOZHI_EVT_BTN_DOWN_CLICK };
    xiaozhi_sm_step(&sm, &evt_down);
    assert(sm.prompt_index == 1);

    // UP button cycles backward
    xiaozhi_event_t evt_up = { .type = XIAOZHI_EVT_BTN_UP_CLICK };
    xiaozhi_sm_step(&sm, &evt_up);
    assert(sm.prompt_index == 0);

    // UP from 0 wraps to end
    xiaozhi_sm_step(&sm, &evt_up);
    assert(sm.prompt_index == count - 1);

    printf("[TEST] test_prompt_cycling PASSED\n");
}

static void test_chat_query_flow(void)
{
    printf("[TEST] Running test_chat_query_flow...\n");
    xiaozhi_state_machine_t sm;
    state_test_ctx_t ctx = {0};

    xiaozhi_sm_init(&sm, true, true, on_test_state_change, &ctx);
    assert(sm.current_state == XIAOZHI_STATE_IDLE);

    // Press OK to start query
    xiaozhi_event_t evt_ok = { .type = XIAOZHI_EVT_BTN_OK_CLICK };
    xiaozhi_sm_step(&sm, &evt_ok);
    assert(sm.current_state == XIAOZHI_STATE_THINKING);

    // First token arrives -> SPEAKING
    xiaozhi_event_t evt_tok = { .type = XIAOZHI_EVT_QUERY_TOKEN };
    xiaozhi_sm_step(&sm, &evt_tok);
    assert(sm.current_state == XIAOZHI_STATE_SPEAKING);

    // Query done -> IDLE
    xiaozhi_event_t evt_done = { .type = XIAOZHI_EVT_QUERY_DONE };
    xiaozhi_sm_step(&sm, &evt_done);
    assert(sm.current_state == XIAOZHI_STATE_IDLE);

    printf("[TEST] test_chat_query_flow PASSED\n");
}
static void test_push_to_talk(void)
{
    printf("[TEST] Running test_push_to_talk...\n");
    xiaozhi_state_machine_t sm;
    state_test_ctx_t ctx = {0};

    xiaozhi_sm_init(&sm, true, true, on_test_state_change, &ctx);
    assert(sm.current_state == XIAOZHI_STATE_IDLE);

    // 1. Press OK -> enters LISTENING
    xiaozhi_event_t evt_press = { .type = XIAOZHI_EVT_BTN_OK_PRESS };
    xiaozhi_sm_step(&sm, &evt_press);
    assert(sm.current_state == XIAOZHI_STATE_LISTENING);

    // 2. Release OK -> enters THINKING
    xiaozhi_event_t evt_release = { .type = XIAOZHI_EVT_BTN_OK_RELEASE };
    xiaozhi_sm_step(&sm, &evt_release);
    assert(sm.current_state == XIAOZHI_STATE_THINKING);

    // 3. Token arrives -> SPEAKING
    xiaozhi_event_t evt_tok = { .type = XIAOZHI_EVT_QUERY_TOKEN };
    xiaozhi_sm_step(&sm, &evt_tok);
    assert(sm.current_state == XIAOZHI_STATE_SPEAKING);

    // 4. Finished -> IDLE
    xiaozhi_event_t evt_done = { .type = XIAOZHI_EVT_QUERY_DONE };
    xiaozhi_sm_step(&sm, &evt_done);
    assert(sm.current_state == XIAOZHI_STATE_IDLE);

    printf("[TEST] test_push_to_talk PASSED\n");
}


static void test_error_and_recovery(void)
{
    printf("[TEST] Running test_error_and_recovery...\n");
    xiaozhi_state_machine_t sm;
    state_test_ctx_t ctx = {0};

    xiaozhi_sm_init(&sm, true, true, on_test_state_change, &ctx);

    // Query error event
    xiaozhi_event_t evt_err = { .type = XIAOZHI_EVT_QUERY_ERROR, .str_payload = "401 Unauthorized" };
    xiaozhi_sm_step(&sm, &evt_err);
    assert(sm.current_state == XIAOZHI_STATE_ERROR);
    assert(strcmp(sm.last_error, "401 Unauthorized") == 0);

    // Pressing OK button clears error and recovers to IDLE
    xiaozhi_event_t evt_ok = { .type = XIAOZHI_EVT_BTN_OK_CLICK };
    xiaozhi_sm_step(&sm, &evt_ok);
    assert(sm.current_state == XIAOZHI_STATE_IDLE);

    printf("[TEST] test_error_and_recovery PASSED\n");
}

int main(void)
{
    printf("========================================\n");
    printf("  Running Xiaozhi State Machine Tests\n");
    printf("========================================\n");

    test_initialization();
    test_wifi_transitions();
    test_prompt_cycling();
    test_chat_query_flow();
    test_push_to_talk();
    test_error_and_recovery();

    printf("ALL XIAOZHI STATE MACHINE TESTS PASSED!\n");
    return 0;
}
