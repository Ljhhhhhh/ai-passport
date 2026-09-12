#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "passport_protocol.h"

#ifdef ESP_PLATFORM
#include "esp_err.h"
#else
typedef int esp_err_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Message types come from passport_protocol.h: snapshot 0x0D, upload 0x0E,
 * result 0x0F. */
#define PASSPORT_VOICE_SAMPLE_RATE 16000U
#define PASSPORT_VOICE_MAX_MS      6000U
#define PASSPORT_VOICE_MIN_SAMPLES 4800U
#define PASSPORT_VOICE_MAX_SAMPLES 96000U
#define PASSPORT_VOICE_MAX_BYTES   48000U
#define PASSPORT_VOICE_SILENCE_MS  5000U
#define PASSPORT_VOICE_SILENCE_RMS 120
#define PASSPORT_VOICE_BLOCK_BYTES 512U
#define PASSPORT_VOICE_TEXT_BYTES  768U
#define PASSPORT_VOICE_BEGIN_BLOCK UINT16_MAX
#define PASSPORT_VOICE_ACK_MS      3000U
#define PASSPORT_VOICE_MAX_RETRIES 3U
#define PASSPORT_VOICE_RESULT_MS   60000U

typedef enum {
    PASSPORT_VOICE_OP_BEGIN = 1,   /* LE <B I 16s I>: op, rid, UUID, sample_count */
    PASSPORT_VOICE_OP_DATA = 2,    /* LE <B I H> + <=512 bytes: op, rid, block */
    PASSPORT_VOICE_OP_END = 3,     /* LE <B I>: op, rid */
    PASSPORT_VOICE_OP_CONFIRM = 4, /* LE <B I>: sent once, never retried */
    PASSPORT_VOICE_OP_CANCEL = 5,  /* LE <B I>: best effort, never replayed */
} passport_voice_op_t;

/* RESULT is LE <I B H> + optional UTF-8, without a trailing NUL. */
typedef enum {
    PASSPORT_VOICE_RESULT_ACK = 1, /* begin block=65535; data block starts at 0 */
    PASSPORT_VOICE_RESULT_TRANSCRIBING = 2,
    PASSPORT_VOICE_RESULT_REVIEW = 3,
    PASSPORT_VOICE_RESULT_SENDING = 4,
    PASSPORT_VOICE_RESULT_SENT = 5,
    PASSPORT_VOICE_RESULT_ERROR = 6,
} passport_voice_result_state_t;

typedef enum {
    PASSPORT_VOICE_IDLE = 0,
    PASSPORT_VOICE_PREPARING,
    PASSPORT_VOICE_RECORDING,
    PASSPORT_VOICE_UPLOADING,
    PASSPORT_VOICE_TRANSCRIBING,
    PASSPORT_VOICE_REVIEW,
    PASSPORT_VOICE_SENDING,
    PASSPORT_VOICE_SENT,
    PASSPORT_VOICE_ERROR,
} passport_voice_state_t;

/* Call once at boot after UI init and before enabling voice input. BLE may be
 * initialized afterward; start rejects a missing connection. Creates
 * a lifetime worker; only that worker may call audio/alert playback APIs.
 * Remove the old alert task and direct settings chime in the integration. */
esp_err_t passport_voice_init(void);

/* Input TASK, on talk gesture: copy the complete NUL-terminated title and 16 raw UUID
 * bytes. No title truncation; false on busy/disconnected/allocation failure.
 * Caller must show a start-failure notice when false is returned.
 * An alert already playing finishes before preparation; wait for "Speak now".
 * One active operation at a time. Busy includes review and terminal dismissal. */
bool passport_voice_start(const uint8_t thread_id[16], const char *title);
/* Compatibility no-op: releasing OK never stops or cancels click-to-talk. */
void passport_voice_release(void);
/* Nonblocking atomic requests, task context (not ISR). Stop recording on click. */
void passport_voice_stop(void);
/* Call directly at physical PRESS, before queueing input. The returned token
 * permits confirmation only if this press began in a confirmable phase. */
uint32_t passport_voice_press(void);
void passport_voice_cancel(void);  /* long OK: discard/cancel, then hide */
void passport_voice_confirm(uint32_t pressed_rid); /* matching press only */
bool passport_voice_busy(void);
passport_voice_state_t passport_voice_get_state(void);

/* BLE task: copy a complete RESULT to a bounded queue without waiting. False
 * means malformed/stale result or queue full; no text/audio is ever logged. */
bool passport_voice_receive(const uint8_t *payload, size_t len);
/* REQUIRED from every BLE disconnect event, before accepting a new connection.
 * Invalidates queued/in-flight work even on immediate reconnect. Cleanup occurs
 * in the worker after any current BSP read (driver timeout is 1000 ms). */
void passport_voice_on_disconnect(void);

/* Integration supplies these functions. send_message must copy/frame payload
 * before returning, serialize complete messages with other BLE sends, and bind
 * each send to its original connection (never finish it on a new connection).
 * UI functions run on the worker: copy strings synchronously and internally
 * take bsp_lvgl_lock before LVGL access. Keep the display awake while busy. */
esp_err_t passport_ble_send_message(uint8_t type, const void *payload, size_t len);
bool passport_ui_voice_show(const char *phase, const char *body, const char *hint);
bool passport_ui_voice_hide(void);

/* Audio: raw continuous IMA ADPCM, predictor/index initially 0/0, low nibble
 * first, no per-block reset/header. sample_count excludes an unused high nibble
 * in the last byte. All captured bytes upload, including at the six-second cap.
 * Five seconds of consecutive silent PCM stops capture before or after speech;
 * the total six-second cap can end capture before that silence interval elapses.
 * No storage, automatic confirmation, resend after confirm, or reconnect replay. */

#ifdef __cplusplus
}
#endif
