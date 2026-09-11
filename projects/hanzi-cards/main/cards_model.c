#include "cards_model.h"

#include <stddef.h>

static cards_result_t result(cards_act_t act, const cards_model_t *model)
{
    cards_result_t out = {
        .act = act,
        .index = 0,
        .play_gen = 0
    };

    if (model) {
        out.index = model->index;
        out.play_gen = model->play_gen;
    }
    return out;
}

static uint8_t wrap_prev(uint8_t index)
{
    return (uint8_t)((index == 0U) ? (CARDS_COUNT - 1U) : (index - 1U));
}

static uint8_t wrap_next(uint8_t index)
{
    return (uint8_t)((index + 1U) % CARDS_COUNT);
}

void cards_model_init(cards_model_t *model)
{
    if (!model) {
        return;
    }
    model->index = 0;
    model->awake = 1;
    model->idle_ms = 0;
    model->play_gen = 1;
}

cards_result_t cards_model_on_button(cards_model_t *model, cards_btn_t button)
{
    if (!model) {
        return result(CARDS_ACT_NONE, NULL);
    }

    if (!model->awake) {
        model->awake = 1;
        model->idle_ms = 0;
        model->play_gen++;
        return result(CARDS_ACT_SHOW_PLAY, model);
    }

    model->idle_ms = 0;
    if (button == CARDS_BTN_UP) {
        model->index = wrap_prev(model->index);
    } else if (button == CARDS_BTN_DOWN) {
        model->index = wrap_next(model->index);
    }
    model->play_gen++;
    return result(CARDS_ACT_SHOW_PLAY, model);
}

cards_result_t cards_model_on_tick(cards_model_t *model, uint32_t dt_ms)
{
    if (!model || !model->awake || dt_ms == 0U) {
        return result(CARDS_ACT_NONE, model);
    }

    if (model->idle_ms >= CARDS_IDLE_MS) {
        return result(CARDS_ACT_NONE, model);
    }

    if (dt_ms >= (CARDS_IDLE_MS - model->idle_ms)) {
        model->idle_ms = CARDS_IDLE_MS;
        model->awake = 0;
        return result(CARDS_ACT_SLEEP, model);
    }

    model->idle_ms += dt_ms;
    return result(CARDS_ACT_NONE, model);
}

int cards_model_play_current(const cards_model_t *model, uint32_t play_gen)
{
    return model && model->awake && play_gen == model->play_gen;
}
