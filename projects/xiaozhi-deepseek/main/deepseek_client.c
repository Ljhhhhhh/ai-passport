// main/deepseek_client.c
#include "deepseek_client.h"
#include "xiaozhi_config.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "deepseek_client";
static bool s_is_busy = false;
static bool s_cancel_requested = false;

void deepseek_client_init(void)
{
    ESP_LOGI(TAG, "DeepSeek client initialized (Built-in provider: %s, model: %s)",
             CONFIG_XIAOZHI_MODEL_PROVIDER, CONFIG_XIAOZHI_DEEPSEEK_MODEL);
}

bool deepseek_client_is_busy(void)
{
    return s_is_busy;
}

void deepseek_client_cancel(void)
{
    if (s_is_busy) {
        s_cancel_requested = true;
        ESP_LOGI(TAG, "Cancellation requested");
    }
}

bool deepseek_client_chat(const char *user_prompt,
                          deepseek_token_cb_t on_token,
                          deepseek_complete_cb_t on_complete,
                          deepseek_error_cb_t on_error,
                          void *user_data)
{
    if (!user_prompt || strlen(user_prompt) == 0) {
        if (on_error) on_error("Empty prompt", user_data);
        return false;
    }

    const xiaozhi_config_t *cfg = xiaozhi_config_get();
    if (!cfg || strlen(cfg->api_key) == 0) {
        ESP_LOGE(TAG, "DeepSeek API Key is empty! Please configure in sdkconfig or menuconfig.");
        if (on_error) on_error("API Key Not Configured", user_data);
        return false;
    }

    s_is_busy = true;
    s_cancel_requested = false;

    // Construct full URL
    char full_url[384];
    if (strstr(cfg->base_url, "/chat/completions")) {
        snprintf(full_url, sizeof(full_url), "%s", cfg->base_url);
    } else {
        // Strip trailing slash if any
        size_t blen = strlen(cfg->base_url);
        if (blen > 0 && cfg->base_url[blen - 1] == '/') {
            snprintf(full_url, sizeof(full_url), "%schat/completions", cfg->base_url);
        } else {
            snprintf(full_url, sizeof(full_url), "%s/chat/completions", cfg->base_url);
        }
    }

    // Build JSON request payload
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", cfg->model);
    cJSON_AddNumberToObject(root, "temperature", (double)cfg->temperature);
    cJSON_AddNumberToObject(root, "max_tokens", cfg->max_tokens);
    cJSON_AddBoolToObject(root, "stream", true);

    cJSON *messages = cJSON_AddArrayToObject(root, "messages");
    if (strlen(cfg->system_prompt) > 0) {
        cJSON *sys_msg = cJSON_CreateObject();
        cJSON_AddStringToObject(sys_msg, "role", "system");
        cJSON_AddStringToObject(sys_msg, "content", cfg->system_prompt);
        cJSON_AddItemToArray(messages, sys_msg);
    }

    cJSON *usr_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(usr_msg, "role", "user");
    cJSON_AddStringToObject(usr_msg, "content", user_prompt);
    cJSON_AddItemToArray(messages, usr_msg);

    char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!post_data) {
        s_is_busy = false;
        if (on_error) on_error("Failed to serialize JSON", user_data);
        return false;
    }

    ESP_LOGI(TAG, "Sending request to: %s (model: %s)", full_url, cfg->model);

    // Setup HTTP Client
    esp_http_client_config_t http_cfg = {
        .url = full_url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = 2048,
        .buffer_size_tx = 1024,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);
    if (!client) {
        free(post_data);
        s_is_busy = false;
        if (on_error) on_error("HTTP client init failed", user_data);
        return false;
    }

    // Set HTTP Headers
    char auth_header[300];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", cfg->api_key);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "text/event-stream");
    esp_http_client_set_header(client, "Authorization", auth_header);

    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_open(client, strlen(post_data));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        free(post_data);
        s_is_busy = false;
        if (on_error) on_error("Connection Failed", user_data);
        return false;
    }

    int wlen = esp_http_client_write(client, post_data, strlen(post_data));
    free(post_data);
    if (wlen < 0) {
        ESP_LOGE(TAG, "Failed to write HTTP POST data");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        s_is_busy = false;
        if (on_error) on_error("HTTP Write Error", user_data);
        return false;
    }

    int content_length = esp_http_client_fetch_headers(client);
    (void)content_length;
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP Response Status Code = %d", status_code);

    if (status_code != 200) {
        char err_body[256];
        int r = esp_http_client_read(client, err_body, sizeof(err_body) - 1);
        if (r > 0) err_body[r] = '\0'; else err_body[0] = '\0';
        ESP_LOGE(TAG, "HTTP Error %d: %s", status_code, err_body);

        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "HTTP %d Error", status_code);
        if (status_code == 401) {
            snprintf(err_msg, sizeof(err_msg), "Invalid API Key (401)");
        } else if (status_code == 402) {
            snprintf(err_msg, sizeof(err_msg), "Insufficient Balance (402)");
        } else if (status_code == 429) {
            snprintf(err_msg, sizeof(err_msg), "Rate Limited (429)");
        }

        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        s_is_busy = false;
        if (on_error) on_error(err_msg, user_data);
        return false;
    }

    // Initialize SSE Stream Parser
    deepseek_sse_parser_t parser;
    deepseek_sse_parser_init(&parser, on_token, on_complete, on_error, user_data);

    char chunk_buf[1024];
    bool success = true;

    while (!parser.is_done && !s_cancel_requested) {
        int read_bytes = esp_http_client_read(client, chunk_buf, sizeof(chunk_buf));
        if (read_bytes < 0) {
            ESP_LOGE(TAG, "HTTP read error");
            success = false;
            if (on_error) on_error("Stream Read Error", user_data);
            break;
        } else if (read_bytes == 0) {
            // EOF
            break;
        }

        deepseek_parse_res_t parse_res = deepseek_sse_parser_feed(&parser, chunk_buf, (size_t)read_bytes);
        if (parse_res == DEEPSEEK_PARSE_ERR_API_ERROR) {
            success = false;
            break;
        }
    }

    if (s_cancel_requested) {
        ESP_LOGI(TAG, "Request cancelled by user");
        success = false;
    } else if (success && !parser.is_done) {
        if (on_complete) on_complete(user_data);
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    s_is_busy = false;
    return success;
}
