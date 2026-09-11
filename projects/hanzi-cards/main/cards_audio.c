#include "cards_audio.h"

#include "bsp_audio.h"
#include "cards_adpcm.h"
#include "cards_player.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define AUDIO_RATE 16000U
#define AUDIO_VOLUME 70U

typedef enum {
    AUDIO_CMD_PLAY = 0,
    AUDIO_CMD_CANCEL
} audio_cmd_type_t;

typedef struct {
    audio_cmd_type_t type;
    const uint8_t *blob;
    size_t blob_len;
    uint32_t play_gen;
} audio_cmd_t;

static const char *TAG = "cards_audio";
static QueueHandle_t s_queue;
static cards_audio_done_cb_t s_done_cb;
static void *s_done_user;
static volatile uint32_t s_generation;
static int s_available;

static void notify_done(int success, uint32_t play_gen)
{
    if (s_done_cb) {
        s_done_cb(success, play_gen, s_done_user);
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

static int play_blob(const uint8_t *blob, size_t blob_len, uint32_t generation)
{
    cards_player_t player;
    int16_t pcm[CARDS_ADPCM_CHUNK_SAMPLES];

    if (!blob || blob_len == 0U || !cards_player_start(&player, blob, blob_len)) {
        ESP_LOGW(TAG, "voice clip missing or invalid; silent fail");
        return 0;
    }

    while (cards_player_status(&player) == CARDS_PLAYER_PLAYING) {
        size_t samples = cards_player_next_chunk(&player, pcm, CARDS_ADPCM_CHUNK_SAMPLES);
        if (samples == 0U) {
            break;
        }
        int wrote = write_pcm(pcm, samples, generation);
        if (wrote <= 0) {
            return wrote;
        }
    }
    return cards_player_status(&player) == CARDS_PLAYER_DONE ? 1 : 0;
}

static void audio_task(void *arg)
{
    audio_cmd_t cmd;

    (void)arg;
    for (;;) {
        if (xQueueReceive(s_queue, &cmd, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (cmd.type == AUDIO_CMD_CANCEL) {
            continue;
        }
        uint32_t generation = s_generation;
        int result = play_blob(cmd.blob, cmd.blob_len, generation);
        if (generation == s_generation) {
            notify_done(result > 0, cmd.play_gen);
        }
    }
}

int cards_audio_init(cards_audio_done_cb_t cb, void *user)
{
    s_done_cb = cb;
    s_done_user = user;
    s_queue = xQueueCreate(4, sizeof(audio_cmd_t));
    if (!s_queue) {
        return 0;
    }
    if (bsp_audio_init() != ESP_OK ||
        bsp_audio_set_format(AUDIO_RATE, 16, 1) != ESP_OK) {
        ESP_LOGW(TAG, "audio unavailable; cards stay browsable");
        s_available = 0;
        return 0;
    }
    bsp_audio_set_volume(AUDIO_VOLUME);
    if (xTaskCreate(audio_task, "cards_audio", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGW(TAG, "audio task creation failed");
        s_available = 0;
        return 0;
    }
    s_available = 1;
    return 1;
}

int cards_audio_play(const uint8_t *blob, size_t blob_len, uint32_t play_gen)
{
    audio_cmd_t cmd = {
        .type = AUDIO_CMD_PLAY,
        .blob = blob,
        .blob_len = blob_len,
        .play_gen = play_gen
    };

    if (!s_available) {
        return 0;
    }
    s_generation++;
    xQueueReset(s_queue);
    return xQueueSend(s_queue, &cmd, 0) == pdTRUE;
}

void cards_audio_cancel(void)
{
    audio_cmd_t cmd = {
        .type = AUDIO_CMD_CANCEL,
        .blob = NULL,
        .blob_len = 0,
        .play_gen = 0
    };

    if (!s_available) {
        return;
    }
    s_generation++;
    xQueueReset(s_queue);
    xQueueSend(s_queue, &cmd, 0);
}

int cards_audio_available(void)
{
    return s_available;
}
