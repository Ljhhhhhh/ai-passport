#include "hanzi_store.h"

#include <stdbool.h>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define STORE_NS "hanzi"
#define STORE_KEY "save"

static const char *TAG = "hanzi_store";
static QueueHandle_t s_queue;
static nvs_handle_t s_nvs;
static bool s_ready;

static void save_task(void *arg)
{
    hanzi_save_t blob;

    (void)arg;
    for (;;) {
        if (xQueueReceive(s_queue, &blob, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (!s_ready) {
            continue;
        }
        esp_err_t err = nvs_set_blob(s_nvs, STORE_KEY, &blob, sizeof(blob));
        if (err == ESP_OK) {
            err = nvs_commit(s_nvs);
        }
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "NVS save failed (%s); keeping RAM state",
                     esp_err_to_name(err));
        }
    }
}

int hanzi_store_init(hanzi_save_t *save)
{
    hanzi_save_defaults(save);

    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS init failed (%s); using RAM-only save",
                 esp_err_to_name(err));
        return 0;
    }

    err = nvs_open(STORE_NS, NVS_READWRITE, &s_nvs);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open failed (%s); using RAM-only save",
                 esp_err_to_name(err));
        return 0;
    }

    hanzi_save_t blob;
    size_t len = sizeof(blob);
    err = nvs_get_blob(s_nvs, STORE_KEY, &blob, &len);
    if (err == ESP_OK && hanzi_save_parse(&blob, len, save)) {
        ESP_LOGI(TAG, "save restored (day=%u river=%u sandbox=%u)",
                 (unsigned)save->current_day,
                 (unsigned)save->world_has_river,
                 (unsigned)save->sandbox_unlocked);
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "save ignored (%s); starting fresh in RAM",
                 esp_err_to_name(err));
        hanzi_save_defaults(save);
    }

    s_queue = xQueueCreate(1, sizeof(hanzi_save_t));
    if (!s_queue ||
        xTaskCreate(save_task, "hanzi_nvs", 3072, NULL, 3, NULL) != pdPASS) {
        ESP_LOGW(TAG, "NVS worker unavailable; using RAM-only save");
        return 0;
    }

    s_ready = true;
    return 1;
}

int hanzi_store_save(const hanzi_save_t *save)
{
    hanzi_save_t blob;

    if (!save || !s_ready || !s_queue) {
        return 0;
    }
    blob = *save;
    hanzi_save_finalize(&blob);
    xQueueOverwrite(s_queue, &blob);
    return 1;
}
