#pragma once

#include <stddef.h>
#include <stdint.h>

#define CARDS_ADPCM_MAGIC 0x34414D49U /* 'IMA4' little-endian */
#define CARDS_ADPCM_CHUNK_SAMPLES 512U

typedef struct {
    int32_t predictor;
    int step_index;
} cards_adpcm_state_t;

typedef struct {
    uint32_t magic;
    uint32_t sample_count;
    int16_t predictor;
    int8_t step_index;
    uint8_t reserved;
} cards_adpcm_header_t;

void cards_adpcm_init(cards_adpcm_state_t *state, int16_t predictor, int step_index);
int16_t cards_adpcm_decode_nibble(cards_adpcm_state_t *state, uint8_t nibble);
size_t cards_adpcm_decode_bytes(cards_adpcm_state_t *state, const uint8_t *src, size_t src_len,
                                size_t nibble_index, int16_t *dst, size_t dst_samples);
int cards_adpcm_parse_clip(const uint8_t *blob, size_t blob_len, cards_adpcm_header_t *header,
                           const uint8_t **data, size_t *data_len);
