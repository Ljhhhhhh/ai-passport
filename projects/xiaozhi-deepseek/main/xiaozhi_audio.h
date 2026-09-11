// main/xiaozhi_audio.h
// Audio management, voice recording and speech synthesis for Xiaozhi DeepSeek AI Assistant.
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XIAOZHI_CHIME_WAKE = 0,   // Two-tone bright chime when starting to listen
    XIAOZHI_CHIME_THINK,      // Gentle soft tone when query is submitted
    XIAOZHI_CHIME_DONE,       // Happy ascending chime when response is complete
    XIAOZHI_CHIME_ERROR,      // Low double-beep on error
    XIAOZHI_CHIME_CLICK,      // Short crisp click on button press
} xiaozhi_chime_type_t;

/**
 * @brief Initialize audio subsystem (ES8311 I2S duplex).
 */
bool xiaozhi_audio_init(void);

/**
 * @brief Set speaker output volume percentage (0-100%).
 */
void xiaozhi_audio_set_volume(uint8_t percent);

/**
 * @brief Play a synthesized notification chime.
 */
void xiaozhi_audio_play_chime(xiaozhi_chime_type_t chime);

/**
 * @brief Start voice recording from ES8311 microphone.
 */
void xiaozhi_audio_start_record(void);

/**
 * @brief Stop voice recording and return recorded duration and bytes.
 * @param out_bytes Pointer to receive total recorded PCM bytes.
 * @return Total recording duration in milliseconds.
 */
uint32_t xiaozhi_audio_stop_record(size_t *out_bytes);

/**
 * @brief Check if microphone is currently recording.
 */
bool xiaozhi_audio_is_recording(void);

/**
 * @brief Play vocal speech audio for streaming text tokens.
 * @param token Text token from DeepSeek.
 * @param len Byte length of token.
 */
void xiaozhi_audio_play_speech_token(const char *token, size_t len);

/**
 * @brief Play raw PCM audio data directly to speaker.
 */
void xiaozhi_audio_play_pcm(const void *pcm, size_t bytes);

#ifdef __cplusplus
}
#endif
