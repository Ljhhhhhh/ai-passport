#include "hanzi_clips.h"

#define DECLARE_CLIP(sym) \
    extern const uint8_t s_##sym##_start[] asm("_binary_" #sym "_ima_start"); \
    extern const uint8_t s_##sym##_end[] asm("_binary_" #sym "_ima_end");

#define CLIP_ENTRY(id, sym) \
    case (id): \
        blob->data = s_##sym##_start; \
        blob->size = (size_t)(s_##sym##_end - s_##sym##_start); \
        return blob->size > 0U;

DECLARE_CLIP(day1_intro)
DECLARE_CLIP(day1_jump_prompt)
DECLARE_CLIP(day1_scale_prompt)
DECLARE_CLIP(day1_choice_prompt)
DECLARE_CLIP(day1_complete)
DECLARE_CLIP(day2_intro)
DECLARE_CLIP(day2_learn_water)
DECLARE_CLIP(day2_choice_prompt)
DECLARE_CLIP(day2_water_flow)
DECLARE_CLIP(day2_complete)
DECLARE_CLIP(day3_intro)
DECLARE_CLIP(day3_test_prompt)
DECLARE_CLIP(day3_test_correct)
DECLARE_CLIP(day3_sandbox_intro)
DECLARE_CLIP(day3_complete)
DECLARE_CLIP(char_da)
DECLARE_CLIP(char_xiao)
DECLARE_CLIP(char_tiao)
DECLARE_CLIP(char_shan)
DECLARE_CLIP(char_shui)
DECLARE_CLIP(char_ba)
DECLARE_CLIP(char_ma)
DECLARE_CLIP(magic_big)
DECLARE_CLIP(magic_small)
DECLARE_CLIP(magic_jump)
DECLARE_CLIP(magic_water)
DECLARE_CLIP(again)
DECLARE_CLIP(hint_up_down)

int hanzi_clip_blob(hanzi_clip_id_t id, hanzi_clip_blob_t *blob)
{
    if (!blob) {
        return 0;
    }
    blob->data = NULL;
    blob->size = 0;
    switch (id) {
    CLIP_ENTRY(HANZI_CLIP_DAY1_INTRO, day1_intro)
    CLIP_ENTRY(HANZI_CLIP_DAY1_JUMP_PROMPT, day1_jump_prompt)
    CLIP_ENTRY(HANZI_CLIP_DAY1_SCALE_PROMPT, day1_scale_prompt)
    CLIP_ENTRY(HANZI_CLIP_DAY1_CHOICE_PROMPT, day1_choice_prompt)
    CLIP_ENTRY(HANZI_CLIP_DAY1_COMPLETE, day1_complete)
    CLIP_ENTRY(HANZI_CLIP_DAY2_INTRO, day2_intro)
    CLIP_ENTRY(HANZI_CLIP_DAY2_LEARN_WATER, day2_learn_water)
    CLIP_ENTRY(HANZI_CLIP_DAY2_CHOICE_PROMPT, day2_choice_prompt)
    CLIP_ENTRY(HANZI_CLIP_DAY2_WATER_FLOW, day2_water_flow)
    CLIP_ENTRY(HANZI_CLIP_DAY2_COMPLETE, day2_complete)
    CLIP_ENTRY(HANZI_CLIP_DAY3_INTRO, day3_intro)
    CLIP_ENTRY(HANZI_CLIP_DAY3_TEST_PROMPT, day3_test_prompt)
    CLIP_ENTRY(HANZI_CLIP_DAY3_TEST_CORRECT, day3_test_correct)
    CLIP_ENTRY(HANZI_CLIP_DAY3_SANDBOX_INTRO, day3_sandbox_intro)
    CLIP_ENTRY(HANZI_CLIP_DAY3_COMPLETE, day3_complete)
    CLIP_ENTRY(HANZI_CLIP_CHAR_DA, char_da)
    CLIP_ENTRY(HANZI_CLIP_CHAR_XIAO, char_xiao)
    CLIP_ENTRY(HANZI_CLIP_CHAR_TIAO, char_tiao)
    CLIP_ENTRY(HANZI_CLIP_CHAR_SHAN, char_shan)
    CLIP_ENTRY(HANZI_CLIP_CHAR_SHUI, char_shui)
    CLIP_ENTRY(HANZI_CLIP_CHAR_BA, char_ba)
    CLIP_ENTRY(HANZI_CLIP_CHAR_MA, char_ma)
    CLIP_ENTRY(HANZI_CLIP_MAGIC_BIG, magic_big)
    CLIP_ENTRY(HANZI_CLIP_MAGIC_SMALL, magic_small)
    CLIP_ENTRY(HANZI_CLIP_MAGIC_JUMP, magic_jump)
    CLIP_ENTRY(HANZI_CLIP_MAGIC_WATER, magic_water)
    CLIP_ENTRY(HANZI_CLIP_AGAIN, again)
    CLIP_ENTRY(HANZI_CLIP_HINT_UP_DOWN, hint_up_down)
    default:
        return 0;
    }
}
