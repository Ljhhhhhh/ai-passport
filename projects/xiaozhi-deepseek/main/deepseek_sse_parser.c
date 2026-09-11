// main/deepseek_sse_parser.c
#include "deepseek_sse_parser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void deepseek_sse_parser_init(deepseek_sse_parser_t *parser,
                              deepseek_token_cb_t on_token,
                              deepseek_complete_cb_t on_complete,
                              deepseek_error_cb_t on_error,
                              void *user_data)
{
    if (!parser) return;
    memset(parser, 0, sizeof(*parser));
    parser->on_token = on_token;
    parser->on_complete = on_complete;
    parser->on_error = on_error;
    parser->user_data = user_data;
    parser->is_done = false;
}

size_t deepseek_json_unescape(const char *src, size_t src_len, char *dst, size_t dst_max)
{
    if (!src || !dst || dst_max == 0) return 0;
    size_t out_idx = 0;
    size_t i = 0;

    while (i < src_len && out_idx + 1 < dst_max) {
        if (src[i] == '\\' && i + 1 < src_len) {
            char next = src[i + 1];
            if (next == '"') {
                dst[out_idx++] = '"';
                i += 2;
            } else if (next == '\\') {
                dst[out_idx++] = '\\';
                i += 2;
            } else if (next == '/') {
                dst[out_idx++] = '/';
                i += 2;
            } else if (next == 'n') {
                dst[out_idx++] = '\n';
                i += 2;
            } else if (next == 'r') {
                dst[out_idx++] = '\r';
                i += 2;
            } else if (next == 't') {
                dst[out_idx++] = '\t';
                i += 2;
            } else if (next == 'b') {
                dst[out_idx++] = '\b';
                i += 2;
            } else if (next == 'f') {
                dst[out_idx++] = '\f';
                i += 2;
            } else if (next == 'u' && i + 5 < src_len) {
                // Unicode hex escape \uXXXX
                char hex[5] = { src[i+2], src[i+3], src[i+4], src[i+5], '\0' };
                unsigned long cp = strtoul(hex, NULL, 16);
                if (cp <= 0x7F) {
                    dst[out_idx++] = (char)cp;
                } else if (cp <= 0x7FF) {
                    if (out_idx + 2 < dst_max) {
                        dst[out_idx++] = (char)(0xC0 | (cp >> 6));
                        dst[out_idx++] = (char)(0x80 | (cp & 0x3F));
                    }
                } else if (cp <= 0xFFFF) {
                    if (out_idx + 3 < dst_max) {
                        dst[out_idx++] = (char)(0xE0 | (cp >> 12));
                        dst[out_idx++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                        dst[out_idx++] = (char)(0x80 | (cp & 0x3F));
                    }
                }
                i += 6;
            } else {
                dst[out_idx++] = src[i++];
            }
        } else {
            dst[out_idx++] = src[i++];
        }
    }
    dst[out_idx] = '\0';
    return out_idx;
}

// Find a JSON field string like "key":"val"
static const char *find_json_string_field(const char *json, size_t json_len, const char *key, size_t *out_val_len)
{
    size_t key_len = strlen(key);
    for (size_t i = 0; i + key_len + 3 < json_len; i++) {
        if (json[i] == '"' && strncmp(&json[i + 1], key, key_len) == 0 && json[i + 1 + key_len] == '"') {
            size_t p = i + 1 + key_len + 1;
            while (p < json_len && (json[p] == ' ' || json[p] == ':' || json[p] == '\t')) {
                p++;
            }
            if (p < json_len && json[p] == '"') {
                p++; // start of string value
                size_t start = p;
                bool escape = false;
                while (p < json_len) {
                    if (escape) {
                        escape = false;
                    } else if (json[p] == '\\') {
                        escape = true;
                    } else if (json[p] == '"') {
                        *out_val_len = p - start;
                        return &json[start];
                    }
                    p++;
                }
            }
        }
    }
    return NULL;
}

int deepseek_extract_delta(const char *json_str, size_t json_len, char *out_buf, size_t max_out, bool *out_is_reasoning)
{
    if (!json_str || !out_buf || max_out == 0) return -1;
    if (out_is_reasoning) *out_is_reasoning = false;

    // First check for error message: {"error":{"message":"..."}}
    size_t err_len = 0;
    const char *err_val = find_json_string_field(json_str, json_len, "message", &err_len);
    if (err_val && strstr(json_str, "\"error\"")) {
        deepseek_json_unescape(err_val, err_len, out_buf, max_out);
        return -2; // Indicates API error
    }

    // Check reasoning_content (DeepSeek-R1 / deepseek-reasoner)
    size_t reasoning_len = 0;
    const char *reasoning_val = find_json_string_field(json_str, json_len, "reasoning_content", &reasoning_len);
    if (reasoning_val && reasoning_len > 0) {
        if (out_is_reasoning) *out_is_reasoning = true;
        return (int)deepseek_json_unescape(reasoning_val, reasoning_len, out_buf, max_out);
    }

    // Check standard content delta
    size_t content_len = 0;
    const char *content_val = find_json_string_field(json_str, json_len, "content", &content_len);
    if (content_val) {
        return (int)deepseek_json_unescape(content_val, content_len, out_buf, max_out);
    }

    return 0; // No delta content in this chunk (e.g. role metadata or ping)
}

deepseek_parse_res_t deepseek_sse_parse_line(deepseek_sse_parser_t *parser, const char *line, size_t len)
{
    if (!parser || !line || len == 0) return DEEPSEEK_PARSE_OK;

    // Trim leading whitespace
    while (len > 0 && (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n')) {
        line++;
        len--;
    }
    // Trim trailing whitespace
    while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t' || line[len - 1] == '\r' || line[len - 1] == '\n')) {
        len--;
    }

    if (len == 0) return DEEPSEEK_PARSE_OK; // Empty line / SSE keepalive

    // Check SSE data prefix
    if (len >= 5 && strncmp(line, "data:", 5) == 0) {
        line += 5;
        len -= 5;
        while (len > 0 && (*line == ' ' || *line == '\t')) {
            line++;
            len--;
        }
    }

    // Check stream termination [DONE]
    if (len >= 6 && strncmp(line, "[DONE]", 6) == 0) {
        parser->is_done = true;
        if (parser->on_complete) {
            parser->on_complete(parser->user_data);
        }
        return DEEPSEEK_PARSE_DONE;
    }

    // Check JSON payload
    if (len > 0 && line[0] == '{') {
        char delta_buf[1024];
        bool is_reasoning = false;
        int res = deepseek_extract_delta(line, len, delta_buf, sizeof(delta_buf), &is_reasoning);
        if (res == -2) {
            // API Error
            if (parser->on_error) {
                parser->on_error(delta_buf, parser->user_data);
            }
            return DEEPSEEK_PARSE_ERR_API_ERROR;
        } else if (res > 0) {
            if (parser->on_token) {
                parser->on_token(delta_buf, (size_t)res, is_reasoning, parser->user_data);
            }
            return DEEPSEEK_PARSE_OK;
        } else if (res == 0) {
            return DEEPSEEK_PARSE_OK;
        } else {
            return DEEPSEEK_PARSE_ERR_INVALID_FORMAT;
        }
    }

    return DEEPSEEK_PARSE_OK;
}

deepseek_parse_res_t deepseek_sse_parser_feed(deepseek_sse_parser_t *parser, const char *chunk, size_t len)
{
    if (!parser || !chunk || len == 0) return DEEPSEEK_PARSE_OK;

    size_t i = 0;
    while (i < len) {
        char c = chunk[i++];
        if (c == '\n') {
            // End of line, parse buffer
            parser->line_buf[parser->line_len] = '\0';
            deepseek_parse_res_t res = deepseek_sse_parse_line(parser, parser->line_buf, parser->line_len);
            parser->line_len = 0;
            if (res == DEEPSEEK_PARSE_ERR_API_ERROR) {
                return res;
            }
        } else if (c != '\r') {
            if (parser->line_len + 1 < sizeof(parser->line_buf)) {
                parser->line_buf[parser->line_len++] = c;
            } else {
                // Buffer overflow protection, flush and error
                parser->line_len = 0;
                if (parser->on_error) {
                    parser->on_error("SSE line buffer overflow", parser->user_data);
                }
                return DEEPSEEK_PARSE_ERR_BUFFER_OVERFLOW;
            }
        }
    }

    return DEEPSEEK_PARSE_OK;
}
