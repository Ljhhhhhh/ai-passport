// main/deepseek_client.h
// DeepSeek Chat Completion HTTPS client with SSE streaming for ESP32.
#pragma once

#include "deepseek_sse_parser.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize DeepSeek client resources.
 */
void deepseek_client_init(void);

/**
 * @brief Send a chat prompt to DeepSeek API with streaming response tokens.
 * @param user_prompt User message string.
 * @param on_token Token callback invoked as words/characters arrive.
 * @param on_complete Callback invoked when the response finishes.
 * @param on_error Callback invoked on HTTP/API/Network error.
 * @param user_data Context pointer passed to callbacks.
 * @return true if request completed successfully, false on error.
 */
bool deepseek_client_chat(const char *user_prompt,
                          deepseek_token_cb_t on_token,
                          deepseek_complete_cb_t on_complete,
                          deepseek_error_cb_t on_error,
                          void *user_data);

/**
 * @brief Cancel any currently ongoing chat request.
 */
void deepseek_client_cancel(void);

/**
 * @brief Check if a request is currently active.
 */
bool deepseek_client_is_busy(void);

#ifdef __cplusplus
}
#endif
