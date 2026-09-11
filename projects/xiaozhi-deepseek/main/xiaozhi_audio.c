// main/xiaozhi_audio.c
#include "xiaozhi_audio.h"
#include "xiaozhi_config.h"
#include "bsp_audio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static const char *TAG = "xiaozhi_audio";
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAMPLE_RATE 16000
#define RECORD_BUF_SIZE (16000 * 2 * 6) // Max 6 seconds @ 16kHz 16-bit mono = ~192KB buffer (dynamic)
static TickType_t s_record_start_tick = 0;
static uint8_t *s_record_buf = NULL;
static size_t s_recorded_bytes = 0;
static bool s_is_recording = false;
static int64_t s_record_start_time = 0;
static TaskHandle_t s_record_task_handle = NULL;

static void record_worker_task(void *pvParameters)
{
    (void)pvParameters;
    int16_t chunk[256];
    size_t chunk_bytes = sizeof(chunk);

    ESP_LOGI(TAG, "Recording worker task started");

    while (s_is_recording) {
        esp_err_t err = bsp_audio_read(chunk, chunk_bytes);
        if (err == ESP_OK && s_record_buf) {
            if (s_recorded_bytes + chunk_bytes <= RECORD_BUF_SIZE) {
                memcpy(&s_record_buf[s_recorded_bytes], chunk, chunk_bytes);
                s_recorded_bytes += chunk_bytes;
            } else {
                // Buffer full, stop recording automatically
                ESP_LOGW(TAG, "Recording buffer full (6s limit)");
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGI(TAG, "Recording worker task ended (total %zu bytes)", s_recorded_bytes);
    s_record_task_handle = NULL;
    vTaskDelete(NULL);
}

bool xiaozhi_audio_init(void)
{
    esp_err_t err = bsp_audio_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_audio_init failed: %s", esp_err_to_name(err));
        return false;
    }

    err = bsp_audio_set_format(SAMPLE_RATE, 16, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_audio_set_format failed: %s", esp_err_to_name(err));
        return false;
    }

    const xiaozhi_config_t *cfg = xiaozhi_config_get();
    uint8_t vol = cfg ? cfg->volume : 75;
    bsp_audio_set_volume(vol);

    ESP_LOGI(TAG, "Audio initialized successfully (16kHz 16-bit mono, vol=%d%%)", vol);
    return true;
}

void xiaozhi_audio_set_volume(uint8_t percent)
{
    if (percent > 100) percent = 100;
    bsp_audio_set_volume(percent);
}

static void play_tone(float freq_hz, uint32_t duration_ms, float gain)
{
    size_t total_samples = (SAMPLE_RATE * duration_ms) / 1000;
    if (total_samples == 0) return;

    int16_t *buf = (int16_t *)malloc(total_samples * sizeof(int16_t));
    if (!buf) return;

    size_t attack_samples = total_samples / 8;
    size_t decay_samples = total_samples / 4;

    for (size_t i = 0; i < total_samples; i++) {
        float env = 1.0f;
        if (i < attack_samples) {
            env = (float)i / (float)attack_samples;
        } else if (i > total_samples - decay_samples) {
            env = (float)(total_samples - i) / (float)decay_samples;
        }

        float sample = sinf(2.0f * (float)M_PI * freq_hz * (float)i / (float)SAMPLE_RATE);
        int32_t val = (int32_t)(sample * env * gain * 28000.0f);
        if (val > 32767) val = 32767;
        if (val < -32768) val = -32768;
        buf[i] = (int16_t)val;
    }

    bsp_audio_write(buf, total_samples * sizeof(int16_t));
    free(buf);
}

void xiaozhi_audio_play_chime(xiaozhi_chime_type_t chime)
{
    switch (chime) {
        case XIAOZHI_CHIME_WAKE:
            play_tone(587.33f, 70, 0.4f);
            play_tone(880.00f, 110, 0.5f);
            break;

        case XIAOZHI_CHIME_THINK:
            play_tone(523.25f, 60, 0.3f);
            break;

        case XIAOZHI_CHIME_DONE:
            play_tone(523.25f, 60, 0.35f);
            play_tone(659.25f, 60, 0.40f);
            play_tone(783.99f, 120, 0.45f);
            break;

        case XIAOZHI_CHIME_ERROR:
            play_tone(330.0f, 90, 0.4f);
            vTaskDelay(pdMS_TO_TICKS(30));
            play_tone(220.0f, 140, 0.4f);
            break;

        case XIAOZHI_CHIME_CLICK:
            play_tone(1200.0f, 15, 0.2f);
            break;

        default:
            break;
    }
}

void xiaozhi_audio_start_record(void)
{
    if (s_is_recording) return;

    if (!s_record_buf) {
        s_record_buf = (uint8_t *)malloc(RECORD_BUF_SIZE);
    }
    s_recorded_bytes = 0;
    s_is_recording = true;
    s_record_start_tick = xTaskGetTickCount();

    xTaskCreate(record_worker_task, "rec_task", 3072, NULL, 5, &s_record_task_handle);
    ESP_LOGI(TAG, "Push-to-Talk recording started");
}

uint32_t xiaozhi_audio_stop_record(size_t *out_bytes)
{
    if (!s_is_recording) {
        if (out_bytes) *out_bytes = 0;
        return 0;
    }

    s_is_recording = false;
    uint32_t duration_ms = (uint32_t)pdTICKS_TO_MS(xTaskGetTickCount() - s_record_start_tick);

    // Wait for record task to exit
    int timeout = 20;
    while (s_record_task_handle != NULL && timeout-- > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (out_bytes) {
        *out_bytes = s_recorded_bytes;
    }

    ESP_LOGI(TAG, "Push-to-Talk recording stopped: duration=%lu ms, bytes=%zu",
             (unsigned long)duration_ms, s_recorded_bytes);
    return duration_ms;
}

bool xiaozhi_audio_is_recording(void)
{
    return s_is_recording;
}

void xiaozhi_audio_play_speech_token(const char *token, size_t len)
{
    if (!token || len == 0) return;

    // Filter punctuation or whitespace
    char first = token[0];
    if (first == ' ' || first == '\n' || first == '\r' || first == '\t' ||
        first == ',' || first == '.' || first == '!' || first == '?' ||
        first == ':' || first == ';') {
        vTaskDelay(pdMS_TO_TICKS(40));
        return;
    }

    // Hash token to produce varied natural formant speech notes (380Hz ~ 680Hz vocal range)
    uint32_t h = 5381;
    for (size_t i = 0; i < len; i++) {
        h = ((h << 5) + h) + (uint8_t)token[i];
    }
    float base_f = 380.0f + (float)(h % 280);

    // Synthesize short syllable voice tone (40ms)
    size_t samples = (SAMPLE_RATE * 40) / 1000;
    int16_t *buf = (int16_t *)malloc(samples * sizeof(int16_t));
    if (!buf) return;

    for (size_t i = 0; i < samples; i++) {
        float env = 1.0f - (float)i / (float)samples;
        float s1 = sinf(2.0f * (float)M_PI * base_f * (float)i / (float)SAMPLE_RATE);
        float s2 = sinf(2.0f * (float)M_PI * (base_f * 2.1f) * (float)i / (float)SAMPLE_RATE) * 0.4f;
        int32_t val = (int32_t)((s1 + s2) * env * 0.45f * 24000.0f);
        if (val > 32767) val = 32767;
        if (val < -32768) val = -32768;
        buf[i] = (int16_t)val;
    }

    bsp_audio_write(buf, samples * sizeof(int16_t));
    free(buf);
}

void xiaozhi_audio_play_pcm(const void *pcm, size_t bytes)
{
    if (!pcm || bytes == 0) return;
    bsp_audio_write(pcm, bytes);
}
