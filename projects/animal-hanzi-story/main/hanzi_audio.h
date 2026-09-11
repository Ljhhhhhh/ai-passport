#pragma once

#include "hanzi_story.h"

#include <stdbool.h>

typedef void (*hanzi_audio_done_cb_t)(int success, void *user);

int hanzi_audio_init(hanzi_audio_done_cb_t cb, void *user);
int hanzi_audio_play_view(const hanzi_view_t *view);
void hanzi_audio_cancel(void);
bool hanzi_audio_available(void);
