#pragma once

#include "hanzi_story.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const uint8_t *data;
    size_t size;
} hanzi_clip_blob_t;

int hanzi_clip_blob(hanzi_clip_id_t id, hanzi_clip_blob_t *blob);
