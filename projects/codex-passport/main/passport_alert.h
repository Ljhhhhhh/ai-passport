#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PASSPORT_ALERT_NONE    = 0,
    PASSPORT_ALERT_WAIT    = 1, // "请确认，等待输入"
    PASSPORT_ALERT_DONE    = 2, // "任务已完成"
    PASSPORT_ALERT_ERROR   = 3, // "任务执行失败"
    PASSPORT_ALERT_NEW_MSG = 4, // "收到新消息"
} passport_alert_type_t;

bool passport_alert_accept(uint32_t *last_sequence, uint32_t sequence);
void passport_alert_play(passport_alert_type_t type);
void passport_alert_play_chime(void);
void passport_alert_set_settings(bool voice_enabled, uint8_t volume);
void passport_alert_get_settings(bool *voice_enabled, uint8_t *volume);
