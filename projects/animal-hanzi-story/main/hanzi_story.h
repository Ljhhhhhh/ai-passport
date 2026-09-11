#pragma once

#include "hanzi_save.h"

#include <stddef.h>
#include <stdint.h>

#define HANZI_IDLE_HINT_MS          8000U
#define HANZI_SANDBOX_DURATION_MS   60000U

typedef enum {
    HANZI_STATE_INTRO = 0,        /* 开场引入与今日目标 */
    HANZI_STATE_PRACTICE_JUMP,    /* Day 1: 练习【跳】(按 OK) */
    HANZI_STATE_PRACTICE_SCALE,   /* Day 1: 练习【大/小】(按 UP/DOWN) */
    HANZI_STATE_LEARN_CARD,       /* Day 2: 展示新字【水】 */
    HANZI_STATE_DIRECT_CHOICE,    /* 幼儿直接二选一 (UP 选上 / DOWN 选下) */
    HANZI_STATE_FEEDBACK,         /* 二选一正向/鼓励反馈 */
    HANZI_STATE_MAGIC_EFFECT,     /* 施法动效 (如 Day 2 河流奔涌注入) */
    HANZI_STATE_DETACHED_TEST,    /* Day 3: 白底脱离场景辨认 */
    HANZI_STATE_SANDBOX,          /* Day 3: 60秒自由魔法沙盒 */
    HANZI_STATE_DAY_COMPLETE      /* 今日完成，休息画面 */
} hanzi_state_t;

typedef enum {
    HANZI_BTN_UP = 0,
    HANZI_BTN_DOWN,
    HANZI_BTN_OK
} hanzi_btn_t;

typedef enum {
    HANZI_AUDIO_NONE = 0,
    HANZI_AUDIO_STORY,
    HANZI_AUDIO_PROMPT,
    HANZI_AUDIO_CHAR,
    HANZI_AUDIO_MAGIC,
    HANZI_AUDIO_HINT,
    HANZI_AUDIO_COMPLETE
} hanzi_audio_kind_t;

typedef enum {
    HANZI_SPRITE_WORLD_BASE = 0,
    HANZI_SPRITE_WORLD_BIG,
    HANZI_SPRITE_WORLD_SMALL,
    HANZI_SPRITE_WORLD_JUMP,
    HANZI_SPRITE_WORLD_RIVER,
    HANZI_SPRITE_WORLD_RIVER_BIG,
    HANZI_SPRITE_WORLD_RIVER_SMALL,
    HANZI_SPRITE_WORLD_RIVER_JUMP,
    HANZI_SPRITE_CARD_DETACHED,
    HANZI_SPRITE_BEDTIME
} hanzi_sprite_id_t;

typedef enum {
    HANZI_CLIP_NONE = 0,
    HANZI_CLIP_DAY1_INTRO,
    HANZI_CLIP_DAY1_JUMP_PROMPT,
    HANZI_CLIP_DAY1_SCALE_PROMPT,
    HANZI_CLIP_DAY1_CHOICE_PROMPT,
    HANZI_CLIP_DAY1_COMPLETE,
    HANZI_CLIP_DAY2_INTRO,
    HANZI_CLIP_DAY2_LEARN_WATER,
    HANZI_CLIP_DAY2_CHOICE_PROMPT,
    HANZI_CLIP_DAY2_WATER_FLOW,
    HANZI_CLIP_DAY2_COMPLETE,
    HANZI_CLIP_DAY3_INTRO,
    HANZI_CLIP_DAY3_TEST_PROMPT,
    HANZI_CLIP_DAY3_TEST_CORRECT,
    HANZI_CLIP_DAY3_SANDBOX_INTRO,
    HANZI_CLIP_DAY3_COMPLETE,
    HANZI_CLIP_CHAR_DA,
    HANZI_CLIP_CHAR_XIAO,
    HANZI_CLIP_CHAR_TIAO,
    HANZI_CLIP_CHAR_SHAN,
    HANZI_CLIP_CHAR_SHUI,
    HANZI_CLIP_CHAR_BA,
    HANZI_CLIP_CHAR_MA,
    HANZI_CLIP_MAGIC_BIG,
    HANZI_CLIP_MAGIC_SMALL,
    HANZI_CLIP_MAGIC_JUMP,
    HANZI_CLIP_MAGIC_WATER,
    HANZI_CLIP_AGAIN,
    HANZI_CLIP_HINT_UP_DOWN,
    HANZI_CLIP_COUNT
} hanzi_clip_id_t;

typedef struct {
    hanzi_state_t state;
    uint8_t current_day;
    const char *title;
    const char *focus_hanzi;
    const char *prompt;
    const char *choice_top;
    const char *choice_bottom;
    uint8_t selected_choice;      /* 0: 无, 1: 上, 2: 下 */
    uint8_t correct_choice;       /* 1: 上, 2: 下 */
    uint8_t show_choices;
    uint8_t detached_mode;        /* 1: 白底纯字脱离测试 */
    uint8_t highlight_correct;
    uint8_t show_ok_hint;
    uint8_t show_choice_hint;
    uint8_t character_state;      /* 0: 正常, 1: 大, 2: 小, 3: 跳 */
    uint8_t world_has_river;
    uint8_t world_mountain_size;  /* 0: 正常, 1: 大, 2: 小 */
    uint8_t sandbox_active;
    uint8_t sandbox_seconds_left;
    hanzi_sprite_id_t sprite;
    hanzi_clip_id_t clip;
    hanzi_audio_kind_t audio_kind;
} hanzi_view_t;

typedef struct {
    hanzi_state_t state;
    uint8_t current_day;
    uint8_t practice_step;
    uint8_t selected_choice;
    uint8_t fail_count;
    uint8_t target_char_id;
    uint8_t distractor_char_id;
    uint8_t correct_is_top;
    uint8_t character_state;
    uint8_t world_mountain_size;
    uint8_t pending_save;
    uint8_t audio_busy;
    uint8_t audio_failed;
    hanzi_audio_kind_t audio_kind;
    hanzi_clip_id_t clip;
    uint32_t state_ms;
    uint32_t sandbox_ms;
    uint32_t idle_ms;
    hanzi_save_t save;
} hanzi_story_t;

void hanzi_story_init(hanzi_story_t *story, const hanzi_save_t *save);
void hanzi_story_on_button(hanzi_story_t *story, hanzi_btn_t button);
void hanzi_story_on_audio_done(hanzi_story_t *story, int success);
void hanzi_story_on_tick(hanzi_story_t *story, uint32_t elapsed_ms);
void hanzi_story_get_view(const hanzi_story_t *story, hanzi_view_t *view);
int hanzi_story_consume_save(hanzi_story_t *story, hanzi_save_t *out);
