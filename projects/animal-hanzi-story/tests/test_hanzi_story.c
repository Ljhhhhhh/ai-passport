#include "hanzi_story.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

static void test_day1_flow(void)
{
    hanzi_story_t story;
    hanzi_view_t view;
    hanzi_save_t save;

    hanzi_story_init(&story, NULL);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_INTRO);
    assert(view.current_day == 1);
    assert(view.world_has_river == 0);

    /* Intro -> Practice Jump */
    hanzi_story_on_button(&story, HANZI_BTN_OK);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_PRACTICE_JUMP);
    assert(view.character_state == 0);

    /* Press OK -> Jump */
    hanzi_story_on_button(&story, HANZI_BTN_OK);
    hanzi_story_get_view(&story, &view);
    assert(view.character_state == 3); /* Jumping */

    /* Tick 600ms -> Recover & move to Practice Scale */
    hanzi_story_on_tick(&story, 600U);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_PRACTICE_SCALE);
    assert(view.character_state == 0);

    /* Press UP -> Big */
    hanzi_story_on_button(&story, HANZI_BTN_UP);
    hanzi_story_get_view(&story, &view);
    assert(view.character_state == 1);
    assert(view.world_mountain_size == 1);

    /* Press DOWN -> Small */
    hanzi_story_on_button(&story, HANZI_BTN_DOWN);
    hanzi_story_get_view(&story, &view);
    assert(view.character_state == 2);
    assert(view.world_mountain_size == 2);

    /* Press OK -> Direct Choice */
    hanzi_story_on_button(&story, HANZI_BTN_OK);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_DIRECT_CHOICE);
    assert(view.show_choices == 1);
    assert(strcmp(view.choice_top, "大") == 0);
    assert(strcmp(view.choice_bottom, "小") == 0);

    /* Direct Choice: press UP to choose Top (大) -> Correct */
    hanzi_story_on_button(&story, HANZI_BTN_UP);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_FEEDBACK);
    assert(story.save.char_mastery[HANZI_CHAR_DA] == HANZI_MASTERY_UNPROMPTED);
    assert(story.save.char_correct[HANZI_CHAR_DA] == 1);

    /* Audio finishes feedback -> Day Complete */
    hanzi_story_on_audio_done(&story, 1);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_DAY_COMPLETE);
    assert(story.save.day_completed[0] == 1);
    assert(story.save.current_day == 2);

    assert(hanzi_story_consume_save(&story, &save));
    assert(save.day_completed[0] == 1);
    assert(save.current_day == 2);
}

static void test_day2_flow(void)
{
    hanzi_story_t story;
    hanzi_view_t view;
    hanzi_save_t save;

    /* Start with Day 2 save */
    hanzi_save_defaults(&save);
    save.day_completed[0] = 1;
    save.current_day = 2;
    hanzi_save_finalize(&save);

    hanzi_story_init(&story, &save);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_INTRO);
    assert(view.current_day == 2);
    assert(view.world_has_river == 0);

    /* Intro -> Learn Card "水" */
    hanzi_story_on_button(&story, HANZI_BTN_OK);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_LEARN_CARD);
    assert(strcmp(view.focus_hanzi, "水") == 0);
    assert(story.save.char_mastery[HANZI_CHAR_SHUI] == HANZI_MASTERY_SEEN);

    /* Learn Card -> Direct Choice (水 vs 山) */
    hanzi_story_on_button(&story, HANZI_BTN_OK);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_DIRECT_CHOICE);
    assert(strcmp(view.choice_top, "水") == 0);
    assert(strcmp(view.choice_bottom, "山") == 0);

    /* Direct Choice: press UP to choose Top (水) -> Magic Effect (River flows) */
    hanzi_story_on_button(&story, HANZI_BTN_UP);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_MAGIC_EFFECT);
    assert(view.world_has_river == 1);
    assert(story.save.world_has_river == 1);
    assert(story.save.char_mastery[HANZI_CHAR_SHUI] == HANZI_MASTERY_UNPROMPTED);

    /* Audio finishes magic effect -> Day Complete */
    hanzi_story_on_audio_done(&story, 1);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_DAY_COMPLETE);
    assert(story.save.day_completed[1] == 1);
    assert(story.save.current_day == 3);

    assert(hanzi_story_consume_save(&story, &save));
    assert(save.world_has_river == 1);
    assert(save.day_completed[1] == 1);
    assert(save.current_day == 3);
}

static void test_day3_flow(void)
{
    hanzi_story_t story;
    hanzi_view_t view;
    hanzi_save_t save;

    /* Start with Day 3 save */
    hanzi_save_defaults(&save);
    save.day_completed[0] = 1;
    save.day_completed[1] = 1;
    save.world_has_river = 1;
    save.current_day = 3;
    hanzi_save_finalize(&save);

    hanzi_story_init(&story, &save);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_INTRO);
    assert(view.current_day == 3);
    assert(view.world_has_river == 1);

    /* Intro -> Detached Test */
    hanzi_story_on_button(&story, HANZI_BTN_OK);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_DETACHED_TEST);
    assert(view.detached_mode == 1);
    assert(strcmp(view.choice_top, "水") == 0);
    assert(strcmp(view.choice_bottom, "爸") == 0);

    /* Detached Test: press UP to choose Top (水) -> Detached Mastery Achieved */
    hanzi_story_on_button(&story, HANZI_BTN_UP);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_FEEDBACK);
    assert(story.save.char_mastery[HANZI_CHAR_SHUI] == HANZI_MASTERY_DETACHED);

    /* Audio finishes feedback -> Sandbox Mode */
    hanzi_story_on_audio_done(&story, 1);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_SANDBOX);
    assert(view.sandbox_active == 1);
    assert(story.save.sandbox_unlocked == 1);

    /* Sandbox play: press buttons */
    hanzi_story_on_button(&story, HANZI_BTN_UP);
    hanzi_story_get_view(&story, &view);
    assert(view.character_state == 1);
    assert(view.world_mountain_size == 1);

    hanzi_story_on_button(&story, HANZI_BTN_DOWN);
    hanzi_story_get_view(&story, &view);
    assert(view.character_state == 2);
    assert(view.world_mountain_size == 2);

    hanzi_story_on_button(&story, HANZI_BTN_OK);
    hanzi_story_get_view(&story, &view);
    assert(view.character_state == 3);

    /* Sandbox timer expires (60s) -> Day Complete */
    hanzi_story_on_tick(&story, 60000U);
    hanzi_story_get_view(&story, &view);
    assert(view.state == HANZI_STATE_DAY_COMPLETE);
    assert(story.save.day_completed[2] == 1);

    assert(hanzi_story_consume_save(&story, &save));
    assert(save.day_completed[2] == 1);
    assert(save.sandbox_unlocked == 1);
}

static void test_save_parse_and_crc(void)
{
    hanzi_save_t save;
    hanzi_save_t parsed;

    hanzi_save_defaults(&save);
    save.day_completed[0] = 1;
    save.world_has_river = 1;
    save.char_mastery[HANZI_CHAR_SHUI] = HANZI_MASTERY_DETACHED;
    hanzi_save_finalize(&save);

    assert(hanzi_save_parse(&save, sizeof(save), &parsed));
    assert(parsed.magic == HANZI_SAVE_MAGIC);
    assert(parsed.day_completed[0] == 1);
    assert(parsed.world_has_river == 1);
    assert(parsed.char_mastery[HANZI_CHAR_SHUI] == HANZI_MASTERY_DETACHED);

    /* Corrupt CRC should fail */
    parsed.crc32 ^= 0x12345678U;
    assert(!hanzi_save_parse(&parsed, sizeof(parsed), &save));
}

int main(void)
{
    test_day1_flow();
    test_day2_flow();
    test_day3_flow();
    test_save_parse_and_crc();
    return 0;
}
