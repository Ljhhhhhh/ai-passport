#include "hanzi_story.h"

#include <string.h>

static void set_clip(hanzi_story_t *story, hanzi_clip_id_t clip, hanzi_audio_kind_t kind)
{
    story->clip = clip;
    story->audio_kind = kind;
    story->audio_busy = (clip != HANZI_CLIP_NONE);
}

static void apply_day_setup(hanzi_story_t *story)
{
    story->practice_step = 0;
    story->selected_choice = 0;
    story->fail_count = 0;
    story->character_state = 0;
    story->world_mountain_size = 0;
    story->state_ms = 0;
    story->idle_ms = 0;

    switch (story->current_day) {
    case 1:
        story->target_char_id = HANZI_CHAR_DA;
        story->distractor_char_id = HANZI_CHAR_XIAO;
        story->correct_is_top = 1;
        set_clip(story, HANZI_CLIP_DAY1_INTRO, HANZI_AUDIO_STORY);
        break;
    case 2:
        story->target_char_id = HANZI_CHAR_SHUI;
        story->distractor_char_id = HANZI_CHAR_SHAN;
        story->correct_is_top = 1;
        set_clip(story, HANZI_CLIP_DAY2_INTRO, HANZI_AUDIO_STORY);
        break;
    case 3:
    default:
        story->current_day = 3;
        story->target_char_id = HANZI_CHAR_SHUI;
        story->distractor_char_id = HANZI_CHAR_BA;
        story->correct_is_top = 1;
        set_clip(story, HANZI_CLIP_DAY3_INTRO, HANZI_AUDIO_STORY);
        break;
    }
}

void hanzi_story_init(hanzi_story_t *story, const hanzi_save_t *save)
{
    if (!story) {
        return;
    }
    memset(story, 0, sizeof(*story));
    if (save && save->magic == HANZI_SAVE_MAGIC) {
        story->save = *save;
    } else {
        hanzi_save_defaults(&story->save);
    }

    if (story->save.current_day == 0 || story->save.current_day > HANZI_DAYS_COUNT) {
        story->save.current_day = 1;
    }
    story->current_day = story->save.current_day;
    story->state = HANZI_STATE_INTRO;
    apply_day_setup(story);
}

void hanzi_story_on_button(hanzi_story_t *story, hanzi_btn_t button)
{
    if (!story) {
        return;
    }

    story->idle_ms = 0;

    switch (story->state) {
    case HANZI_STATE_INTRO:
        if (story->current_day == 1) {
            story->state = HANZI_STATE_PRACTICE_JUMP;
            set_clip(story, HANZI_CLIP_DAY1_JUMP_PROMPT, HANZI_AUDIO_PROMPT);
        } else if (story->current_day == 2) {
            story->state = HANZI_STATE_LEARN_CARD;
            story->save.char_seen[HANZI_CHAR_SHUI]++;
            if (story->save.char_mastery[HANZI_CHAR_SHUI] == HANZI_MASTERY_UNKNOWN) {
                story->save.char_mastery[HANZI_CHAR_SHUI] = HANZI_MASTERY_SEEN;
            }
            set_clip(story, HANZI_CLIP_DAY2_LEARN_WATER, HANZI_AUDIO_STORY);
        } else {
            story->state = HANZI_STATE_DETACHED_TEST;
            set_clip(story, HANZI_CLIP_DAY3_TEST_PROMPT, HANZI_AUDIO_PROMPT);
        }
        break;

    case HANZI_STATE_PRACTICE_JUMP:
        if (button == HANZI_BTN_OK) {
            story->character_state = 3; /* Jumping */
            story->save.total_magic_casts++;
            story->practice_step = 1;
            set_clip(story, HANZI_CLIP_MAGIC_JUMP, HANZI_AUDIO_MAGIC);
        } else {
            set_clip(story, HANZI_CLIP_DAY1_JUMP_PROMPT, HANZI_AUDIO_PROMPT);
        }
        break;

    case HANZI_STATE_PRACTICE_SCALE:
        if (button == HANZI_BTN_UP) {
            story->character_state = 1; /* Big */
            story->world_mountain_size = 1;
            story->save.total_magic_casts++;
            story->practice_step++;
            set_clip(story, HANZI_CLIP_MAGIC_BIG, HANZI_AUDIO_MAGIC);
        } else if (button == HANZI_BTN_DOWN) {
            story->character_state = 2; /* Small */
            story->world_mountain_size = 2;
            story->save.total_magic_casts++;
            story->practice_step++;
            set_clip(story, HANZI_CLIP_MAGIC_SMALL, HANZI_AUDIO_MAGIC);
        } else if (button == HANZI_BTN_OK) {
            if (story->practice_step >= 1) {
                story->state = HANZI_STATE_DIRECT_CHOICE;
                story->save.char_seen[story->target_char_id]++;
                set_clip(story, HANZI_CLIP_DAY1_CHOICE_PROMPT, HANZI_AUDIO_PROMPT);
            }
        }
        break;

    case HANZI_STATE_LEARN_CARD:
        story->state = HANZI_STATE_DIRECT_CHOICE;
        set_clip(story, HANZI_CLIP_DAY2_CHOICE_PROMPT, HANZI_AUDIO_PROMPT);
        break;

    case HANZI_STATE_DIRECT_CHOICE:
        if (button == HANZI_BTN_UP || button == HANZI_BTN_DOWN) {
            uint8_t choice = (button == HANZI_BTN_UP) ? 1 : 2;
            uint8_t correct = story->correct_is_top ? 1 : 2;
            story->selected_choice = choice;

            if (choice == correct) {
                story->save.char_correct[story->target_char_id]++;
                if (story->fail_count == 0) {
                    story->save.char_mastery[story->target_char_id] = HANZI_MASTERY_UNPROMPTED;
                } else {
                    story->save.char_mastery[story->target_char_id] = HANZI_MASTERY_PROMPTED;
                }

                if (story->current_day == 1) {
                    story->state = HANZI_STATE_FEEDBACK;
                    set_clip(story, HANZI_CLIP_DAY1_COMPLETE, HANZI_AUDIO_COMPLETE);
                } else {
                    story->state = HANZI_STATE_MAGIC_EFFECT;
                    story->save.world_has_river = 1;
                    set_clip(story, HANZI_CLIP_DAY2_WATER_FLOW, HANZI_AUDIO_MAGIC);
                }
            } else {
                story->fail_count++;
                set_clip(story, HANZI_CLIP_HINT_UP_DOWN, HANZI_AUDIO_HINT);
            }
        } else if (button == HANZI_BTN_OK) {
            set_clip(story, (story->current_day == 1) ? HANZI_CLIP_DAY1_CHOICE_PROMPT : HANZI_CLIP_DAY2_CHOICE_PROMPT,
                     HANZI_AUDIO_PROMPT);
        }
        break;

    case HANZI_STATE_DETACHED_TEST:
        if (button == HANZI_BTN_UP || button == HANZI_BTN_DOWN) {
            uint8_t choice = (button == HANZI_BTN_UP) ? 1 : 2;
            uint8_t correct = story->correct_is_top ? 1 : 2;
            story->selected_choice = choice;

            if (choice == correct) {
                story->save.char_seen[HANZI_CHAR_SHUI]++;
                story->save.char_correct[HANZI_CHAR_SHUI]++;
                story->save.char_mastery[HANZI_CHAR_SHUI] = HANZI_MASTERY_DETACHED;
                story->state = HANZI_STATE_FEEDBACK;
                set_clip(story, HANZI_CLIP_DAY3_TEST_CORRECT, HANZI_AUDIO_COMPLETE);
            } else {
                story->fail_count++;
                set_clip(story, HANZI_CLIP_HINT_UP_DOWN, HANZI_AUDIO_HINT);
            }
        } else if (button == HANZI_BTN_OK) {
            set_clip(story, HANZI_CLIP_DAY3_TEST_PROMPT, HANZI_AUDIO_PROMPT);
        }
        break;

    case HANZI_STATE_FEEDBACK:
        if (story->current_day == 1) {
            story->state = HANZI_STATE_DAY_COMPLETE;
            story->save.day_completed[0] = 1;
            story->save.current_day = 2;
            story->pending_save = 1;
        } else if (story->current_day == 3) {
            story->state = HANZI_STATE_SANDBOX;
            story->sandbox_ms = HANZI_SANDBOX_DURATION_MS;
            story->save.sandbox_unlocked = 1;
            story->save.sandbox_play_count++;
            set_clip(story, HANZI_CLIP_DAY3_SANDBOX_INTRO, HANZI_AUDIO_STORY);
        }
        break;

    case HANZI_STATE_MAGIC_EFFECT:
        story->state = HANZI_STATE_DAY_COMPLETE;
        story->save.day_completed[1] = 1;
        story->save.current_day = 3;
        story->pending_save = 1;
        set_clip(story, HANZI_CLIP_DAY2_COMPLETE, HANZI_AUDIO_COMPLETE);
        break;

    case HANZI_STATE_SANDBOX:
        if (button == HANZI_BTN_UP) {
            story->character_state = 1;
            story->world_mountain_size = 1;
            story->save.total_magic_casts++;
            set_clip(story, HANZI_CLIP_MAGIC_BIG, HANZI_AUDIO_MAGIC);
        } else if (button == HANZI_BTN_DOWN) {
            story->character_state = 2;
            story->world_mountain_size = 2;
            story->save.total_magic_casts++;
            set_clip(story, HANZI_CLIP_MAGIC_SMALL, HANZI_AUDIO_MAGIC);
        } else if (button == HANZI_BTN_OK) {
            story->character_state = 3;
            story->save.total_magic_casts++;
            set_clip(story, HANZI_CLIP_MAGIC_JUMP, HANZI_AUDIO_MAGIC);
        }
        break;

    case HANZI_STATE_DAY_COMPLETE:
        if (button == HANZI_BTN_OK) {
            if (story->save.sandbox_unlocked) {
                story->state = HANZI_STATE_SANDBOX;
                story->sandbox_ms = HANZI_SANDBOX_DURATION_MS;
                story->save.sandbox_play_count++;
                set_clip(story, HANZI_CLIP_DAY3_SANDBOX_INTRO, HANZI_AUDIO_STORY);
            }
        }
        break;
    }
}

void hanzi_story_on_audio_done(hanzi_story_t *story, int success)
{
    if (!story) {
        return;
    }
    story->audio_busy = 0;
    story->audio_failed = success ? 0 : 1;

    if (story->state == HANZI_STATE_FEEDBACK) {
        if (story->current_day == 1) {
            story->state = HANZI_STATE_DAY_COMPLETE;
            story->save.day_completed[0] = 1;
            story->save.current_day = 2;
            story->pending_save = 1;
        } else if (story->current_day == 3) {
            story->state = HANZI_STATE_SANDBOX;
            story->sandbox_ms = HANZI_SANDBOX_DURATION_MS;
            story->save.sandbox_unlocked = 1;
            story->save.sandbox_play_count++;
            set_clip(story, HANZI_CLIP_DAY3_SANDBOX_INTRO, HANZI_AUDIO_STORY);
        }
    } else if (story->state == HANZI_STATE_MAGIC_EFFECT) {
        story->state = HANZI_STATE_DAY_COMPLETE;
        story->save.day_completed[1] = 1;
        story->save.current_day = 3;
        story->pending_save = 1;
        set_clip(story, HANZI_CLIP_DAY2_COMPLETE, HANZI_AUDIO_COMPLETE);
    }
}

void hanzi_story_on_tick(hanzi_story_t *story, uint32_t elapsed_ms)
{
    if (!story) {
        return;
    }

    story->state_ms += elapsed_ms;
    story->idle_ms += elapsed_ms;

    /* Jump recovery */
    if (story->character_state == 3) {
        if (story->state_ms >= 600U) {
            story->character_state = 0;
            if (story->state == HANZI_STATE_PRACTICE_JUMP) {
                story->state = HANZI_STATE_PRACTICE_SCALE;
                set_clip(story, HANZI_CLIP_DAY1_SCALE_PROMPT, HANZI_AUDIO_PROMPT);
            }
        }
    }

    /* Sandbox timer */
    if (story->state == HANZI_STATE_SANDBOX) {
        if (story->sandbox_ms > elapsed_ms) {
            story->sandbox_ms -= elapsed_ms;
        } else {
            story->sandbox_ms = 0;
            story->state = HANZI_STATE_DAY_COMPLETE;
            story->save.day_completed[2] = 1;
            story->pending_save = 1;
            set_clip(story, HANZI_CLIP_DAY3_COMPLETE, HANZI_AUDIO_COMPLETE);
        }
    }
}

void hanzi_story_get_view(const hanzi_story_t *story, hanzi_view_t *view)
{
    if (!story || !view) {
        return;
    }
    memset(view, 0, sizeof(*view));
    view->state = story->state;
    view->current_day = story->current_day;
    view->character_state = story->character_state;
    view->world_has_river = story->save.world_has_river;
    view->world_mountain_size = story->world_mountain_size;
    view->clip = story->clip;
    view->audio_kind = story->audio_kind;

    switch (story->state) {
    case HANZI_STATE_INTRO:
        view->title = (story->current_day == 1) ? "第1天·汉字魔法" :
                      (story->current_day == 2) ? "第2天·遇见新字" : "第3天·认识水吗";
        view->prompt = "按任意键开始";
        view->show_ok_hint = 1;
        break;

    case HANZI_STATE_PRACTICE_JUMP:
        view->title = "汉字魔法·跳";
        view->focus_hanzi = "跳";
        view->prompt = "按确定让小人跳一跳";
        view->show_ok_hint = 1;
        break;

    case HANZI_STATE_PRACTICE_SCALE:
        view->title = "汉字魔法·大与小";
        view->prompt = "按上变大，按下变小";
        view->show_choice_hint = 1;
        view->show_ok_hint = (story->practice_step >= 1);
        break;

    case HANZI_STATE_LEARN_CARD:
        view->title = "新朋友·水";
        view->focus_hanzi = "水";
        view->prompt = "这是【水】，按任意键继续";
        view->show_ok_hint = 1;
        break;

    case HANZI_STATE_DIRECT_CHOICE:
        view->title = (story->current_day == 1) ? "哪个字是【大】？" : "找到【水】让小河奔流";
        view->show_choices = 1;
        view->choice_top = story->correct_is_top ? hanzi_char_name(story->target_char_id)
                                                  : hanzi_char_name(story->distractor_char_id);
        view->choice_bottom = story->correct_is_top ? hanzi_char_name(story->distractor_char_id)
                                                     : hanzi_char_name(story->target_char_id);
        view->selected_choice = story->selected_choice;
        view->correct_choice = story->correct_is_top ? 1 : 2;
        view->show_choice_hint = 1;
        view->show_ok_hint = 1;
        break;

    case HANZI_STATE_DETACHED_TEST:
        view->title = "认一认：哪个是【水】？";
        view->detached_mode = 1;
        view->show_choices = 1;
        view->choice_top = story->correct_is_top ? hanzi_char_name(story->target_char_id)
                                                  : hanzi_char_name(story->distractor_char_id);
        view->choice_bottom = story->correct_is_top ? hanzi_char_name(story->distractor_char_id)
                                                     : hanzi_char_name(story->target_char_id);
        view->selected_choice = story->selected_choice;
        view->correct_choice = story->correct_is_top ? 1 : 2;
        view->show_choice_hint = 1;
        break;

    case HANZI_STATE_FEEDBACK:
        view->title = "太棒啦！";
        view->focus_hanzi = hanzi_char_name(story->target_char_id);
        view->prompt = "选对啦！";
        break;

    case HANZI_STATE_MAGIC_EFFECT:
        view->title = "清清的小河！";
        view->focus_hanzi = "水";
        view->prompt = "小河奔流起来啦！";
        view->world_has_river = 1;
        break;

    case HANZI_STATE_SANDBOX:
        view->title = "自由小世界";
        view->prompt = "上:大 下:小 OK:跳";
        view->sandbox_active = 1;
        view->sandbox_seconds_left = (uint8_t)((story->sandbox_ms + 999U) / 1000U);
        view->show_choice_hint = 1;
        view->show_ok_hint = 1;
        break;

    case HANZI_STATE_DAY_COMPLETE:
        view->title = "今天探索完成！";
        view->prompt = story->save.sandbox_unlocked ? "按确定进入自由小世界" : "眼睛休息啦，明天见！";
        view->show_ok_hint = story->save.sandbox_unlocked;
        break;
    }

    /* Sprite selection */
    if (view->detached_mode) {
        view->sprite = HANZI_SPRITE_CARD_DETACHED;
    } else if (view->state == HANZI_STATE_DAY_COMPLETE && !story->save.sandbox_unlocked) {
        view->sprite = HANZI_SPRITE_BEDTIME;
    } else if (view->world_has_river) {
        if (view->character_state == 3) {
            view->sprite = HANZI_SPRITE_WORLD_RIVER_JUMP;
        } else if (view->character_state == 1) {
            view->sprite = HANZI_SPRITE_WORLD_RIVER_BIG;
        } else if (view->character_state == 2) {
            view->sprite = HANZI_SPRITE_WORLD_RIVER_SMALL;
        } else {
            view->sprite = HANZI_SPRITE_WORLD_RIVER;
        }
    } else {
        if (view->character_state == 3) {
            view->sprite = HANZI_SPRITE_WORLD_JUMP;
        } else if (view->character_state == 1) {
            view->sprite = HANZI_SPRITE_WORLD_BIG;
        } else if (view->character_state == 2) {
            view->sprite = HANZI_SPRITE_WORLD_SMALL;
        } else {
            view->sprite = HANZI_SPRITE_WORLD_BASE;
        }
    }
}

int hanzi_story_consume_save(hanzi_story_t *story, hanzi_save_t *out)
{
    if (!story || !out || !story->pending_save) {
        return 0;
    }
    hanzi_save_finalize(&story->save);
    *out = story->save;
    story->pending_save = 0;
    return 1;
}
