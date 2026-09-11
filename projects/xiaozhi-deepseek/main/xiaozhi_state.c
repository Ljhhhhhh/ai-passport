// main/xiaozhi_state.c
#include "xiaozhi_state.h"
#include <string.h>
#include <stdio.h>

static const char *PRESET_PROMPTS[] = {
    "介绍一下你自己，以及你能帮我做些什么？",
    "讲一个有趣且简短的科学冷知识。",
    "用通俗易懂的一句话解释量子力学。",
    "写一首关于随身AI与旅途的精简小诗。",
    "推荐今天一个能让人心情变好的小习惯。",
    "讲一个充满反转的幽默笑话。"
};
#define PRESET_PROMPTS_COUNT (sizeof(PRESET_PROMPTS) / sizeof(PRESET_PROMPTS[0]))

const char *xiaozhi_state_to_str(xiaozhi_state_t state)
{
    switch (state) {
        case XIAOZHI_STATE_BOOT:            return "BOOT";
        case XIAOZHI_STATE_WIFI_CONNECTING: return "WIFI_CONNECTING";
        case XIAOZHI_STATE_IDLE:            return "IDLE";
        case XIAOZHI_STATE_LISTENING:       return "LISTENING";
        case XIAOZHI_STATE_THINKING:        return "THINKING";
        case XIAOZHI_STATE_SPEAKING:        return "SPEAKING";
        case XIAOZHI_STATE_ERROR:           return "ERROR";
        default:                            return "UNKNOWN";
    }
}

uint32_t xiaozhi_get_preset_prompt_count(void)
{
    return PRESET_PROMPTS_COUNT;
}

const char *xiaozhi_get_preset_prompt(uint32_t index)
{
    if (index >= PRESET_PROMPTS_COUNT) {
        index = index % PRESET_PROMPTS_COUNT;
    }
    return PRESET_PROMPTS[index];
}

void xiaozhi_sm_init(xiaozhi_state_machine_t *sm,
                     bool wifi_connected,
                     bool has_api_key,
                     xiaozhi_state_change_cb_t on_state_change,
                     void *user_data)
{
    if (!sm) return;
    memset(sm, 0, sizeof(*sm));
    sm->wifi_connected = wifi_connected;
    sm->has_api_key = has_api_key;
    sm->on_state_change = on_state_change;
    sm->user_data = user_data;
    sm->prompt_index = 0;

    if (!has_api_key) {
        sm->current_state = XIAOZHI_STATE_ERROR;
        snprintf(sm->last_error, sizeof(sm->last_error), "API Key Not Set");
    } else if (wifi_connected) {
        sm->current_state = XIAOZHI_STATE_IDLE;
    } else {
        sm->current_state = XIAOZHI_STATE_WIFI_CONNECTING;
    }
}

static void transition_to(xiaozhi_state_machine_t *sm, xiaozhi_state_t new_state)
{
    if (sm->current_state == new_state) return;
    xiaozhi_state_t old_state = sm->current_state;
    sm->current_state = new_state;
    if (sm->on_state_change) {
        sm->on_state_change(old_state, new_state, sm->user_data);
    }
}

xiaozhi_state_t xiaozhi_sm_step(xiaozhi_state_machine_t *sm, const xiaozhi_event_t *event)
{
    if (!sm || !event) return sm ? sm->current_state : XIAOZHI_STATE_ERROR;

    switch (event->type) {
        case XIAOZHI_EVT_WIFI_CONNECTED:
            sm->wifi_connected = true;
            if (!sm->has_api_key) {
                snprintf(sm->last_error, sizeof(sm->last_error), "API Key Not Configured");
                transition_to(sm, XIAOZHI_STATE_ERROR);
            } else if (sm->current_state == XIAOZHI_STATE_WIFI_CONNECTING || sm->current_state == XIAOZHI_STATE_BOOT) {
                transition_to(sm, XIAOZHI_STATE_IDLE);
            }
            break;

        case XIAOZHI_EVT_WIFI_DISCONNECTED:
            sm->wifi_connected = false;
            if (sm->current_state != XIAOZHI_STATE_BOOT) {
                transition_to(sm, XIAOZHI_STATE_WIFI_CONNECTING);
            }
            break;

        case XIAOZHI_EVT_BTN_UP_CLICK:
            if (sm->current_state == XIAOZHI_STATE_IDLE) {
                if (sm->prompt_index == 0) {
                    sm->prompt_index = PRESET_PROMPTS_COUNT - 1;
                } else {
                    sm->prompt_index--;
                }
            } else if (sm->current_state == XIAOZHI_STATE_ERROR) {
                transition_to(sm, sm->wifi_connected ? XIAOZHI_STATE_IDLE : XIAOZHI_STATE_WIFI_CONNECTING);
            }
            break;

        case XIAOZHI_EVT_BTN_DOWN_CLICK:
            if (sm->current_state == XIAOZHI_STATE_IDLE) {
                sm->prompt_index = (sm->prompt_index + 1) % PRESET_PROMPTS_COUNT;
            } else if (sm->current_state == XIAOZHI_STATE_ERROR) {
                transition_to(sm, sm->wifi_connected ? XIAOZHI_STATE_IDLE : XIAOZHI_STATE_WIFI_CONNECTING);
            }
            break;

        case XIAOZHI_EVT_BTN_OK_PRESS:
            if (sm->current_state == XIAOZHI_STATE_IDLE) {
                if (!sm->wifi_connected) {
                    snprintf(sm->last_error, sizeof(sm->last_error), "Wi-Fi Not Connected");
                    transition_to(sm, XIAOZHI_STATE_ERROR);
                } else if (!sm->has_api_key) {
                    snprintf(sm->last_error, sizeof(sm->last_error), "No DeepSeek Key");
                    transition_to(sm, XIAOZHI_STATE_ERROR);
                } else {
                    // Hold OK to start recording
                    transition_to(sm, XIAOZHI_STATE_LISTENING);
                }
            } else if (sm->current_state == XIAOZHI_STATE_SPEAKING || sm->current_state == XIAOZHI_STATE_THINKING) {
                // Interrupt and return to idle
                transition_to(sm, XIAOZHI_STATE_IDLE);
            }
            break;

        case XIAOZHI_EVT_BTN_OK_RELEASE:
            if (sm->current_state == XIAOZHI_STATE_LISTENING) {
                // Release OK to stop recording and send to DeepSeek
                transition_to(sm, XIAOZHI_STATE_THINKING);
            }
            break;

        case XIAOZHI_EVT_BTN_OK_CLICK:
            if (sm->current_state == XIAOZHI_STATE_IDLE) {
                if (!sm->wifi_connected) {
                    snprintf(sm->last_error, sizeof(sm->last_error), "Wi-Fi Not Connected");
                    transition_to(sm, XIAOZHI_STATE_ERROR);
                } else if (!sm->has_api_key) {
                    snprintf(sm->last_error, sizeof(sm->last_error), "No DeepSeek Key");
                    transition_to(sm, XIAOZHI_STATE_ERROR);
                } else {
                    // Short tap OK to trigger query for current preset prompt
                    transition_to(sm, XIAOZHI_STATE_THINKING);
                }
            } else if (sm->current_state == XIAOZHI_STATE_ERROR) {
                transition_to(sm, sm->wifi_connected ? XIAOZHI_STATE_IDLE : XIAOZHI_STATE_WIFI_CONNECTING);
            } else if (sm->current_state == XIAOZHI_STATE_SPEAKING || sm->current_state == XIAOZHI_STATE_THINKING) {
                transition_to(sm, XIAOZHI_STATE_IDLE);
            }
            break;

        case XIAOZHI_EVT_BTN_OK_LONG:
            // Long press handled seamlessly by PRESS + RELEASE
            break;

        case XIAOZHI_EVT_QUERY_START:
            transition_to(sm, XIAOZHI_STATE_THINKING);
            break;

        case XIAOZHI_EVT_QUERY_TOKEN:
            if (sm->current_state == XIAOZHI_STATE_THINKING) {
                transition_to(sm, XIAOZHI_STATE_SPEAKING);
            }
            break;

        case XIAOZHI_EVT_QUERY_DONE:
        case XIAOZHI_EVT_SPEECH_PLAY_DONE:
            if (sm->current_state == XIAOZHI_STATE_SPEAKING || sm->current_state == XIAOZHI_STATE_THINKING) {
                transition_to(sm, XIAOZHI_STATE_IDLE);
            }
            break;

        case XIAOZHI_EVT_QUERY_ERROR:
            if (event->str_payload && strlen(event->str_payload) > 0) {
                snprintf(sm->last_error, sizeof(sm->last_error), "%s", event->str_payload);
            } else {
                snprintf(sm->last_error, sizeof(sm->last_error), "Network / API Error");
            }
            transition_to(sm, XIAOZHI_STATE_ERROR);
            break;

        case XIAOZHI_EVT_RESET:
            transition_to(sm, sm->wifi_connected ? XIAOZHI_STATE_IDLE : XIAOZHI_STATE_WIFI_CONNECTING);
            break;

        default:
            break;
    }

    return sm->current_state;
}
