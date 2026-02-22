

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


void app_main(void)
{
    ESP_LOGI(TAG, "ILI9486 lvgl demo example");
    esp_err_t ret = ili9486_display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel init failed: %s", esp_err_to_name(ret));
        return;
    }

    if (lvgl_port_lock(0)) {

        // TEST 1 — Label: verifies text rendering and font
        lv_obj_t *scr = lv_scr_act();
        lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

        lv_obj_t *label = lv_label_create(scr);
        lv_label_set_text(label, "ILI9486 + LVGL");
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_center(label);

        lvgl_port_unlock();
    }
    vTaskDelay(pdMS_TO_TICKS(2000));

    if (lvgl_port_lock(0)) {

        // TEST 2 — Multiple labels at different positions
        // verifies CASET/RASET window addressing via LVGL dirty regions
        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr);
        lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

        lv_obj_t *tl = lv_label_create(scr);
        lv_label_set_text(tl, "TOP LEFT");
        lv_obj_set_style_text_color(tl, lv_color_make(255,0,0), 0);
        lv_obj_set_pos(tl, 10, 10);

        lv_obj_t *tr = lv_label_create(scr);
        lv_label_set_text(tr, "TOP RIGHT");
        lv_obj_set_style_text_color(tr, lv_color_make(0,255,0), 0);
        lv_obj_set_pos(tr, 220, 10);

        lv_obj_t *bl = lv_label_create(scr);
        lv_label_set_text(bl, "BOT LEFT");
        lv_obj_set_style_text_color(bl, lv_color_make(0,0,255), 0);
        lv_obj_set_pos(bl, 10, 450);

        lv_obj_t *br = lv_label_create(scr);
        lv_label_set_text(br, "BOT RIGHT");
        lv_obj_set_style_text_color(br, lv_color_make(255,255,0), 0);
        lv_obj_set_pos(br, 230, 450);

        lvgl_port_unlock();
    }
    vTaskDelay(pdMS_TO_TICKS(2000));

    if (lvgl_port_lock(0)) {

        // TEST 3 — Button widget: verifies LVGL widget rendering
        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr);
        lv_obj_set_style_bg_color(scr, lv_color_make(30,30,30), 0);

        lv_obj_t *btn = lv_btn_create(scr);
        lv_obj_set_size(btn, 200, 60);
        lv_obj_center(btn);

        lv_obj_t *btn_label = lv_label_create(btn);
        lv_label_set_text(btn_label, "LVGL Button");
        lv_obj_center(btn_label);

        lvgl_port_unlock();
    }
    vTaskDelay(pdMS_TO_TICKS(2000));

    if (lvgl_port_lock(0)) {

        // TEST 4 — Animation: verifies LVGL timer and continuous flushing
        lv_obj_t *scr = lv_scr_act();
        lv_obj_clean(scr);
        lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

        lv_obj_t *bar = lv_bar_create(scr);
        lv_obj_set_size(bar, 280, 30);
        lv_obj_center(bar);
        lv_bar_set_range(bar, 0, 100);
        lv_bar_set_value(bar, 0, LV_ANIM_OFF);

        lvgl_port_unlock();

        // Animate bar from 0 to 100
        for (int v = 0; v <= 100; v += 2) {
            if (lvgl_port_lock(0)) {
                lv_bar_set_value(bar, v, LV_ANIM_ON);
                lvgl_port_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "LVGL demo complete");
    ESP_LOGI(TAG, "  Labels visible and correctly positioned -> LVGL + driver OK");
    ESP_LOGI(TAG, "  Garbled text -> byte swap issue");
    ESP_LOGI(TAG, "  Text only in strip -> RASET issue not fixed");
    ESP_LOGI(TAG, "  Bar did not animate -> LVGL task or lock issue");

    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}