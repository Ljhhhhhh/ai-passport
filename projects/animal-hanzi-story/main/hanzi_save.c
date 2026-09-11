#include "hanzi_save.h"

#include <string.h>

static const char * const s_char_names[HANZI_CHAR_COUNT] = {
    "大", "小", "跳", "山", "水", "爸", "妈"
};

const char *hanzi_char_name(uint8_t char_id)
{
    if (char_id < HANZI_CHAR_COUNT) {
        return s_char_names[char_id];
    }
    return "?";
}

uint32_t hanzi_crc32(const void *data, size_t len)
{
    const uint8_t *bytes = data;
    uint32_t crc = 0xFFFFFFFFU;

    if (!bytes) {
        return 0U;
    }

    for (size_t i = 0; i < len; i++) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xEDB88320U & (uint32_t)-(int32_t)(crc & 1));
        }
    }
    return ~crc;
}

void hanzi_save_defaults(hanzi_save_t *save)
{
    if (!save) {
        return;
    }
    memset(save, 0, sizeof(*save));
    save->magic = HANZI_SAVE_MAGIC;
    save->version = HANZI_SAVE_VERSION;
    save->size = (uint16_t)sizeof(*save);
    save->current_day = 1;
    save->world_has_river = 0;
    save->world_mountain_size = 0;
    save->sandbox_unlocked = 0;
    hanzi_save_finalize(save);
}

void hanzi_save_finalize(hanzi_save_t *save)
{
    if (!save) {
        return;
    }
    save->magic = HANZI_SAVE_MAGIC;
    save->version = HANZI_SAVE_VERSION;
    save->size = (uint16_t)sizeof(*save);
    save->crc32 = hanzi_crc32(save, offsetof(hanzi_save_t, crc32));
}

int hanzi_save_parse(const void *blob, size_t len, hanzi_save_t *save)
{
    hanzi_save_t parsed;

    if (!blob || !save || len < sizeof(hanzi_save_t)) {
        return 0;
    }

    memcpy(&parsed, blob, sizeof(parsed));
    if (parsed.magic != HANZI_SAVE_MAGIC ||
        parsed.version != HANZI_SAVE_VERSION ||
        parsed.size != sizeof(parsed) ||
        parsed.crc32 != hanzi_crc32(&parsed, offsetof(hanzi_save_t, crc32))) {
        return 0;
    }
    if (parsed.current_day == 0 || parsed.current_day > HANZI_DAYS_COUNT) {
        return 0;
    }

    *save = parsed;
    return 1;
}
