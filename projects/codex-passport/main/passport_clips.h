#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum {
    PASSPORT_CLIP_NONE = 0,
    PASSPORT_CLIP_DONE,
    PASSPORT_CLIP_WAIT,
    PASSPORT_CLIP_ERROR,
    PASSPORT_CLIP_NEW_MSG,
} passport_clip_id_t;

typedef struct {
    const uint8_t *data;
    size_t size;
} passport_clip_blob_t;

int passport_clip_blob(passport_clip_id_t id, passport_clip_blob_t *blob);
