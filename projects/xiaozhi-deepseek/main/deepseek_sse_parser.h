// main/deepseek_sse_parser.h
// Pure C, zero-dependency SSE (Server-Sent Events) and JSON response parser for DeepSeek / OpenAI API.
#pragma once

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DEEPSEEK_PARSE_OK = 0,
    DEEPSEEK_PARSE_DONE,
    DEEPSEEK_PARSE_NEED_MORE,
    DEEPSEEK_PARSE_ERR_INVALID_FORMAT,
    DEEPSEEK_PARSE_ERR_API_ERROR,
    DEEPSEEK_PARSE_ERR_BUFFER_OVERFLOW,
} deepseek_parse_res_t;

typedef void (*deepseek_token_cb_t)(const char *token, size_t len, bool is_reasoning, void *user_data);
typedef void (*deepseek_complete_cb_t)(void *user_data);
typedef void (*deepseek_error_cb_t)(const char *err_msg, void *user_data);

typedef struct {
    char line_buf[2048];
    size_t line_len;
    deepseek_token_cb_t on_token;
    deepseek_complete_cb_t on_complete;
    deepseek_error_cb_t on_error;
    void *user_data;
    bool is_done;
} deepseek_sse_parser_t;

/**
 * @brief Initialize the SSE stream parser.
 */
void deepseek_sse_parser_init(deepseek_sse_parser_t *parser,
                              deepseek_token_cb_t on_token,
                              deepseek_complete_cb_t on_complete,
                              deepseek_error_cb_t on_error,
                              void *user_data);

/**
 * @brief Feed incoming raw HTTP chunk bytes into the parser.
 * @param parser Pointer to parser struct.
 * @param chunk Raw chunk bytes.
 * @param len Length of chunk in bytes.
 * @return parse result status.
 */
deepseek_parse_res_t deepseek_sse_parser_feed(deepseek_sse_parser_t *parser, const char *chunk, size_t len);

/**
 * @brief Parse a single SSE line (e.g. "data: {...}" or "data: [DONE]").
 */
deepseek_parse_res_t deepseek_sse_parse_line(deepseek_sse_parser_t *parser, const char *line, size_t len);

/**
 * @brief Extract delta content string from a DeepSeek/OpenAI JSON delta payload.
 * @param json_str JSON string.
 * @param json_len Length of JSON string.
 * @param out_buf Output buffer for extracted unescaped content.
 * @param max_out Maximum bytes for out_buf.
 * @param out_is_reasoning Output flag set to true if content is reasoning_content (DeepSeek-R1).
 * @return Number of bytes written to out_buf, or -1 on error.
 */
int deepseek_extract_delta(const char *json_str, size_t json_len, char *out_buf, size_t max_out, bool *out_is_reasoning);

/**
 * @brief Helper to unescape JSON strings (handling \", \\, \n, \r, \t, etc.).
 */
size_t deepseek_json_unescape(const char *src, size_t src_len, char *dst, size_t dst_max);

#ifdef __cplusplus
}
#endif
