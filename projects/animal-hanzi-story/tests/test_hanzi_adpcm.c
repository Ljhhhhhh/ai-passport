#include "hanzi_adpcm.h"
#include "hanzi_player.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

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
    if (value > 32767) {
        return 32767;
    }
    if (value < -32768) {
        return -32768;
    }
    return (int16_t)value;
}

static uint8_t encode_nibble(hanzi_adpcm_state_t *state, int16_t sample)
{
    int step = s_step_table[state->step_index];
    int32_t diff = (int32_t)sample - state->predictor;
    uint8_t nibble = 0;
    int32_t predicted;

    if (diff < 0) {
        nibble = 8;
        diff = -diff;
    }
    if (diff >= step) {
        nibble |= 4;
        diff -= step;
    }
    if (diff >= (step >> 1)) {
        nibble |= 2;
        diff -= step >> 1;
    }
    if (diff >= (step >> 2)) {
        nibble |= 1;
    }

    predicted = step >> 3;
    if (nibble & 4U) {
        predicted += step;
    }
    if (nibble & 2U) {
        predicted += step >> 1;
    }
    if (nibble & 1U) {
        predicted += step >> 2;
    }
    if (nibble & 8U) {
        state->predictor -= predicted;
    } else {
        state->predictor += predicted;
    }
    state->predictor = clamp_i16(state->predictor);
    state->step_index += s_index_table[nibble];
    if (state->step_index < 0) {
        state->step_index = 0;
    }
    if (state->step_index > 88) {
        state->step_index = 88;
    }
    return nibble;
}

int main(void)
{
    static const int16_t vector[] = {0, 1200, 2400, 1800, 400, -900, -1800, -600, 0, 700};
    static const int16_t expected[] = {0, 11, 41, 104, 240, -53, -684, -594, -19, 653};
    uint8_t packed[(sizeof(vector) / sizeof(vector[0]) + 1) / 2];
    uint8_t blob[sizeof(hanzi_adpcm_header_t) + sizeof(packed)];
    hanzi_adpcm_state_t encode_state;
    hanzi_adpcm_state_t decode_state;
    hanzi_adpcm_header_t header;
    const uint8_t *data;
    size_t data_len;
    int16_t decoded[sizeof(vector) / sizeof(vector[0])];
    int16_t chunk[4];
    hanzi_player_t player;
    size_t i;

    memset(packed, 0, sizeof(packed));
    hanzi_adpcm_init(&encode_state, 0, 0);
    for (i = 0; i < sizeof(vector) / sizeof(vector[0]); i++) {
        uint8_t nibble = encode_nibble(&encode_state, vector[i]);
        if ((i & 1U) == 0U) {
            packed[i / 2U] = nibble;
        } else {
            packed[i / 2U] |= (uint8_t)(nibble << 4);
        }
    }

    memset(&header, 0, sizeof(header));
    header.magic = HANZI_ADPCM_MAGIC;
    header.sample_count = (uint32_t)(sizeof(vector) / sizeof(vector[0]));
    header.predictor = 0;
    header.step_index = 0;
    memcpy(blob, &header, sizeof(header));
    memcpy(blob + sizeof(header), packed, sizeof(packed));

    assert(hanzi_adpcm_parse_clip(blob, sizeof(blob), &header, &data, &data_len));
    hanzi_adpcm_init(&decode_state, header.predictor, header.step_index);
    assert(hanzi_adpcm_decode_bytes(&decode_state, data, data_len, 0, decoded,
                                    sizeof(decoded) / sizeof(decoded[0])) ==
           sizeof(decoded) / sizeof(decoded[0]));
    assert(memcmp(decoded, expected, sizeof(expected)) == 0);

    assert(hanzi_player_start(&player, blob, sizeof(blob)));
    assert(hanzi_player_next_chunk(&player, chunk, 4) == 4);
    assert(hanzi_player_status(&player) == HANZI_PLAYER_PLAYING);
    hanzi_player_cancel(&player);
    assert(hanzi_player_status(&player) == HANZI_PLAYER_CANCELLED);
    assert(hanzi_player_next_chunk(&player, chunk, 4) == 0);

    assert(hanzi_player_start(&player, blob, sizeof(blob)));
    size_t total = 0;
    while (hanzi_player_status(&player) == HANZI_PLAYER_PLAYING) {
        total += hanzi_player_next_chunk(&player, chunk, 3);
    }
    assert(total == header.sample_count);
    assert(hanzi_player_status(&player) == HANZI_PLAYER_DONE);

    blob[0] ^= 0xFF;
    assert(!hanzi_player_start(&player, blob, sizeof(blob)));
    assert(hanzi_player_status(&player) == HANZI_PLAYER_FAILED);
    return 0;
}
