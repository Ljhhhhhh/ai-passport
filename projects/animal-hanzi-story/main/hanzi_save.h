#pragma once

#include <stddef.h>
#include <stdint.h>

#define HANZI_SAVE_MAGIC   0x57484149U /* 'IAHW' little-endian */
#define HANZI_SAVE_VERSION 2U
#define HANZI_CHAR_COUNT   7U
#define HANZI_DAYS_COUNT   3U

typedef enum {
    HANZI_CHAR_DA = 0,    /* 大 */
    HANZI_CHAR_XIAO,      /* 小 */
    HANZI_CHAR_TIAO,      /* 跳 */
    HANZI_CHAR_SHAN,      /* 山 */
    HANZI_CHAR_SHUI,      /* 水 */
    HANZI_CHAR_BA,        /* 爸 */
    HANZI_CHAR_MA         /* 妈 */
} hanzi_char_id_t;

typedef enum {
    HANZI_MASTERY_UNKNOWN = 0,  /* 未接触 */
    HANZI_MASTERY_SEEN,         /* 见过字形/听过发音 */
    HANZI_MASTERY_PROMPTED,     /* 有提示下选对 */
    HANZI_MASTERY_UNPROMPTED,   /* 无提示下二选一选对 */
    HANZI_MASTERY_DETACHED      /* 次日脱离小世界独立辨识 */
} hanzi_mastery_level_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;

    uint8_t current_day;          /* 当前天数: 1, 2, 3 */
    uint8_t day_completed[HANZI_DAYS_COUNT]; /* Day 1, Day 2, Day 3 完成标记 */

    /* 世界永久状态 */
    uint8_t world_has_river;      /* 水: 河流是否永久解锁 (Day 2 解锁) */
    uint8_t world_mountain_size;  /* 山: 0 正常, 1 巨大, 2 微小 */
    uint8_t sandbox_unlocked;     /* 自由沙盒是否解锁 (Day 3 达成后常驻) */
    uint8_t reserved1;

    /* 汉字认知与掌握进度 */
    uint8_t char_mastery[HANZI_CHAR_COUNT];  /* hanzi_mastery_level_t */
    uint8_t reserved2;
    uint16_t char_seen[HANZI_CHAR_COUNT];
    uint16_t char_correct[HANZI_CHAR_COUNT];

    uint16_t sandbox_play_count;
    uint16_t total_magic_casts;

    uint32_t crc32;
} hanzi_save_t;

uint32_t hanzi_crc32(const void *data, size_t len);
void hanzi_save_defaults(hanzi_save_t *save);
void hanzi_save_finalize(hanzi_save_t *save);
int hanzi_save_parse(const void *blob, size_t len, hanzi_save_t *save);
const char *hanzi_char_name(uint8_t char_id);
