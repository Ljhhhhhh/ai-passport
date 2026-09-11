#include "cards_model.h"

#include <assert.h>
#include <stdint.h>

static void test_boot_first_card(void)
{
    cards_model_t model;

    cards_model_init(&model);
    assert(model.index == 0);
    assert(model.awake == 1);
    assert(model.idle_ms == 0);
    assert(model.play_gen == 1);
}

static void test_wrap_and_replay(void)
{
    cards_model_t model;
    cards_result_t res;

    cards_model_init(&model);
    res = cards_model_on_button(&model, CARDS_BTN_UP);
    assert(res.act == CARDS_ACT_SHOW_PLAY);
    assert(res.index == CARDS_COUNT - 1U);

    res = cards_model_on_button(&model, CARDS_BTN_DOWN);
    assert(res.index == 0);

    for (uint8_t i = 0; i < CARDS_COUNT; i++) {
        res = cards_model_on_button(&model, CARDS_BTN_DOWN);
    }
    assert(res.index == 0);

    uint8_t before = model.index;
    uint32_t gen = model.play_gen;
    res = cards_model_on_button(&model, CARDS_BTN_OK);
    assert(res.index == before);
    assert(res.play_gen == gen + 1U);
}

static void test_rapid_keys_keep_last_index(void)
{
    cards_model_t model;
    cards_result_t res;

    cards_model_init(&model);
    cards_model_on_button(&model, CARDS_BTN_DOWN);
    cards_model_on_button(&model, CARDS_BTN_DOWN);
    cards_model_on_button(&model, CARDS_BTN_DOWN);
    cards_model_on_button(&model, CARDS_BTN_UP);
    res = cards_model_on_button(&model, CARDS_BTN_DOWN);
    assert(res.index == 3);
    assert(cards_model_play_current(&model, res.play_gen));
    assert(!cards_model_play_current(&model, res.play_gen - 1U));
}

static void test_idle_sleep_once(void)
{
    cards_model_t model;
    cards_result_t res;

    cards_model_init(&model);
    res = cards_model_on_tick(&model, 59999U);
    assert(res.act == CARDS_ACT_NONE);
    assert(model.awake == 1);

    res = cards_model_on_tick(&model, 1U);
    assert(res.act == CARDS_ACT_SLEEP);
    assert(model.awake == 0);
    assert(model.idle_ms == CARDS_IDLE_MS);

    res = cards_model_on_tick(&model, 1000U);
    assert(res.act == CARDS_ACT_NONE);
    assert(model.awake == 0);
}

static void test_wake_keeps_index(void)
{
    cards_model_t model;
    cards_result_t res;
    cards_btn_t buttons[3] = {CARDS_BTN_UP, CARDS_BTN_DOWN, CARDS_BTN_OK};

    for (int i = 0; i < 3; i++) {
        cards_model_init(&model);
        cards_model_on_button(&model, CARDS_BTN_DOWN);
        cards_model_on_button(&model, CARDS_BTN_DOWN);
        cards_model_on_tick(&model, CARDS_IDLE_MS);
        assert(model.awake == 0);
        assert(model.index == 2);

        res = cards_model_on_button(&model, buttons[i]);
        assert(res.act == CARDS_ACT_SHOW_PLAY);
        assert(res.index == 2);
        assert(model.awake == 1);

        res = cards_model_on_button(&model, buttons[i]);
        if (buttons[i] == CARDS_BTN_UP) {
            assert(res.index == 1);
        } else if (buttons[i] == CARDS_BTN_DOWN) {
            assert(res.index == 3);
        } else {
            assert(res.index == 2);
        }
    }
}

int main(void)
{
    test_boot_first_card();
    test_wrap_and_replay();
    test_rapid_keys_keep_last_index();
    test_idle_sleep_once();
    test_wake_keeps_index();
    return 0;
}
