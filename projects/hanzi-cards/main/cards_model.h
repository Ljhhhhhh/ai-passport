#pragma once

#include <stdint.h>

#define CARDS_COUNT 50U
#define CARDS_IDLE_MS 60000U

typedef enum {
    CARDS_BTN_UP = 0,
    CARDS_BTN_DOWN,
    CARDS_BTN_OK
} cards_btn_t;

typedef enum {
    CARDS_ACT_NONE = 0,
    CARDS_ACT_SHOW_PLAY,
    CARDS_ACT_SLEEP
} cards_act_t;

typedef struct {
    uint8_t index;
    uint8_t awake;
    uint32_t idle_ms;
    uint32_t play_gen;
} cards_model_t;

typedef struct {
    cards_act_t act;
    uint8_t index;
    uint32_t play_gen;
} cards_result_t;

void cards_model_init(cards_model_t *model);
cards_result_t cards_model_on_button(cards_model_t *model, cards_btn_t button);
cards_result_t cards_model_on_tick(cards_model_t *model, uint32_t dt_ms);
int cards_model_play_current(const cards_model_t *model, uint32_t play_gen);
