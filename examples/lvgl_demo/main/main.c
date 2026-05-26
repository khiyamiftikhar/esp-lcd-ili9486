

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "panel_lvgl_init.h"   // your existing display init header
#include "tests.h"   // your existing display init header
#include "esp_lvgl_port.h"
#include "lvgl.h"  // LVGL main header


static const char* TAG = "main";
// ── Pixel buffer — one row at a time to avoid large stack allocs ──────────────


static void log_diagnostics(void)
{
    ESP_LOGI(TAG, "──────────────────────────────────────────");
    ESP_LOGI(TAG, "Colour demo complete — diagnostic hints:");
    ESP_LOGI(TAG, "  All primaries correct             -> driver OK");
    ESP_LOGI(TAG, "  Red/Blue swapped on solid screens -> byte-swap issue");
    ESP_LOGI(TAG, "  Stripe colours wrong hue          -> RGB order wrong");
    ESP_LOGI(TAG, "  Bar never changes colour          -> style refresh issue");
    ESP_LOGI(TAG, "  Text only in one strip            -> RASET not fixed");
    ESP_LOGI(TAG, "──────────────────────────────────────────");
}


void app_main(void)
{
    ESP_LOGI(TAG, "ILI9486 LVGL colour demo");

    lv_display_t *handle = NULL;
    esp_err_t ret = ili9486_display_init(&handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel init failed: %s", esp_err_to_name(ret));
        return;
    }

    demo_test_primary_flashes();
    demo_test_rainbow_stripes();
    demo_test_corner_labels();
    demo_test_styled_buttons();
    demo_test_colour_bar();

    log_diagnostics();

    while (1) vTaskDelay(1000 / portTICK_PERIOD_MS);
}