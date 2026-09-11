#pragma once

#include <stddef.h>
#include <stdint.h>

#include "cards_model.h"

typedef struct {
    const char *id;
    const char *hanzi;
    const char *phrase;
    const uint8_t *image;
    size_t image_len;
    const uint8_t *voice;
    size_t voice_len;
} cards_entry_t;

extern const cards_entry_t cards_catalog[CARDS_COUNT];

int cards_catalog_get(uint8_t index, const cards_entry_t **entry);
