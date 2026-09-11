// main/xiaozhi_state.h
// Xiaozhi AI Companion state machine definitions.
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XIAOZHI_STATE_BOOT = 0,
    XIAOZHI_STATE_WIFI_CONNECTING,
    XIAOZHI_STATE_IDLE,
    XIAOZHI_STATE_LISTENING,
    XIAOZHI_STATE_THINKING,
    XIAOZHI_STATE_SPEAKING,
    XIAOZHI_STATE_ERROR,
} xiaozhi_state_t;

typedef enum {
    XIAOZHI_EVT_NONE = 0,
    XIAOZHI_EVT_WIFI_CONNECTED,
    XIAOZHI_EVT_WIFI_DISCONNECTED,
    XIAOZHI_EVT_BTN_OK_PRESS,
    XIAOZHI_EVT_BTN_OK_RELEASE,
    XIAOZHI_EVT_BTN_OK_CLICK,
    XIAOZHI_EVT_BTN_OK_LONG,
    XIAOZHI_EVT_BTN_UP_CLICK,
    XIAOZHI_EVT_BTN_DOWN_CLICK,
    XIAOZHI_EVT_QUERY_START,
    XIAOZHI_EVT_QUERY_TOKEN,
    XIAOZHI_EVT_QUERY_DONE,
    XIAOZHI_EVT_QUERY_ERROR,
    XIAOZHI_EVT_SPEECH_PLAY_DONE,
    XIAOZHI_EVT_RESET,
} xiaozhi_event_type_t;

typedef struct {
    xiaozhi_event_type_t type;
    const char *str_payload;
    int int_payload;
} xiaozhi_event_t;

typedef void (*xiaozhi_state_change_cb_t)(xiaozhi_state_t old_state, xiaozhi_state_t new_state, void *user_data);

typedef struct {
    xiaozhi_state_t current_state;
    xiaozhi_state_change_cb_t on_state_change;
    void *user_data;
    bool wifi_connected;
    bool has_api_key;
    char last_error[128];
    uint32_t prompt_index;
} xiaozhi_state_machine_t;

/**
 * @brief Convert state enum to human-readable string.
 */
const char *xiaozhi_state_to_str(xiaozhi_state_t state);

/**
 * @brief Initialize state machine.
 */
void xiaozhi_sm_init(xiaozhi_state_machine_t *sm,
                     bool wifi_connected,
                     bool has_api_key,
                     xiaozhi_state_change_cb_t on_state_change,
                     void *user_data);

/**
 * @brief Step state machine with an incoming event.
 * @return New state after transition.
 */
xiaozhi_state_t xiaozhi_sm_step(xiaozhi_state_machine_t *sm, const xiaozhi_event_t *event);

/**
 * @brief Helper to get preset prompt text by index.
 */
const char *xiaozhi_get_preset_prompt(uint32_t index);

/**
 * @brief Total number of built-in preset conversation prompts.
 */
uint32_t xiaozhi_get_preset_prompt_count(void);

#ifdef __cplusplus
}
#endif
