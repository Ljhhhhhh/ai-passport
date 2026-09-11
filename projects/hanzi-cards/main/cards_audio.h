#pragma once

#include <stddef.h>
#include <stdint.h>

typedef void (*cards_audio_done_cb_t)(int success, uint32_t play_gen, void *user);

int cards_audio_init(cards_audio_done_cb_t cb, void *user);
int cards_audio_play(const uint8_t *blob, size_t blob_len, uint32_t play_gen);
void cards_audio_cancel(void);
int cards_audio_available(void);
