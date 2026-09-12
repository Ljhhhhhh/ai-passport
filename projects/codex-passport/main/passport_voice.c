#include "passport_voice.h"
#include "passport_adpcm.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATE_MASK 0xFFU
#define REQUEST_CANCEL (1U << 9)
#define REQUEST_CONFIRM (1U << 10)
#define REQUEST_DISCONNECT (1U << 11)
#define REQUEST_STOP (1U << 12)
#define PCM_SAMPLES 256U

typedef struct {
    uint32_t rid;
    uint16_t block;
    uint8_t state;
    char text[PASSPORT_VOICE_TEXT_BYTES + 1U];
} voice_result_t;

/* Pure helpers are also exercised by test_passport_voice.c. */
static uint32_t button_control(uint32_t control, uint32_t request)
{
    passport_voice_state_t state = control & STATE_MASK;
    if (state == PASSPORT_VOICE_IDLE) return control;
    if (request == REQUEST_STOP && state != PASSPORT_VOICE_RECORDING) return control;
    if (request == REQUEST_CONFIRM && state != PASSPORT_VOICE_REVIEW &&
        state != PASSPORT_VOICE_SENT && state != PASSPORT_VOICE_ERROR) return control;
    return control | request;
}

static uint32_t state_control(uint32_t control, passport_voice_state_t state)
{
    /* A press from an earlier phase must never confirm a later phase. */
    return (control & (REQUEST_CANCEL | REQUEST_DISCONNECT | REQUEST_STOP)) | state;
}

void passport_voice_release(void) {}

static uint32_t confirm_token(uint32_t control, uint32_t rid)
{
    return button_control(control, REQUEST_CONFIRM) & REQUEST_CONFIRM ? rid : 0;
}

static uint32_t confirm_control(uint32_t control, uint32_t rid, uint32_t pressed_rid)
{
    return rid && rid == pressed_rid ? button_control(control, REQUEST_CONFIRM) : control;
}

static passport_voice_state_t preview_state(bool shown)
{
    return shown ? PASSPORT_VOICE_REVIEW : PASSPORT_VOICE_ERROR;
}

static bool expired(uint32_t start, uint32_t now, uint32_t timeout)
{
    return (uint32_t)(now - start) >= timeout;
}

static bool capture_at_limit(uint32_t samples, uint32_t elapsed_ms)
{
    return samples >= PASSPORT_VOICE_MAX_SAMPLES || elapsed_ms >= PASSPORT_VOICE_MAX_MS;
}

static bool vad_is_silent(const int16_t *pcm, size_t samples, int32_t thresh_rms)
{
    if (!pcm || samples == 0) return true;
    uint64_t sum_sq = 0;
    for (size_t i = 0; i < samples; ++i) {
        int32_t v = pcm[i];
        sum_sq += (uint64_t)(v * v);
    }
    uint64_t mean_sq = sum_sq / samples;
    return mean_sq < (uint64_t)(thresh_rms * thresh_rms);
}

static bool silence_limit(uint32_t *silent_samples, bool silent, uint32_t samples)
{
    *silent_samples = silent ? *silent_samples + samples : 0;
    return *silent_samples >= PASSPORT_VOICE_SAMPLE_RATE * PASSPORT_VOICE_SILENCE_MS / 1000U;
}

static bool ack_matches(const voice_result_t *result, uint32_t rid, uint16_t block)
{
    return result->rid == rid && result->state == PASSPORT_VOICE_RESULT_ACK && result->block == block;
}

static bool result_allowed(passport_voice_state_t local, uint8_t remote)
{
    switch (remote) {
    case PASSPORT_VOICE_RESULT_ACK: return local == PASSPORT_VOICE_UPLOADING;
    case PASSPORT_VOICE_RESULT_TRANSCRIBING:
    case PASSPORT_VOICE_RESULT_REVIEW: return local == PASSPORT_VOICE_TRANSCRIBING;
    case PASSPORT_VOICE_RESULT_SENDING:
    case PASSPORT_VOICE_RESULT_SENT: return local == PASSPORT_VOICE_SENDING;
    case PASSPORT_VOICE_RESULT_ERROR:
        return local >= PASSPORT_VOICE_UPLOADING && local <= PASSPORT_VOICE_SENDING;
    default: return false;
    }
}

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void write_le32(uint8_t *p, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (i * 8U));
}

static bool valid_utf8(const uint8_t *p, size_t len)
{
    for (size_t i = 0; i < len;) {
        uint32_t cp = p[i++];
        if (cp < 0x80U) {
            if (cp == 0) return false; /* No embedded NUL hiding unreviewed text. */
            continue;
        }
        unsigned extra;
        uint32_t minimum;
        if (cp >= 0xC2U && cp <= 0xDFU) { extra = 1; minimum = 0x80; cp &= 0x1F; }
        else if (cp >= 0xE0U && cp <= 0xEFU) { extra = 2; minimum = 0x800; cp &= 0x0F; }
        else if (cp >= 0xF0U && cp <= 0xF4U) { extra = 3; minimum = 0x10000; cp &= 7; }
        else return false;
        if (len - i < extra) return false;
        while (extra--) {
            if ((p[i] & 0xC0U) != 0x80U) return false;
            cp = (cp << 6) | (p[i++] & 0x3FU);
        }
        if (cp < minimum || cp > 0x10FFFFU || (cp >= 0xD800U && cp <= 0xDFFFU)) return false;
    }
    return true;
}

static bool parse_result(const uint8_t *p, size_t len, uint32_t rid, voice_result_t *out)
{
    if (!p || !out || !rid || len < 7U || len > 7U + PASSPORT_VOICE_TEXT_BYTES ||
        read_le32(p) != rid || p[4] < PASSPORT_VOICE_RESULT_ACK ||
        p[4] > PASSPORT_VOICE_RESULT_ERROR || !valid_utf8(p + 7, len - 7U)) return false;
    uint16_t block = (uint16_t)p[5] | (uint16_t)p[6] << 8;
    if (p[4] == PASSPORT_VOICE_RESULT_ACK && block != PASSPORT_VOICE_BEGIN_BLOCK &&
        block >= (PASSPORT_VOICE_MAX_BYTES + PASSPORT_VOICE_BLOCK_BYTES - 1U) /
                 PASSPORT_VOICE_BLOCK_BYTES) return false;
    if (p[4] == PASSPORT_VOICE_RESULT_REVIEW && len == 7U) return false;
    *out = (voice_result_t){ .rid = rid, .block = block, .state = p[4] };
    memcpy(out->text, p + 7, len - 7U);
    return true;
}

#ifdef ESP_PLATFORM
#include "bsp_audio.h"
#include "passport_alert.h"
#include "passport_ble.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

typedef struct {
    uint8_t thread_id[16];
    char *title;
    uint32_t rid;
    uint32_t connection;
} voice_request_t;

static const char *TAG = "passport_voice";
static QueueHandle_t s_starts;
static QueueHandle_t s_results;
static _Atomic bool s_ready;
static _Atomic uint32_t s_control;
static _Atomic uint32_t s_rid;
static _Atomic uint32_t s_connection;
static uint32_t s_next_rid; /* Starts are serialized by the input task. */

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

passport_voice_state_t passport_voice_get_state(void)
{
    return atomic_load(&s_control) & STATE_MASK;
}

bool passport_voice_busy(void)
{
    return passport_voice_get_state() != PASSPORT_VOICE_IDLE;
}

static void request_control(uint32_t request)
{
    uint32_t old = atomic_load(&s_control);
    while (!atomic_compare_exchange_weak(&s_control, &old, button_control(old, request))) {}
}

static void set_state(passport_voice_state_t state)
{
    uint32_t old = atomic_load(&s_control);
    while (!atomic_compare_exchange_weak(&s_control, &old, state_control(old, state))) {}
}

uint32_t passport_voice_press(void)
{
    return confirm_token(atomic_load(&s_control), atomic_load(&s_rid));
}

void passport_voice_stop(void)
{
    request_control(REQUEST_STOP);
}
void passport_voice_cancel(void) { request_control(REQUEST_CANCEL); }
void passport_voice_confirm(uint32_t pressed_rid)
{
    uint32_t old = atomic_load(&s_control);
    while (!atomic_compare_exchange_weak(&s_control, &old,
            confirm_control(old, atomic_load(&s_rid), pressed_rid))) {}
}

void passport_voice_on_disconnect(void)
{
    atomic_fetch_add(&s_connection, 1U);
    request_control(REQUEST_DISCONNECT);
}

bool passport_voice_receive(const uint8_t *payload, size_t len)
{
    voice_result_t result;
    if (!atomic_load(&s_ready) || (atomic_load(&s_control) & REQUEST_DISCONNECT) ||
        !parse_result(payload, len, atomic_load(&s_rid), &result) ||
        !result_allowed(passport_voice_get_state(), result.state)) return false;
    return xQueueSend(s_results, &result, 0) == pdTRUE;
}

bool passport_voice_start(const uint8_t thread_id[16], const char *title)
{
    if (!thread_id || !title || !atomic_load(&s_ready) || !passport_ble_is_connected()) return false;
    uint8_t nonzero = 0;
    for (unsigned i = 0; i < 16; ++i) nonzero |= thread_id[i];
    if (!nonzero) return false;
    voice_request_t request = { .connection = atomic_load(&s_connection) };
    uint32_t expected = PASSPORT_VOICE_IDLE;
    if (!atomic_compare_exchange_strong(&s_control, &expected, PASSPORT_VOICE_PREPARING)) return false;
    /* Reserve busy before allocation, so cancellation cannot be lost. */
    request.title = strdup(title);
    if (request.title) {
        memcpy(request.thread_id, thread_id, sizeof(request.thread_id));
        if (++s_next_rid == 0) ++s_next_rid;
        request.rid = s_next_rid;
        atomic_store(&s_rid, request.rid);
        if (request.connection == atomic_load(&s_connection) && passport_ble_is_connected() &&
            xQueueSend(s_starts, &request, 0) == pdTRUE) return true;
        free(request.title);
    }
    atomic_store(&s_rid, 0);
    atomic_store(&s_control, PASSPORT_VOICE_IDLE);
    return false;
}

static bool connected(const voice_request_t *request)
{
    return request->connection == atomic_load(&s_connection) &&
           !(atomic_load(&s_control) & REQUEST_DISCONNECT) && passport_ble_is_connected();
}

static bool active(const voice_request_t *request)
{
    return connected(request) && !(atomic_load(&s_control) & REQUEST_CANCEL);
}

static void show_error(const char *body)
{
    passport_ui_voice_show("Voice error", body, "OK: close");
    set_state(PASSPORT_VOICE_ERROR);
}

static esp_err_t send_op(const voice_request_t *request, passport_voice_op_t op)
{
    uint8_t packet[5] = { (uint8_t)op };
    write_le32(packet + 1, request->rid);
    if (!connected(request)) return ESP_ERR_INVALID_STATE;
    return passport_ble_send_message(MSG_TYPE_VOICE, packet, sizeof(packet));
}

static bool wait_ack(const voice_request_t *request, const uint8_t *packet, size_t len, uint16_t block)
{
    voice_result_t result;
    for (unsigned attempt = 0; attempt <= PASSPORT_VOICE_MAX_RETRIES; ++attempt) {
        if (!active(request)) return false;
        uint32_t started = now_ms();
        esp_err_t err = passport_ble_send_message(MSG_TYPE_VOICE, packet, len);
        /* A partial send can still reach the host. Retry the identical rid/block. */
        if (err != ESP_OK || attempt) {
            ESP_LOGI(TAG, "upload block=%u attempt=%u transport=%d", block, attempt + 1U, (int)err);
        }
        while (active(request) && !expired(started, now_ms(), PASSPORT_VOICE_ACK_MS)) {
            if (xQueueReceive(s_results, &result, pdMS_TO_TICKS(20)) != pdTRUE) continue;
            if (result.rid != request->rid) continue;
            if (result.state == PASSPORT_VOICE_RESULT_ERROR) {
                show_error(result.text[0] ? result.text : "The host rejected the recording.");
                return false;
            }
            if (ack_matches(&result, request->rid, block)) return true;
        }
    }
    if (active(request)) show_error("Upload timed out. The message was not confirmed.");
    return false;
}

static bool capture(const voice_request_t *request, uint8_t *audio, uint32_t *sample_count, bool *limited)
{
    int16_t pcm[PCM_SAMPLES];
    passport_ui_voice_show("Get ready", request->title, "Wait for Speak now");
    if (!active(request)) return false;
    bsp_audio_set_volume(0);
    /* BSP has 6 x 240 stereo DMA frames and no RX-flush API. Discard more than
     * that capacity, then require a fresh blocking read. Bound this preparation
     * to 1 s; retune the threshold if BSP DMA geometry or sample rate changes. */
    uint32_t flush_started = now_ms();
    uint32_t discarded = 0;
    for (;;) {
        if (!active(request)) return false;
        uint32_t before = now_ms();
        if (bsp_audio_read(pcm, sizeof(pcm)) != ESP_OK) {
            show_error("Microphone read failed.");
            return false;
        }
        discarded += PCM_SAMPLES;
        if (discarded >= 6U * 240U * 2U && expired(before, now_ms(), 8U)) break;
        if (expired(flush_started, now_ms(), 1000U)) {
            show_error("Microphone did not become ready.");
            return false;
        }
    }
    if (!active(request)) return false;
    if (!passport_ui_voice_show("Speak now (max 6s)", request->title, "OK: stop; long OK: cancel\n5s silence: auto-stop")) {
        show_error("Display busy. Recording cancelled; try again.");
        return false;
    }
    set_state(PASSPORT_VOICE_RECORDING);
    uint32_t started = now_ms();
    uint32_t silent_samples = 0;
    uint32_t last_ui_update = started;
    passport_adpcm_state_t encoder;
    passport_adpcm_init(&encoder, 0, 0);
    while (active(request)) {
        *limited = capture_at_limit(*sample_count, (uint32_t)(now_ms() - started));
        if (*limited || (atomic_load(&s_control) & REQUEST_STOP)) break;
        size_t count = PASSPORT_VOICE_MAX_SAMPLES - *sample_count;
        if (count > PCM_SAMPLES) count = PCM_SAMPLES;
        if (bsp_audio_read(pcm, count * sizeof(*pcm)) != ESP_OK) {
            show_error("Microphone read failed. Recording discarded.");
            return false;
        }
        bool is_silence = vad_is_silent(pcm, count, PASSPORT_VOICE_SILENCE_RMS);
        bool silence_done = silence_limit(&silent_samples, is_silence, (uint32_t)count);
        if (expired(last_ui_update, now_ms(), 200U)) {
            last_ui_update = now_ms();
            char phase[32];
            snprintf(phase, sizeof(phase), "Speak now %lus / 6s",
                     (unsigned long)(*sample_count / PASSPORT_VOICE_SAMPLE_RATE));
            passport_ui_voice_show(phase, request->title,
                                  is_silence ? "OK: stop [ . . . ]\nLong OK: cancel" :
                                               "OK: stop [ |||||||| ]\nLong OK: cancel");
        }
        /* Retain the entire successful read, including the silence boundary. */
        for (size_t i = 0; i < count; ++i) {
            uint8_t nibble = passport_adpcm_encode_nibble(&encoder, pcm[i]);
            uint32_t n = (*sample_count)++;
            if (n & 1U) audio[n / 2U] |= (uint8_t)(nibble << 4);
            else audio[n / 2U] = nibble;
        }
        if (silence_done) break;
    }
    *limited = capture_at_limit(*sample_count, (uint32_t)(now_ms() - started));
    if (!active(request)) return false;
    if (*sample_count < PASSPORT_VOICE_MIN_SAMPLES) {
        show_error("Recording too short. Speak for at least 0.3 seconds.");
        return false;
    }
    return true;
}

static bool upload(const voice_request_t *request, const uint8_t *audio, uint32_t samples,
                   bool limited, bool *host_started)
{
    uint8_t packet[7U + PASSPORT_VOICE_BLOCK_BYTES];
    passport_ui_voice_show("Uploading audio", request->title,
                          limited ? "6s limit reached\nWait for upload" : "Wait for upload\nReview text next");
    xQueueReset(s_results);
    set_state(PASSPORT_VOICE_UPLOADING);
    packet[0] = PASSPORT_VOICE_OP_BEGIN;
    write_le32(packet + 1, request->rid);
    memcpy(packet + 5, request->thread_id, 16);
    write_le32(packet + 21, samples);
    *host_started = true; /* Even a transport error may mean partial delivery. */
    if (!wait_ack(request, packet, 25U, PASSPORT_VOICE_BEGIN_BLOCK)) return false;
    size_t bytes = (samples + 1U) / 2U;
    uint16_t block = 0;
    for (size_t offset = 0; offset < bytes; ++block) {
        if (!active(request)) return false;
        size_t count = bytes - offset;
        if (count > PASSPORT_VOICE_BLOCK_BYTES) count = PASSPORT_VOICE_BLOCK_BYTES;
        packet[0] = PASSPORT_VOICE_OP_DATA;
        packet[5] = (uint8_t)block;
        packet[6] = (uint8_t)(block >> 8);
        memcpy(packet + 7, audio + offset, count);
        /* Host deduplicates identical rid/block retries and ACKs every block. */
        if (!wait_ack(request, packet, count + 7U, block)) return false;
        offset += count;
        if (block % 8U == 0 || offset == bytes) {
            char phase[32];
            snprintf(phase, sizeof(phase), "Uploading %u%%", (unsigned)(offset * 100U / bytes));
            passport_ui_voice_show(phase, request->title, "Long OK: cancel\nReview text next");
        }
    }
    if (!active(request)) return false;
    xQueueReset(s_results);
    set_state(PASSPORT_VOICE_TRANSCRIBING);
    passport_ui_voice_show("Waiting for host", request->title, "Upload sent\nWaiting for transcription");
    if (send_op(request, PASSPORT_VOICE_OP_END) != ESP_OK) {
        show_error("Could not finish upload. The message was not confirmed.");
        return false;
    }
    return true;
}

static void await_user_and_result(const voice_request_t *request, bool *host_started)
{
    voice_result_t result;
    uint32_t started = now_ms();
    while (active(request)) {
        uint32_t control = atomic_load(&s_control);
        passport_voice_state_t state = control & STATE_MASK;
        if (control & REQUEST_CONFIRM) {
            if (state == PASSPORT_VOICE_SENT || state == PASSPORT_VOICE_ERROR) return;
            if (state == PASSPORT_VOICE_REVIEW) {
                passport_ui_voice_show("Sending", "Sending the message you confirmed.", "Waiting for receipt");
                set_state(PASSPORT_VOICE_SENDING); /* Consumes confirm before sending. */
                started = now_ms();
                if (!active(request)) return;
                if (send_op(request, PASSPORT_VOICE_OP_CONFIRM) != ESP_OK) {
                    show_error("Send status unknown. Check the task on the host before trying again.");
                }
                continue;
            }
        }
        if ((state == PASSPORT_VOICE_TRANSCRIBING || state == PASSPORT_VOICE_SENDING) &&
            expired(started, now_ms(), PASSPORT_VOICE_RESULT_MS)) {
            show_error(state == PASSPORT_VOICE_SENDING ?
                       "Send status unknown. Check the task on the host before trying again." :
                       "Transcription timed out. The message was not confirmed.");
            continue;
        }
        if (xQueueReceive(s_results, &result, pdMS_TO_TICKS(20)) != pdTRUE ||
            result.rid != request->rid || !result_allowed(state, result.state)) continue;
        switch (result.state) {
        case PASSPORT_VOICE_RESULT_TRANSCRIBING:
            passport_ui_voice_show("Transcribing", request->title, "Wait, then review text");
            break;
        case PASSPORT_VOICE_RESULT_REVIEW:
            /* Publish REVIEW only after the recognized text is visible. */
            set_state(preview_state(passport_ui_voice_show(request->title, result.text,
                      "OK: send; long OK: cancel\nReview text: UP/DOWN")));
            if (passport_voice_get_state() == PASSPORT_VOICE_ERROR)
                show_error("Preview unavailable. Nothing sent; try again.");
            break;
        case PASSPORT_VOICE_RESULT_SENT:
            *host_started = false;
            passport_ui_voice_show("Accepted by Codex", result.text[0] ? result.text : "Message accepted; task completion pending.", "OK: close\nRead reply in Codex");
            set_state(PASSPORT_VOICE_SENT);
            break;
        case PASSPORT_VOICE_RESULT_ERROR:
            show_error(result.text[0] ? result.text : "The host could not complete the voice request.");
            break;
        default:
            /* Progress/duplicate events do not extend the absolute deadline. */
            break;
        }
    }
}

static void voice_worker(void *arg)
{
    (void)arg;
    for (;;) {
        voice_request_t request;
        if (xQueueReceive(s_starts, &request, pdMS_TO_TICKS(20)) != pdTRUE) {
            if (!passport_voice_busy()) {
                uint8_t type;
                if (passport_ble_take_alert_type(&type)) passport_alert_play((passport_alert_type_t)type);
            }
            continue;
        }
        uint8_t *audio = NULL;
        uint32_t samples = 0;
        bool limited = false;
        bool host_started = false;
        if (active(&request)) {
            /* Reserve codec/DMA resources before the much larger recording. */
            bool audio_ready = bsp_audio_init() == ESP_OK &&
                               bsp_audio_set_format(PASSPORT_VOICE_SAMPLE_RATE, 16, 1) == ESP_OK;
            if (audio_ready) audio = malloc(PASSPORT_VOICE_MAX_BYTES);
            ESP_LOGI(TAG, "capture allocation=%u free=%u largest=%u", audio ? PASSPORT_VOICE_MAX_BYTES : 0U,
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
                     (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
            if (!audio_ready) show_error("Microphone initialization failed.");
            else if (!audio) show_error("Not enough memory to record. Close this message and try again.");
            else if (capture(&request, audio, &samples, &limited)) {
                upload(&request, audio, samples, limited, &host_started);
            }
        }
        free(audio); /* No audio retained during review or after disconnect. */
        ESP_LOGI(TAG, "capture samples=%lu bytes=%lu limited=%u free=%u stack=%u",
                 (unsigned long)samples, (unsigned long)((samples + 1U) / 2U), (unsigned)limited,
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
                 (unsigned)uxTaskGetStackHighWaterMark(NULL));
        if (active(&request)) await_user_and_result(&request, &host_started);
        if (host_started && connected(&request)) send_op(&request, PASSPORT_VOICE_OP_CANCEL);
        atomic_store(&s_rid, 0);
        xQueueReset(s_results);
        while (!passport_ui_voice_hide()) vTaskDelay(pdMS_TO_TICKS(20));
        free(request.title);
        atomic_store(&s_control, PASSPORT_VOICE_IDLE); /* Release ownership last. */
    }
}

esp_err_t passport_voice_init(void)
{
    if (atomic_load(&s_ready)) return ESP_OK;
    s_starts = xQueueCreate(1, sizeof(voice_request_t));
    s_results = xQueueCreate(4, sizeof(voice_result_t));
    if (s_starts && s_results) {
        s_next_rid = esp_random();
        if (xTaskCreate(voice_worker, "passport_audio", 6144, NULL, 3, NULL) == pdPASS) {
            atomic_store(&s_ready, true);
            return ESP_OK;
        }
    }
    if (s_starts) vQueueDelete(s_starts);
    if (s_results) vQueueDelete(s_results);
    s_starts = s_results = NULL;
    return ESP_ERR_NO_MEM;
}
#endif
