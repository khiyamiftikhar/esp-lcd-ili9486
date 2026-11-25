

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "panel_init.h"   // your existing display init header
#include "unity.h"

//static const char *TAG = "panel_test";

#define LCD_W  320
#define LCD_H  480

// ── RGB565 helpers ────────────────────────────────────────────────────────────
// The panel expects each pixel high byte first; ESP32 stores uint16_t low byte
// first, so swap (same as basic_init, and LVGL's swap_bytes = true).
#define RGB565(r,g,b)  __builtin_bswap16((uint16_t)((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3)))
#define WHITE   0xFFFF
#define BLACK   0x0000
#define RED     RGB565(255,0,0)
#define GREEN   RGB565(0,255,0)
#define BLUE    RGB565(0,0,255)
#define YELLOW  RGB565(255,255,0)
#define CYAN    RGB565(0,255,255)
#define MAGENTA RGB565(255,0,255)


static const char* TAG = "test_panel";
// ── Pixel buffer — one row at a time to avoid large stack allocs ──────────────
static uint16_t row_buf[LCD_W];

// ── Fill a rectangle with a solid colour ─────────────────────────────────────
static void fill_rect(esp_lcd_panel_handle_t panel,
                      int x0, int y0, int x1, int y1,
                      uint16_t colour)
{
    int w = x1 - x0 + 1;
    for (int i = 0; i < w; i++) row_buf[i] = colour;

    for (int y = y0; y <= y1; y++) {
        esp_lcd_panel_draw_bitmap(panel, x0, y, x1 + 1, y + 1, row_buf);
    }

    /* The last row is still being sent by DMA when draw_bitmap returns.
     * Wait before row_buf is refilled by the next call (same as basic_init). */
    vTaskDelay(pdMS_TO_TICKS(10));
}

// ── Fill entire screen ────────────────────────────────────────────────────────
static void fill_screen(esp_lcd_panel_handle_t panel, uint16_t colour)
{
    fill_rect(panel, 0, 0, LCD_W - 1, LCD_H - 1, colour);
}

// ── Single pixel ──────────────────────────────────────────────────────────────
static void draw_pixel(esp_lcd_panel_handle_t panel, int x, int y, uint16_t colour)
{
    /* static: DMA reads the pixel after this function returns */
    static uint16_t px;
    px = colour;
    esp_lcd_panel_draw_bitmap(panel, x, y, x + 1, y + 1, &px);
    vTaskDelay(pdMS_TO_TICKS(10));
}

// ─────────────────────────────────────────────────────────────────────────────
// TESTS
// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
// TEST CASES
// ─────────────────────────────────────────────────────────────────────────────

/**
 * TEST 1 — Single pixel
 *
 * Draws one red pixel at (0,0) on a black screen.
 *
 * Pass: one red dot visible at top-left, rest black
 * Fail: nothing visible → draw_bitmap not reaching display
 */


 void setUp(){

    
    esp_lcd_panel_handle_t panel = ili9486_display_get_panel();
    if(panel==NULL) {
        esp_err_t ret=ili9486_display_init();
        TEST_ASSERT_EQUAL(ESP_OK, ret);
        panel = ili9486_display_get_panel();
    }
    fill_screen(panel, BLACK);
    vTaskDelay(pdMS_TO_TICKS(500));

 }

 TEST_CASE("single pixel at origin", "[ili9486]")
{
    esp_lcd_panel_handle_t panel = ili9486_display_get_panel();
    TEST_ASSERT_NOT_NULL(panel);

    fill_screen(panel, BLACK);
    vTaskDelay(pdMS_TO_TICKS(500));

    draw_pixel(panel, 0, 0, RED);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "VISUAL CHECK: ONE red dot top-left on black");
    /* Visual test — no automated assertion possible on hardware */
    TEST_PASS();
}

/**
 * TEST 2 — Full screen solid colours
 *
 * Fills entire screen with WHITE, RED, GREEN, BLUE in sequence.
 *
 * Pass: full screen changes colour each time
 * Fail (strip only):      RASET addressing wrong
 * Fail (dim grey):        pixel byte order wrong
 * Fail (wrong hue):       R/B swapped — check the BGR bit in the driver's MADCTL
 */
TEST_CASE("full screen solid colours", "[ili9486]")
{
    esp_lcd_panel_handle_t panel = ili9486_display_get_panel();
    TEST_ASSERT_NOT_NULL(panel);

    ESP_LOGI(TAG, "WHITE");
    fill_screen(panel, WHITE);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "RED");
    fill_screen(panel, RED);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "GREEN");
    fill_screen(panel, GREEN);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "BLUE");
    fill_screen(panel, BLUE);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "VISUAL CHECK: Full screen changed colour 4 times");
    ESP_LOGI(TAG, "  RED shows as BLUE -> BGR bit in the driver's MADCTL");
    ESP_LOGI(TAG, "  Only top strip    -> RASET addressing wrong");
    ESP_LOGI(TAG, "  Dim grey          -> pixel byte order wrong");
    TEST_PASS();
}

/**
 * TEST 3 — Horizontal colour bars
 *
 * Draws 6 horizontal bands of 80px each covering full screen height.
 *
 * Pass: 6 equal bands R/G/B/Y/C/M top to bottom
 * Fail (wrong height):    RASET Y addressing wrong
 * Fail (overlap/missing): window end coordinate off by one
 */
TEST_CASE("horizontal colour bars", "[ili9486]")
{
    esp_lcd_panel_handle_t panel = ili9486_display_get_panel();
    TEST_ASSERT_NOT_NULL(panel);

    fill_rect(panel, 0,   0,   LCD_W-1,  79, RED);
    fill_rect(panel, 0,  80,   LCD_W-1, 159, GREEN);
    fill_rect(panel, 0, 160,   LCD_W-1, 239, BLUE);
    fill_rect(panel, 0, 240,   LCD_W-1, 319, YELLOW);
    fill_rect(panel, 0, 320,   LCD_W-1, 399, CYAN);
    fill_rect(panel, 0, 400,   LCD_W-1, 479, MAGENTA);
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "VISUAL CHECK: 6 equal horizontal bands R/G/B/Y/C/M");
    ESP_LOGI(TAG, "  Wrong height   -> RASET addressing issue");
    ESP_LOGI(TAG, "  Gap between    -> off-by-one in y_end");
    TEST_PASS();
}

/**
 * TEST 4 — Vertical colour bars
 *
 * Draws 4 vertical bands of 80px each covering full screen width.
 *
 * Pass: 4 equal bands R/G/B/W left to right
 * Fail (wrong width):  CASET X addressing wrong
 * Fail (overlap):      off-by-one in x_end
 */
TEST_CASE("vertical colour bars", "[ili9486]")
{
    esp_lcd_panel_handle_t panel = ili9486_display_get_panel();
    TEST_ASSERT_NOT_NULL(panel);

    fill_rect(panel,   0, 0,  79, LCD_H-1, RED);
    fill_rect(panel,  80, 0, 159, LCD_H-1, GREEN);
    fill_rect(panel, 160, 0, 239, LCD_H-1, BLUE);
    fill_rect(panel, 240, 0, 319, LCD_H-1, WHITE);
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "VISUAL CHECK: 4 equal vertical bands R/G/B/W");
    ESP_LOGI(TAG, "  Wrong width -> CASET addressing issue");
    TEST_PASS();
}

/**
 * TEST 5 — Corner markers
 *
 * Draws 30x30 coloured squares in each corner on a black background.
 *   Top-left     = RED
 *   Top-right    = GREEN
 *   Bottom-left  = BLUE
 *   Bottom-right = WHITE
 *
 * Pass: correct colour in correct corner, with no mirror call (driver 2.0.0+)
 * Fail (H mirror):  init settings not reaching the panel (0xB6 SS bit), or
 *                   the app still calls esp_lcd_panel_mirror(panel, true, false)
 * Fail (V mirror):  esp_lcd_panel_mirror(panel, false, true)
 * Fail (rotated):   esp_lcd_panel_swap_xy() and swap H/V res
 */
TEST_CASE("corner orientation markers", "[ili9486]")
{
    esp_lcd_panel_handle_t panel = ili9486_display_get_panel();
    TEST_ASSERT_NOT_NULL(panel);

    fill_screen(panel, BLACK);
    vTaskDelay(pdMS_TO_TICKS(300));

    fill_rect(panel,         0,         0,  29,  29, RED);
    fill_rect(panel, LCD_W-30,         0, LCD_W-1,  29, GREEN);
    fill_rect(panel,         0, LCD_H-30,  29, LCD_H-1, BLUE);
    fill_rect(panel, LCD_W-30, LCD_H-30, LCD_W-1, LCD_H-1, WHITE);
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "VISUAL CHECK: TL=RED  TR=GREEN  BL=BLUE  BR=WHITE");
    ESP_LOGI(TAG, "  H mirrored -> init not applied, or app still mirrors X");
    ESP_LOGI(TAG, "  V mirrored -> esp_lcd_panel_mirror(panel, false, true)");
    ESP_LOGI(TAG, "  Rotated 90 -> esp_lcd_panel_swap_xy() and swap H/V res");
    TEST_PASS();
}

/**
 * TEST 6 — Full screen gradient
 *
 * Draws a red-to-blue gradient row by row across full screen.
 *
 * Pass: smooth gradient, no banding
 * Fail (banding):       RASET byte order or partial flush issue
 * Fail (washed out):    init settings (gamma/VCOM) not reaching the panel
 * Fail (corruption):    DMA or buffer size issue
 */
TEST_CASE("full screen gradient", "[ili9486]")
{
    esp_lcd_panel_handle_t panel = ili9486_display_get_panel();
    TEST_ASSERT_NOT_NULL(panel);

    /* Two row buffers, used alternately: drawing row y waits for row y-1 to
     * finish, so the buffer of row y-2 is free to refill. */
    static uint16_t grad_buf[2][LCD_W];
    for (int y = 0; y < LCD_H; y++) {
        uint16_t *buf   = grad_buf[y & 1];
        uint8_t val     = (y * 255) / (LCD_H - 1);
        uint16_t colour = RGB565(val, 0, 255 - val);
        for (int x = 0; x < LCD_W; x++) buf[x] = colour;
        esp_lcd_panel_draw_bitmap(panel, 0, y, LCD_W, y + 1, buf);
    }
    vTaskDelay(pdMS_TO_TICKS(10));   /* last row's DMA */
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "VISUAL CHECK: Smooth red->blue gradient top to bottom");
    ESP_LOGI(TAG, "  Banding      -> RASET byte ordering issue");
    ESP_LOGI(TAG, "  Washed out   -> gamma/VCOM init not reaching the panel");
    TEST_PASS();
}