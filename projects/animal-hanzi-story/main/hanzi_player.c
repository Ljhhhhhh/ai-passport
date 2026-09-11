#include "hanzi_player.h"

#include <string.h>

int hanzi_player_start(hanzi_player_t *player, const uint8_t *blob, size_t blob_len)
{
    const uint8_t *data = NULL;
    size_t data_len = 0;

    if (!player) {
        return 0;
    }

    memset(player, 0, sizeof(*player));
    if (!hanzi_adpcm_parse_clip(blob, blob_len, &player->header, &data, &data_len)) {
        player->status = HANZI_PLAYER_FAILED;
        return 0;
    }

    hanzi_adpcm_init(&player->state, player->header.predictor, player->header.step_index);
    player->data = data;
    player->data_len = data_len;
    player->nibble_index = 0;
    player->status = HANZI_PLAYER_PLAYING;
    return 1;
}

void hanzi_player_cancel(hanzi_player_t *player)
{
    if (!player) {
        return;
    }
    player->status = HANZI_PLAYER_CANCELLED;
}

size_t hanzi_player_next_chunk(hanzi_player_t *player, int16_t *dst, size_t dst_samples)
{
    size_t remaining;
    size_t produced;

    if (!player || player->status != HANZI_PLAYER_PLAYING || !dst || dst_samples == 0U) {
        return 0;
    }

    remaining = player->header.sample_count - player->nibble_index;
    if (remaining == 0U) {
        player->status = HANZI_PLAYER_DONE;
        return 0;
    }
    if (dst_samples > remaining) {
        dst_samples = remaining;
    }

    produced = hanzi_adpcm_decode_bytes(&player->state, player->data, player->data_len,
                                        player->nibble_index, dst, dst_samples);
    player->nibble_index += produced;
    if (produced == 0U) {
        player->status = HANZI_PLAYER_FAILED;
        return 0;
    }
    if (player->nibble_index >= player->header.sample_count) {
        player->status = HANZI_PLAYER_DONE;
    }
    return produced;
}

hanzi_player_status_t hanzi_player_status(const hanzi_player_t *player)
{
    return player ? player->status : HANZI_PLAYER_FAILED;
}
