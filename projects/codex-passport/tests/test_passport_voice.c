/* Standalone: cc -std=c11 -Wall -Wextra -Werror -Iprojects/codex-passport/main
 * projects/codex-passport/tests/test_passport_voice.c -o /tmp/test_passport_voice
 * Includes the implementation to test its pure helpers without a test-only API. */
#include "../main/passport_voice.c"
#include <assert.h>

static void test_payloads(void)
{
    uint8_t packet[7U + PASSPORT_VOICE_TEXT_BYTES + 1U] = {0};
    write_le32(packet, 0x87654321U);
    assert(packet[0] == 0x21 && packet[3] == 0x87);
    packet[4] = PASSPORT_VOICE_RESULT_REVIEW;
    memcpy(packet + 7, "hello", 5);
    voice_result_t result;
    assert(parse_result(packet, 12, 0x87654321U, &result));
    assert(strcmp(result.text, "hello") == 0 && result.rid == 0x87654321U);
    voice_result_t original = result;
#define REJECT(p, len, rid) do { \
    assert(!parse_result(p, len, rid, &result)); \
    assert(memcmp(&result, &original, sizeof(result)) == 0); \
} while (0)
    REJECT(NULL, 12, original.rid);
    REJECT(packet, 6, original.rid);
    REJECT(packet, sizeof(packet), original.rid);
    REJECT(packet, 12, 1);
    REJECT(packet, 12, 0);
    REJECT(packet, 7, original.rid); /* empty review */
    packet[4] = 0;
    REJECT(packet, 12, original.rid);
    packet[4] = 7;
    REJECT(packet, 12, original.rid);
    packet[4] = PASSPORT_VOICE_RESULT_REVIEW;
    static const uint8_t invalid[][4] = {
        {0, 'a', 'b', 'c'}, {0xC0, 0x80, 'a', 'b'}, {0xED, 0xA0, 0x80, 'a'},
        {0xF4, 0x90, 0x80, 0x80}, {0xF0, 0x9F, 'a', 'b'}, {'a', 'b', 'c', 0xC2},
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        memcpy(packet + 7, invalid[i], 4);
        REJECT(packet, 11, original.rid);
    }
    packet[4] = PASSPORT_VOICE_RESULT_ACK;
    packet[5] = 94; /* maximum valid data block is 93 */
    REJECT(packet, 7, original.rid);
    packet[5] = packet[6] = 255;
    assert(parse_result(packet, 7, original.rid, &result));
    assert(result.block == PASSPORT_VOICE_BEGIN_BLOCK);
    packet[4] = PASSPORT_VOICE_RESULT_REVIEW;
    memset(packet + 7, 'a', PASSPORT_VOICE_TEXT_BYTES);
    memcpy(packet + 7, "\xE4\xBD\xA0\xF0\x9F\x98\x80", 7);
    assert(parse_result(packet, 7U + PASSPORT_VOICE_TEXT_BYTES, original.rid, &result));
    assert(strlen(result.text) == PASSPORT_VOICE_TEXT_BYTES);
    assert(result.text[PASSPORT_VOICE_TEXT_BYTES] == 0);
#undef REJECT
}

static void test_buttons_and_time(void)
{
    int16_t silent_pcm[256] = {0};
    assert(vad_is_silent(silent_pcm, 256, 120));
    int16_t loud_pcm[256];
    for (int i = 0; i < 256; ++i) loud_pcm[i] = 1000;
    assert(!vad_is_silent(loud_pcm, 256, 120));
    assert(PASSPORT_VOICE_SILENCE_MS == 5000U);

    assert(preview_state(false) == PASSPORT_VOICE_ERROR);
    assert(preview_state(true) == PASSPORT_VOICE_REVIEW);
    uint32_t early_press = confirm_token(PASSPORT_VOICE_TRANSCRIBING, 42);
    assert(!early_press);
    assert(confirm_control(PASSPORT_VOICE_REVIEW, 42, early_press) == PASSPORT_VOICE_REVIEW);
    uint32_t review_press = confirm_token(PASSPORT_VOICE_REVIEW, 42);
    assert(review_press == 42);
    assert(confirm_control(PASSPORT_VOICE_REVIEW, 42, review_press) & REQUEST_CONFIRM);
    assert(confirm_control(PASSPORT_VOICE_REVIEW, 43, review_press) == PASSPORT_VOICE_REVIEW);
    assert(confirm_control(PASSPORT_VOICE_SENDING, 42, review_press) == PASSPORT_VOICE_SENDING);
    passport_voice_release(); /* Compatibility API remains callable without initialization. */
    uint32_t c = PASSPORT_VOICE_PREPARING;
    assert(button_control(c, REQUEST_STOP) == c);
    c = state_control(c, PASSPORT_VOICE_RECORDING);
    assert(c == PASSPORT_VOICE_RECORDING);
    assert(button_control(PASSPORT_VOICE_RECORDING, REQUEST_STOP) & REQUEST_STOP);
    uint32_t stop_press = confirm_token(PASSPORT_VOICE_RECORDING, 42);
    assert(stop_press == 0);
    uint32_t stopped = button_control(PASSPORT_VOICE_RECORDING, REQUEST_STOP);
    stopped = state_control(stopped, PASSPORT_VOICE_UPLOADING);
    stopped = state_control(stopped, PASSPORT_VOICE_TRANSCRIBING);
    stopped = state_control(stopped, PASSPORT_VOICE_REVIEW);
    assert(!(confirm_control(stopped, 42, stop_press) & REQUEST_CONFIRM));
    assert(button_control(c, REQUEST_CONFIRM) == c);
    c = state_control(c, PASSPORT_VOICE_REVIEW);
    c = button_control(c, REQUEST_CONFIRM);
    assert(c & REQUEST_CONFIRM);
    assert(button_control(c, REQUEST_CONFIRM) == c); /* repeated click coalesces */
    c = state_control(c, PASSPORT_VOICE_SENDING);
    assert(!(c & REQUEST_CONFIRM));
    assert(button_control(c, REQUEST_CONFIRM) == c); /* never resubmit while sending */
    assert(!result_allowed(PASSPORT_VOICE_REVIEW, PASSPORT_VOICE_RESULT_SENT));
    assert(!result_allowed(PASSPORT_VOICE_SENDING, PASSPORT_VOICE_RESULT_REVIEW));
    assert(result_allowed(PASSPORT_VOICE_SENDING, PASSPORT_VOICE_RESULT_SENT));
    assert(result_allowed(PASSPORT_VOICE_TRANSCRIBING, PASSPORT_VOICE_RESULT_REVIEW));
    assert(result_allowed(PASSPORT_VOICE_UPLOADING, PASSPORT_VOICE_RESULT_ACK));
    assert(!result_allowed(PASSPORT_VOICE_ERROR, PASSPORT_VOICE_RESULT_REVIEW));
    assert(!result_allowed(PASSPORT_VOICE_IDLE, 255));
    c = button_control(c, REQUEST_CANCEL);
    c = button_control(c, REQUEST_DISCONNECT);
    c = state_control(c, PASSPORT_VOICE_ERROR);
    assert((c & (REQUEST_CANCEL | REQUEST_DISCONNECT)) == (REQUEST_CANCEL | REQUEST_DISCONNECT));
    assert(button_control(PASSPORT_VOICE_ERROR, REQUEST_CONFIRM) & REQUEST_CONFIRM);
    assert(button_control(PASSPORT_VOICE_SENT, REQUEST_CONFIRM) & REQUEST_CONFIRM);
    assert(!capture_at_limit(95999, 5999));
    assert(capture_at_limit(96000, 5999));
    assert(capture_at_limit(95999, 6000));
    assert(PASSPORT_VOICE_MAX_BYTES * 2U == PASSPORT_VOICE_MAX_SAMPLES);
    assert(PASSPORT_VOICE_MIN_SAMPLES == PASSPORT_VOICE_SAMPLE_RATE * 3U / 10U);
    assert(!expired(UINT32_MAX - 999U, 1999, 3000));
    assert(expired(UINT32_MAX - 999U, 2000, 3000));
    assert(!expired(100, 60099, PASSPORT_VOICE_RESULT_MS));
    assert(expired(100, 60100, PASSPORT_VOICE_RESULT_MS));
}

static void test_silence_and_ack(void)
{
    uint32_t silent = 0;
    /* Five seconds of actual PCM before speech; a 1.2s pause is insufficient. */
    assert(!silence_limit(&silent, true, 19200));
    assert(!silence_limit(&silent, true, 60799));
    assert(silence_limit(&silent, true, 1));
    assert(!silence_limit(&silent, false, 256));
    assert(silent == 0);
    /* Speech resets the same five-second timer, including interrupted pauses. */
    assert(!silence_limit(&silent, true, 79999));
    assert(!silence_limit(&silent, false, 256));
    assert(!silence_limit(&silent, true, 79999));
    assert(silence_limit(&silent, true, 1));
    int16_t boundary[] = {120, -120};
    assert(!vad_is_silent(boundary, 2, PASSPORT_VOICE_SILENCE_RMS));
    int16_t full_scale[] = {INT16_MIN, INT16_MAX};
    assert(!vad_is_silent(full_scale, 2, PASSPORT_VOICE_SILENCE_RMS));

    voice_result_t ack = {.rid = 42, .state = PASSPORT_VOICE_RESULT_ACK,
                          .block = PASSPORT_VOICE_BEGIN_BLOCK};
    assert(ack_matches(&ack, 42, PASSPORT_VOICE_BEGIN_BLOCK));
    assert(!ack_matches(&ack, 42, 0));
    for (uint16_t block = 0; block < 94; ++block) {
        ack.block = block;
        assert(ack_matches(&ack, 42, block));
        assert(!ack_matches(&ack, 43, block));
        assert(!ack_matches(&ack, 42, block + 1));
        ack.state = PASSPORT_VOICE_RESULT_ERROR;
        assert(!ack_matches(&ack, 42, block));
        ack.state = PASSPORT_VOICE_RESULT_ACK;
    }
}

int main(void)
{
    test_payloads();
    test_buttons_and_time();
    test_silence_and_ack();
    return 0;
}
