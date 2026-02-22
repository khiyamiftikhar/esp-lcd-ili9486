#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void test1_single_pixel(esp_lcd_panel_handle_t panel);


// TEST 2: Full screen solid colours
// Pass: entire screen changes colour
// Fail (strip only): RASET addressing wrong
void test2_solid_colours(esp_lcd_panel_handle_t panel);
// TEST 3: Horizontal colour bars — tests Y window addressing
// Pass: 6 equal horizontal bands each 80px tall
// Fail: bands wrong height or overlapping = RASET problem
void test3_h_bars(esp_lcd_panel_handle_t panel);
// TEST 4: Vertical colour bars — tests X window addressing
// Pass: 4 equal vertical bands each 80px wide
// Fail: bands wrong width = CASET problem
void test4_v_bars(esp_lcd_panel_handle_t panel);
// TEST 5: Corner markers — tests orientation / MADCTL
// Pass: correct colours in correct corners
// Fail: corners swapped = mirror/swap_xy wrong
void test5_corners(esp_lcd_panel_handle_t panel);

// TEST 6: Gradient — tests pixel-level accuracy across full screen
// Pass: smooth gradient across full screen
// Fail: banding or corruption = partial flush or pixel format issue
void test6_gradient(esp_lcd_panel_handle_t panel);

// ─────────────────────────────────
#ifdef __cplusplus
}
#endif







