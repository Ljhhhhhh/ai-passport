// tests/test_deepseek_parser.c
// Host unit test for DeepSeek SSE Stream Parser.
#include "../main/deepseek_sse_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    char accumulated_text[4096];
    size_t text_len;
    char last_error[256];
    bool is_completed;
    int token_count;
    bool has_reasoning;
} test_ctx_t;

static void on_test_token(const char *token, size_t len, bool is_reasoning, void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    if (is_reasoning) {
        ctx->has_reasoning = true;
    }
    if (ctx->text_len + len < sizeof(ctx->accumulated_text) - 1) {
        memcpy(&ctx->accumulated_text[ctx->text_len], token, len);
        ctx->text_len += len;
        ctx->accumulated_text[ctx->text_len] = '\0';
    }
    ctx->token_count++;
}

static void on_test_complete(void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    ctx->is_completed = true;
}

static void on_test_error(const char *err_msg, void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    if (err_msg) {
        snprintf(ctx->last_error, sizeof(ctx->last_error), "%s", err_msg);
    }
}

static void test_json_unescape(void)
{
    printf("[TEST] Running test_json_unescape...\n");
    char out[128];

    // Simple string
    size_t n = deepseek_json_unescape("Hello World", 11, out, sizeof(out));
    assert(n == 11);
    assert(strcmp(out, "Hello World") == 0);

    // Escaped newline and quotes
    const char *escaped = "Line 1\\nLine 2 with \\\"quotes\\\" and \\\\ slash";
    n = deepseek_json_unescape(escaped, strlen(escaped), out, sizeof(out));
    assert(strcmp(out, "Line 1\nLine 2 with \"quotes\" and \\ slash") == 0);

    // Unicode escape \u4f60\u597d (你好)
    const char *u_escaped = "\\u4f60\\u597d";
    n = deepseek_json_unescape(u_escaped, strlen(u_escaped), out, sizeof(out));
    assert(n == 6); // UTF-8 3 bytes each -> 6 bytes
    assert(strcmp(out, "你好") == 0);

    printf("[TEST] test_json_unescape PASSED\n");
}

static void test_sse_streaming_chunk(void)
{
    printf("[TEST] Running test_sse_streaming_chunk...\n");
    test_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    deepseek_sse_parser_t parser;
    deepseek_sse_parser_init(&parser, on_test_token, on_test_complete, on_test_error, &ctx);

    const char *sse_payload =
        "data: {\"id\":\"1\",\"choices\":[{\"index\":0,\"delta\":{\"content\":\"Hello\"}}]}\n\n"
        "data: {\"id\":\"2\",\"choices\":[{\"index\":0,\"delta\":{\"content\":\" from\"}}]}\n\n"
        "data: {\"id\":\"3\",\"choices\":[{\"index\":0,\"delta\":{\"content\":\" DeepSeek!\"}}]}\n\n"
        "data: [DONE]\n\n";

    deepseek_parse_res_t res = deepseek_sse_parser_feed(&parser, sse_payload, strlen(sse_payload));
    assert(res == DEEPSEEK_PARSE_OK);
    assert(ctx.token_count == 3);
    assert(strcmp(ctx.accumulated_text, "Hello from DeepSeek!") == 0);
    assert(ctx.is_completed == true);

    printf("[TEST] test_sse_streaming_chunk PASSED\n");
}

static void test_fragmented_feed(void)
{
    printf("[TEST] Running test_fragmented_feed...\n");
    test_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    deepseek_sse_parser_t parser;
    deepseek_sse_parser_init(&parser, on_test_token, on_test_complete, on_test_error, &ctx);

    // Feed bytes 1 by 1 to simulate TCP/TLS fragmentation
    const char *sse_payload =
        "data: {\"choices\":[{\"delta\":{\"content\":\"你好\"}}]}\n"
        "data: {\"choices\":[{\"delta\":{\"content\":\"世界\"}}]}\n"
        "data: [DONE]\n";

    size_t total = strlen(sse_payload);
    for (size_t i = 0; i < total; i++) {
        deepseek_sse_parser_feed(&parser, &sse_payload[i], 1);
    }

    assert(ctx.token_count == 2);
    assert(strcmp(ctx.accumulated_text, "你好世界") == 0);
    assert(ctx.is_completed == true);

    printf("[TEST] test_fragmented_feed PASSED\n");
}

static void test_deepseek_reasoning_content(void)
{
    printf("[TEST] Running test_deepseek_reasoning_content...\n");
    test_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    deepseek_sse_parser_t parser;
    deepseek_sse_parser_init(&parser, on_test_token, on_test_complete, on_test_error, &ctx);

    const char *r1_payload =
        "data: {\"choices\":[{\"delta\":{\"reasoning_content\":\"Thinking step 1...\"}}]}\n"
        "data: {\"choices\":[{\"delta\":{\"content\":\"Final Answer.\"}}]}\n"
        "data: [DONE]\n";

    deepseek_sse_parser_feed(&parser, r1_payload, strlen(r1_payload));

    assert(ctx.has_reasoning == true);
    assert(strcmp(ctx.accumulated_text, "Thinking step 1...Final Answer.") == 0);
    assert(ctx.is_completed == true);

    printf("[TEST] test_deepseek_reasoning_content PASSED\n");
}

static void test_api_error_payload(void)
{
    printf("[TEST] Running test_api_error_payload...\n");
    test_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    deepseek_sse_parser_t parser;
    deepseek_sse_parser_init(&parser, on_test_token, on_test_complete, on_test_error, &ctx);

    const char *err_payload =
        "{\"error\":{\"message\":\"Authentication FAILED: Invalid API Key\",\"type\":\"auth_error\"}}\n";

    deepseek_parse_res_t res = deepseek_sse_parser_feed(&parser, err_payload, strlen(err_payload));
    assert(res == DEEPSEEK_PARSE_ERR_API_ERROR);
    assert(strcmp(ctx.last_error, "Authentication FAILED: Invalid API Key") == 0);

    printf("[TEST] test_api_error_payload PASSED\n");
}

int main(void)
{
    printf("========================================\n");
    printf("  Running DeepSeek SSE Parser Tests\n");
    printf("========================================\n");

    test_json_unescape();
    test_sse_streaming_chunk();
    test_fragmented_feed();
    test_deepseek_reasoning_content();
    test_api_error_payload();

    printf("ALL DEEPSEEK PARSER TESTS PASSED!\n");
    return 0;
}
