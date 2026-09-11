#include "hanzi_audio.h"

#include "bsp_audio.h"
#include "hanzi_clips.h"
#include "hanzi_player.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <math.h>
#include <string.h>

#define AUDIO_RATE 16000U
#define REWARD_MS 1800U

typedef enum {
    AUDIO_CMD_PLAY_CLIP = 0,
    AUDIO_CMD_PLAY_REWARD,
    AUDIO_CMD_CANCEL
} audio_cmd_type_t;

typedef struct {
    audio_cmd_type_t type;
    hanzi_clip_id_t clip;
} audio_cmd_t;

static const char *TAG = "hanzi_audio";
static QueueHandle_t s_queue;
static hanzi_audio_done_cb_t s_done_cb;
static void *s_done_user;
static volatile uint32_t s_generation;
static bool s_available;

static void notify_done(int success)
{
    if (s_done_cb) {
        s_done_cb(success, s_done_user);
    }
}

static int write_pcm(const int16_t *pcm, size_t samples, uint32_t generation)
{
    if (generation != s_generation) {
        return 0;
    }
    if (bsp_audio_write(pcm, samples * sizeof(int16_t)) != ESP_OK) {
        return -1;
    }
    return 1;
}

static int play_clip(hanzi_clip_id_t clip, uint32_t generation)
{
    hanzi_clip_blob_t blob;
    hanzi_player_t player;
    int16_t pcm[HANZI_ADPCM_CHUNK_SAMPLES];

    if (!hanzi_clip_blob(clip, &blob) || !hanzi_player_start(&player, blob.data, blob.size)) {
        ESP_LOGW(TAG, "clip %u missing or invalid; degraded playback", (unsigned)clip);
        return 0;
    }

    while (hanzi_player_status(&player) == HANZI_PLAYER_PLAYING) {
        size_t samples = hanzi_player_next_chunk(&player, pcm, HANZI_ADPCM_CHUNK_SAMPLES);
        if (samples == 0U) {
            break;
        }
        int wrote = write_pcm(pcm, samples, generation);
        if (wrote <= 0) {
            return wrote;
        }
    }
    return hanzi_player_status(&player) == HANZI_PLAYER_DONE ? 1 : 0;
}

static int play_reward(uint32_t generation)
{
    static const float notes[] = {392.0f, 494.0f, 587.0f, 784.0f, 659.0f, 784.0f, 988.0f};
    const size_t note_count = sizeof(notes) / sizeof(notes[0]);
    const size_t samples_per_note = (AUDIO_RATE * REWARD_MS / 1000U) / note_count;
    int16_t pcm[HANZI_ADPCM_CHUNK_SAMPLES];

    for (size_t note = 0; note < note_count; note++) {
        for (size_t offset = 0; offset < samples_per_note; ) {
            size_t chunk = samples_per_note - offset;
            if (chunk > HANZI_ADPCM_CHUNK_SAMPLES) {
                chunk = HANZI_ADPCM_CHUNK_SAMPLES;
            }
            for (size_t i = 0; i < chunk; i++) {
                float t = (float)(offset + i) / (float)AUDIO_RATE;
                float env = 1.0f - (float)(offset + i) / (float)samples_per_note;
                float v = sinf(2.0f * (float)M_PI * notes[note] * t) * env;
                pcm[i] = (int16_t)(v * 16000.0f);
            }
            int wrote = write_pcm(pcm, chunk, generation);
            if (wrote <= 0) {
                return wrote;
            }
            offset += chunk;
        }
    }
    return 1;
}

static void audio_task(void *arg)
{
    audio_cmd_t cmd;

    (void)arg;
    for (;;) {
        if (xQueueReceive(s_queue, &cmd, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        uint32_t generation = s_generation;
        int result = 1;
        if (cmd.type == AUDIO_CMD_CANCEL) {
            continue;
        }
        if (cmd.type == AUDIO_CMD_PLAY_REWARD) {
            result = play_reward(generation);
        } else {
            result = play_clip(cmd.clip, generation);
        }
        if (generation == s_generation) {
            notify_done(result > 0);
        }
    }
}

int hanzi_audio_init(hanzi_audio_done_cb_t cb, void *user)
{
    s_done_cb = cb;
    s_done_user = user;
    s_queue = xQueueCreate(4, sizeof(audio_cmd_t));
    if (!s_queue) {
        return 0;
    }
    if (bsp_audio_init() != ESP_OK ||
        bsp_audio_set_format(AUDIO_RATE, 16, 1) != ESP_OK) {
        ESP_LOGW(TAG, "audio unavailable; continuing without sound");
        s_available = false;
        return 0;
    }
    bsp_audio_set_volume(78);
    if (xTaskCreate(audio_task, "hanzi_audio", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGW(TAG, "audio task creation failed");
        s_available = false;
        return 0;
    }
    s_available = true;
    return 1;
}

int hanzi_audio_play_view(const hanzi_view_t *view)
{
    audio_cmd_t cmd;

    if (!view || !s_available) {
        return 0;
    }
    if (view->audio_kind == HANZI_AUDIO_NONE) {
        return 0;
    }

    s_generation++;
    if (view->clip != HANZI_CLIP_NONE) {
        cmd.type = AUDIO_CMD_PLAY_CLIP;
    } else if (view->audio_kind == HANZI_AUDIO_COMPLETE) {
        cmd.type = AUDIO_CMD_PLAY_REWARD;
    } else {
        cmd.type = AUDIO_CMD_PLAY_CLIP;
    }
    cmd.clip = view->clip;
    xQueueReset(s_queue);
    xQueueSend(s_queue, &cmd, 0);
    return 1;
}

void hanzi_audio_cancel(void)
{
    audio_cmd_t cmd = {.type = AUDIO_CMD_CANCEL, .clip = HANZI_CLIP_NONE};

    if (!s_available) {
        return;
    }
    s_generation++;
    xQueueReset(s_queue);
    xQueueSend(s_queue, &cmd, 0);
}

bool hanzi_audio_available(void)
{
    return s_available;
}
