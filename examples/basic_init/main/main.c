

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "panel_init.h"   // your existing display init header
#include "tests.h"   // your existing display init header


static const char* TAG = "main";
// ── Pixel buffer — one row at a time to avoid large stack allocs ──────────────


void app_main(void)
{
    ESP_LOGI(TAG, "ILI9486 basic init example");
    esp_err_t ret = ili9486_display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel init failed: %s", esp_err_to_name(ret));
        return;
    }

    esp_lcd_panel_handle_t panel = ili9486_display_get_panel();
    if (panel == NULL) {
        ESP_LOGE(TAG, "Failed to get panel handle");
        return;
    }

    test1_single_pixel(panel);
    test2_solid_colours(panel);
    test3_h_bars(panel);
    test4_v_bars(panel);
    test5_corners(panel);
    test6_gradient(panel);

    ESP_LOGI(TAG, "All tests completed");
}