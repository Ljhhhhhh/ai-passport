#pragma once

#include <stddef.h>
#include <stdint.h>

#define HANZI_ADPCM_MAGIC 0x34414D49U /* 'IMA4' little-endian */
#define HANZI_ADPCM_CHUNK_SAMPLES 512U

typedef struct {
    int32_t predictor;
    int step_index;
} hanzi_adpcm_state_t;

typedef struct {
    uint32_t magic;
    uint32_t sample_count;
    int16_t predictor;
    int8_t step_index;
    uint8_t reserved;
} hanzi_adpcm_header_t;

void hanzi_adpcm_init(hanzi_adpcm_state_t *state, int16_t predictor, int step_index);
int16_t hanzi_adpcm_decode_nibble(hanzi_adpcm_state_t *state, uint8_t nibble);
size_t hanzi_adpcm_decode_bytes(hanzi_adpcm_state_t *state, const uint8_t *src, size_t src_len,
                                size_t nibble_index, int16_t *dst, size_t dst_samples);
int hanzi_adpcm_parse_clip(const uint8_t *blob, size_t blob_len, hanzi_adpcm_header_t *header,
                           const uint8_t **data, size_t *data_len);
