// main/xiaozhi_ui.h
// LVGL UI interface for Xiaozhi DeepSeek AI Assistant.
#pragma once

#include "xiaozhi_state.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Xiaozhi LVGL UI on current screen.
 */
void xiaozhi_ui_init(void);

/**
 * @brief Update UI display to reflect new state.
 */
void xiaozhi_ui_set_state(xiaozhi_state_t state);

/**
 * @brief Update Wi-Fi connection indicator in top status bar.
 */
void xiaozhi_ui_set_wifi_status(bool connected, const char *ip_str);

/**
 * @brief Update battery status in top bar.
 */
void xiaozhi_ui_set_battery(uint8_t percent, bool charging);

/**
 * @brief Set current user prompt / topic displayed in dialog box.
 */
void xiaozhi_ui_set_prompt(const char *prompt);

/**
 * @brief Clear streaming response text and prepare for new output.
 */
void xiaozhi_ui_clear_response(void);

/**
 * @brief Append incoming streaming token to response text view.
 */
void xiaozhi_ui_append_response_token(const char *token, size_t len, bool is_reasoning);

/**
 * @brief Set error message display.
 */
void xiaozhi_ui_set_error(const char *error_msg);

/**
 * @brief Periodic timer callback to animate avatar eyes, waves, and status spinners.
 */
void xiaozhi_ui_tick_anim(void);

#ifdef __cplusplus
}
#endif
