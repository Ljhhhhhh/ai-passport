#include "passport_adpcm.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

static void test_clipping(void)
{
    passport_adpcm_state_t encoder, decoder;
    passport_adpcm_init(&encoder, 32760, 88);
    assert(passport_adpcm_encode_nibble(&encoder, INT16_MAX) == 0);
    assert(encoder.predictor == INT16_MAX && encoder.step_index == 87);
    passport_adpcm_init(&encoder, -32760, 88);
    assert(passport_adpcm_encode_nibble(&encoder, INT16_MIN) == 8);
    assert(encoder.predictor == INT16_MIN && encoder.step_index == 87);

    passport_adpcm_init(&encoder, 0, 0);
    passport_adpcm_init(&decoder, 0, 0);
    for (unsigned i = 0; i < 4096; ++i) {
        int16_t sample = (i & 1U) ? INT16_MIN : INT16_MAX;
        uint8_t nibble = passport_adpcm_encode_nibble(&encoder, sample);
        assert(nibble < 16);
        passport_adpcm_decode_nibble(&decoder, nibble);
        assert(encoder.predictor == decoder.predictor);
        assert(encoder.step_index == decoder.step_index);
        assert(encoder.predictor >= INT16_MIN && encoder.predictor <= INT16_MAX);
        assert(encoder.step_index >= 0 && encoder.step_index <= 88);
    }
    /* Defensive index/predictor normalization cannot overflow before clipping. */
    encoder = (passport_adpcm_state_t){ INT32_MAX, 999 };
    assert(passport_adpcm_encode_nibble(&encoder, INT16_MAX) == 0);
    assert(encoder.predictor == INT16_MAX && encoder.step_index == 87);
    encoder = (passport_adpcm_state_t){ INT32_MIN, -100 };
    assert(passport_adpcm_encode_nibble(&encoder, INT16_MIN) == 0);
    assert(encoder.predictor == INT16_MIN && encoder.step_index == 0);
    assert(passport_adpcm_encode_nibble(NULL, 0) == 0);
}

int main(void)
{
    static const int16_t vector[] = {0, 1200, 2400, 1800, 400, -900, -1800, -600, 0, 700};
    static const int16_t expected[] = {0, 11, 41, 104, 240, -53, -684, -594, -19, 653};
    static const uint8_t expected_bytes[] = {0x70, 0x77, 0xF7, 0x0F, 0x43};
    uint8_t packed[(sizeof(vector) / sizeof(vector[0]) + 1) / 2];
    uint8_t blob[sizeof(passport_adpcm_header_t) + sizeof(packed)];
    passport_adpcm_state_t encode_state;
    passport_adpcm_state_t decode_state;
    passport_adpcm_header_t header;
    const uint8_t *data;
    size_t data_len;
    int16_t decoded[sizeof(vector) / sizeof(vector[0])];
    size_t i;

    memset(packed, 0, sizeof(packed));
    passport_adpcm_init(&encode_state, 0, 0);
    for (i = 0; i < sizeof(vector) / sizeof(vector[0]); i++) {
        uint8_t nibble = passport_adpcm_encode_nibble(&encode_state, vector[i]);
        if ((i & 1U) == 0U) {
            packed[i / 2U] = nibble;
        } else {
            packed[i / 2U] |= (uint8_t)(nibble << 4);
        }
    }
    assert(memcmp(packed, expected_bytes, sizeof(packed)) == 0);

    memset(&header, 0, sizeof(header));
    header.magic = PASSPORT_ADPCM_MAGIC;
    header.sample_count = (uint32_t)(sizeof(vector) / sizeof(vector[0]));
    header.predictor = 0;
    header.step_index = 0;
    memcpy(blob, &header, sizeof(header));
    memcpy(blob + sizeof(header), packed, sizeof(packed));

    assert(passport_adpcm_parse_clip(blob, sizeof(blob), &header, &data, &data_len));
    passport_adpcm_init(&decode_state, header.predictor, header.step_index);
    assert(passport_adpcm_decode_bytes(&decode_state, data, data_len, 0, decoded,
                                      sizeof(decoded) / sizeof(decoded[0])) ==
           sizeof(decoded) / sizeof(decoded[0]));
    assert(memcmp(decoded, expected, sizeof(expected)) == 0);

    /* Resume a stream across an odd boundary; the final padded nibble is not a sample. */
    passport_adpcm_init(&decode_state, 0, 0);
    assert(passport_adpcm_decode_bytes(&decode_state, packed, sizeof(packed), 0, decoded, 3) == 3);
    assert(passport_adpcm_decode_bytes(&decode_state, packed, sizeof(packed), 3, decoded + 3, 6) == 6);
    assert(memcmp(decoded, expected, 9 * sizeof(*decoded)) == 0);

    blob[0] ^= 0xFF;
    assert(!passport_adpcm_parse_clip(blob, sizeof(blob), &header, &data, &data_len));
    test_clipping();
    return 0;
}
