#include "passport_adpcm.h"

#include <string.h>

_Static_assert(sizeof(passport_adpcm_header_t) == 12U, "ADPCM clip header must stay stable");

static const int s_step_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

static const int s_index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static int16_t clamp_i16(int32_t value)
{
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (int16_t)value;
}

void passport_adpcm_init(passport_adpcm_state_t *state, int16_t predictor, int step_index)
{
    if (!state) return;
    state->predictor = predictor;
    state->step_index = step_index;
    if (state->step_index < 0) state->step_index = 0;
    if (state->step_index > 88) state->step_index = 88;
}

int16_t passport_adpcm_decode_nibble(passport_adpcm_state_t *state, uint8_t nibble)
{
    int step;
    int32_t diff;

    nibble &= 0x0FU;
    if (state->step_index < 0) state->step_index = 0;
    if (state->step_index > 88) state->step_index = 88;

    step = s_step_table[state->step_index];
    diff = step >> 3;
    if (nibble & 4U) diff += step;
    if (nibble & 2U) diff += step >> 1;
    if (nibble & 1U) diff += step >> 2;
    if (nibble & 8U) {
        state->predictor -= diff;
    } else {
        state->predictor += diff;
    }
    state->predictor = clamp_i16(state->predictor);

    state->step_index += s_index_table[nibble];
    if (state->step_index < 0) state->step_index = 0;
    if (state->step_index > 88) state->step_index = 88;
    return (int16_t)state->predictor;
}

size_t passport_adpcm_decode_bytes(passport_adpcm_state_t *state, const uint8_t *src, size_t src_len,
                                   size_t nibble_index, int16_t *dst, size_t dst_samples)
{
    size_t produced = 0;
    if (!state || !src || !dst || dst_samples == 0U) return 0;

    while (produced < dst_samples) {
        size_t byte_index = nibble_index / 2U;
        uint8_t nibble;

        if (byte_index >= src_len) break;
        if ((nibble_index & 1U) == 0U) {
            nibble = src[byte_index] & 0x0FU;
        } else {
            nibble = (uint8_t)(src[byte_index] >> 4);
        }
        dst[produced++] = passport_adpcm_decode_nibble(state, nibble);
        nibble_index++;
    }
    return produced;
}

int passport_adpcm_parse_clip(const uint8_t *blob, size_t blob_len, passport_adpcm_header_t *header,
                              const uint8_t **data, size_t *data_len)
{
    passport_adpcm_header_t parsed;
    size_t packed_len;

    if (!blob || blob_len < sizeof(passport_adpcm_header_t) || !header || !data || !data_len) {
        return 0;
    }

    memcpy(&parsed, blob, sizeof(parsed));
    if (parsed.magic != PASSPORT_ADPCM_MAGIC || parsed.sample_count == 0U) {
        return 0;
    }
    if (parsed.step_index < 0 || parsed.step_index > 88) {
        return 0;
    }

    packed_len = ((size_t)parsed.sample_count + 1U) / 2U;
    if (blob_len < sizeof(passport_adpcm_header_t) + packed_len) {
        return 0;
    }

    *header = parsed;
    *data = blob + sizeof(passport_adpcm_header_t);
    *data_len = packed_len;
    return 1;
}
