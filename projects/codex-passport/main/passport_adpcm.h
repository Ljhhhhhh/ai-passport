#pragma once

#include <stddef.h>
#include <stdint.h>

#define PASSPORT_ADPCM_MAGIC 0x34414D49U /* 'IMA4' little-endian */
#define PASSPORT_ADPCM_CHUNK_SAMPLES 512U

typedef struct {
    int32_t predictor;
    int step_index;
} passport_adpcm_state_t;

typedef struct {
    uint32_t magic;
    uint32_t sample_count;
    int16_t predictor;
    int8_t step_index;
    uint8_t reserved;
} passport_adpcm_header_t;

void passport_adpcm_init(passport_adpcm_state_t *state, int16_t predictor, int step_index);
int16_t passport_adpcm_decode_nibble(passport_adpcm_state_t *state, uint8_t nibble);
size_t passport_adpcm_decode_bytes(passport_adpcm_state_t *state, const uint8_t *src, size_t src_len,
                                   size_t nibble_index, int16_t *dst, size_t dst_samples);
int passport_adpcm_parse_clip(const uint8_t *blob, size_t blob_len, passport_adpcm_header_t *header,
                              const uint8_t **data, size_t *data_len);
