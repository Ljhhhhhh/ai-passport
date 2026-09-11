#pragma once

#include "cards_adpcm.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
    CARDS_PLAYER_IDLE = 0,
    CARDS_PLAYER_PLAYING,
    CARDS_PLAYER_CANCELLED,
    CARDS_PLAYER_FAILED,
    CARDS_PLAYER_DONE
} cards_player_status_t;

typedef struct {
    cards_player_status_t status;
    cards_adpcm_header_t header;
    cards_adpcm_state_t state;
    const uint8_t *data;
    size_t data_len;
    size_t nibble_index;
} cards_player_t;

int cards_player_start(cards_player_t *player, const uint8_t *blob, size_t blob_len);
void cards_player_cancel(cards_player_t *player);
size_t cards_player_next_chunk(cards_player_t *player, int16_t *dst, size_t dst_samples);
cards_player_status_t cards_player_status(const cards_player_t *player);
