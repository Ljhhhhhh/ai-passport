#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "cards_app.h"
#include "cards_ui.h"

#include "esp_log.h"

static const char *TAG = "cards";

void app_main(void)
{
    ESP_LOGI(TAG, "hanzi cards starting");

    bsp_i2c_init();
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "display init failed (MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(100);

    if (!bsp_lvgl_lock(1000)) {
        ESP_LOGE(TAG, "LVGL lock timeout");
        return;
    }
    cards_ui_create();
    bsp_lvgl_unlock();

    cards_app_start();
}
