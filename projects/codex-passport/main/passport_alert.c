#include "passport_alert.h"
#include <math.h>
#include <string.h>

bool passport_alert_accept(uint32_t *last_sequence, uint32_t sequence)
{
    if (!last_sequence || sequence == 0 || sequence <= *last_sequence) return false;
    *last_sequence = sequence;
    return true;
}

#ifdef ESP_PLATFORM
#include "bsp_audio.h"
#include "passport_clips.h"
#include "passport_adpcm.h"
#include "esp_log.h"

static const char *TAG = "passport_alert";
static bool s_audio_inited = false;
static bool s_voice_enabled = true;
static uint8_t s_volume = 80;

void passport_alert_set_settings(bool voice_enabled, uint8_t volume)
{
    s_voice_enabled = voice_enabled;
    if (volume > 100) volume = 100;
    s_volume = volume;
}

void passport_alert_get_settings(bool *voice_enabled, uint8_t *volume)
{
    if (voice_enabled) *voice_enabled = s_voice_enabled;
    if (volume) *volume = s_volume;
}

static bool ensure_audio(uint8_t volume)
{
    if (!s_audio_inited) {
        if (bsp_audio_init() == ESP_OK && bsp_audio_set_format(16000, 16, 1) == ESP_OK) {
            bsp_audio_set_volume(volume);
            s_audio_inited = true;
        } else {
            ESP_LOGW(TAG, "Audio init failed for alert");
            return false;
        }
    } else {
        bsp_audio_set_volume(volume);
    }
    return true;
}

void passport_alert_play_chime(void)
{
    if (!s_voice_enabled || s_volume == 0) return;
    if (!ensure_audio(s_volume)) return;

    // Soft C5/E5 major third; a short attack avoids an abrupt waveform edge.
    const uint32_t sample_rate = 16000;
    const float freqs[2] = {523.25f, 659.25f};
    const size_t samples_per_tone = (sample_rate * 180) / 1000;
    int16_t buffer[256];

    for (int t = 0; t < 2; t++) {
        float freq = freqs[t];
        size_t done = 0;
        while (done < samples_per_tone) {
            size_t chunk = samples_per_tone - done;
            if (chunk > sizeof(buffer) / sizeof(buffer[0])) {
                chunk = sizeof(buffer) / sizeof(buffer[0]);
            }
            for (size_t i = 0; i < chunk; i++) {
                size_t idx = done + i;
                float time = (float)idx / (float)sample_rate;
                float attack = fminf(1.0f, (float)idx / (sample_rate * 0.015f));
                float release = 1.0f - (float)idx / (float)(samples_per_tone - 1);
                float envelope = attack * attack * release * release;
                float val = sinf(2.0f * (float)M_PI * freq * time) * envelope;
                buffer[i] = (int16_t)(val * 10000.0f);
            }
            if (bsp_audio_write(buffer, chunk * sizeof(int16_t)) != ESP_OK) {
                ESP_LOGW(TAG, "Alert audio write failed");
                return;
            }
            done += chunk;
        }
        // Silence separates the notes and drains the final samples through I2S.
        for (size_t i = 0; i < 256; ++i) buffer[i] = 0;
        for (int i = 0; i < 2; ++i) {
            if (bsp_audio_write(buffer, sizeof(buffer)) != ESP_OK) {
                ESP_LOGW(TAG, "Alert audio tail write failed");
                return;
            }
        }
    }
}

static bool play_clip(passport_clip_id_t clip_id)
{
    passport_clip_blob_t blob;
    if (!passport_clip_blob(clip_id, &blob) || !blob.data || blob.size == 0) {
        return false;
    }

    passport_adpcm_header_t header;
    const uint8_t *data = NULL;
    size_t data_len = 0;
    if (!passport_adpcm_parse_clip(blob.data, blob.size, &header, &data, &data_len)) {
        return false;
    }

    if (!s_voice_enabled || s_volume == 0) return true;
    if (!ensure_audio(s_volume)) return false;

    passport_adpcm_state_t state;
    passport_adpcm_init(&state, header.predictor, header.step_index);

    int16_t pcm[PASSPORT_ADPCM_CHUNK_SAMPLES];
    size_t nibble_index = 0;
    while (nibble_index < header.sample_count) {
        size_t samples = header.sample_count - nibble_index;
        if (samples > PASSPORT_ADPCM_CHUNK_SAMPLES) {
            samples = PASSPORT_ADPCM_CHUNK_SAMPLES;
        }
        size_t decoded = passport_adpcm_decode_bytes(&state, data, data_len, nibble_index, pcm, samples);
        if (decoded == 0) break;
        if (bsp_audio_write(pcm, decoded * sizeof(int16_t)) != ESP_OK) {
            ESP_LOGW(TAG, "Voice playback write failed");
            return false;
        }
        nibble_index += decoded;
    }

    // Brief silence tail to drain DMA
    memset(pcm, 0, sizeof(pcm));
    bsp_audio_write(pcm, 256 * sizeof(int16_t));
    return true;
}

void passport_alert_play(passport_alert_type_t type)
{
    if (!s_voice_enabled || s_volume == 0) return;
    passport_clip_id_t clip_id = PASSPORT_CLIP_NONE;
    switch (type) {
    case PASSPORT_ALERT_WAIT:
        clip_id = PASSPORT_CLIP_WAIT;
        break;
    case PASSPORT_ALERT_DONE:
        clip_id = PASSPORT_CLIP_DONE;
        break;
    case PASSPORT_ALERT_ERROR:
        clip_id = PASSPORT_CLIP_ERROR;
        break;
    case PASSPORT_ALERT_NEW_MSG:
        clip_id = PASSPORT_CLIP_NEW_MSG;
        break;
    default:
        break;
    }

    if (clip_id != PASSPORT_CLIP_NONE && play_clip(clip_id)) {
        return;
    }

    // Fallback to chime if clip not found or type is default
    passport_alert_play_chime();
}

#else

static bool s_voice_enabled = true;
static uint8_t s_volume = 80;

void passport_alert_set_settings(bool voice_enabled, uint8_t volume)
{
    s_voice_enabled = voice_enabled;
    if (volume > 100) volume = 100;
    s_volume = volume;
}

void passport_alert_get_settings(bool *voice_enabled, uint8_t *volume)
{
    if (voice_enabled) *voice_enabled = s_voice_enabled;
    if (volume) *volume = s_volume;
}

void passport_alert_play_chime(void) {}
void passport_alert_play(passport_alert_type_t type) { (void)type; }

#endif
