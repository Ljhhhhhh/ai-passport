#pragma once

#include "hanzi_adpcm.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
    HANZI_PLAYER_IDLE = 0,
    HANZI_PLAYER_PLAYING,
    HANZI_PLAYER_CANCELLED,
    HANZI_PLAYER_FAILED,
    HANZI_PLAYER_DONE
} hanzi_player_status_t;

typedef struct {
    hanzi_player_status_t status;
    hanzi_adpcm_header_t header;
    hanzi_adpcm_state_t state;
    const uint8_t *data;
    size_t data_len;
    size_t nibble_index;
} hanzi_player_t;

int hanzi_player_start(hanzi_player_t *player, const uint8_t *blob, size_t blob_len);
void hanzi_player_cancel(hanzi_player_t *player);
size_t hanzi_player_next_chunk(hanzi_player_t *player, int16_t *dst, size_t dst_samples);
hanzi_player_status_t hanzi_player_status(const hanzi_player_t *player);
