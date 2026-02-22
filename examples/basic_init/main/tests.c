
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_ili9486_panel.h"
#include "tests.h"  


#define LCD_W  320
#define LCD_H  480

// ── RGB565 helpers ────────────────────────────────────────────────────────────
// NOTE: ILI9486 is BGR so colours may appear swapped — that's useful info!
#define RGB565(r,g,b)  ((uint16_t)((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3)))
#define WHITE   0xFFFF
#define BLACK   0x0000
#define RED     RGB565(255,0,0)
#define GREEN   RGB565(0,255,0)
#define BLUE    RGB565(0,0,255)
#define YELLOW  RGB565(255,255,0)
#define CYAN    RGB565(0,255,255)
#define MAGENTA RGB565(255,0,255)

static const char *TAG = "TESTS";
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
}

// ── Fill entire screen ────────────────────────────────────────────────────────
static void fill_screen(esp_lcd_panel_handle_t panel, uint16_t colour)
{
    fill_rect(panel, 0, 0, LCD_W - 1, LCD_H - 1, colour);
}

// ── Single pixel ──────────────────────────────────────────────────────────────
static void draw_pixel(esp_lcd_panel_handle_t panel, int x, int y, uint16_t colour)
{
    esp_lcd_panel_draw_bitmap(panel, x, y, x + 1, y + 1, &colour);
}


// TEST 1: Single pixel in top-left corner
// Pass: one coloured dot at 0,0
// Fail (nothing): draw_bitmap never reaches display
void test1_single_pixel(esp_lcd_panel_handle_t panel)
{
    ESP_LOGI(TAG, "TEST 1: Single RED pixel at (0,0)");
    fill_screen(panel, BLACK);
    vTaskDelay(pdMS_TO_TICKS(500));
    draw_pixel(panel, 0, 0, RED);
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "TEST 1: Expect ONE red dot top-left on black");
}

// TEST 2: Full screen solid colours
// Pass: entire screen changes colour
// Fail (strip only): RASET addressing wrong
void test2_solid_colours(esp_lcd_panel_handle_t panel)
{
    ESP_LOGI(TAG, "TEST 2: Solid WHITE");
    fill_screen(panel, WHITE);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "TEST 2: Solid RED");
    fill_screen(panel, RED);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "TEST 2: Solid GREEN");
    fill_screen(panel, GREEN);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "TEST 2: Solid BLUE");
    fill_screen(panel, BLUE);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "TEST 2 notes:");
    ESP_LOGI(TAG, "  All correct         -> colour format OK");
    ESP_LOGI(TAG, "  RED shows as BLUE   -> BGR/RGB swapped, toggle MADCTL bit 3");
    ESP_LOGI(TAG, "  Dim grey only       -> pixel data byte order wrong (swap_bytes)");
    ESP_LOGI(TAG, "  Only top strip      -> RASET window addressing wrong");
}

// TEST 3: Horizontal colour bars — tests Y window addressing
// Pass: 6 equal horizontal bands each 80px tall
// Fail: bands wrong height or overlapping = RASET problem
void test3_h_bars(esp_lcd_panel_handle_t panel)
{
    ESP_LOGI(TAG, "TEST 3: Horizontal colour bars (tests RASET)");
    fill_rect(panel, 0,   0,   319,  79, RED);
    fill_rect(panel, 0,  80,   319, 159, GREEN);
    fill_rect(panel, 0, 160,   319, 239, BLUE);
    fill_rect(panel, 0, 240,   319, 319, YELLOW);
    fill_rect(panel, 0, 320,   319, 399, CYAN);
    fill_rect(panel, 0, 400,   319, 479, MAGENTA);
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "TEST 3: Expect 6 equal horizontal bands R/G/B/Y/C/M top to bottom");
}

// TEST 4: Vertical colour bars — tests X window addressing
// Pass: 4 equal vertical bands each 80px wide
// Fail: bands wrong width = CASET problem
void test4_v_bars(esp_lcd_panel_handle_t panel)
{
    ESP_LOGI(TAG, "TEST 4: Vertical colour bars (tests CASET)");
    fill_rect(panel,   0, 0,  79, 479, RED);
    fill_rect(panel,  80, 0, 159, 479, GREEN);
    fill_rect(panel, 160, 0, 239, 479, BLUE);
    fill_rect(panel, 240, 0, 319, 479, WHITE);
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "TEST 4: Expect 4 equal vertical bands R/G/B/W left to right");
}

// TEST 5: Corner markers — tests orientation / MADCTL
// Pass: correct colours in correct corners
// Fail: corners swapped = mirror/swap_xy wrong
void test5_corners(esp_lcd_panel_handle_t panel)
{
    ESP_LOGI(TAG, "TEST 5: Corner markers (tests MADCTL orientation)");
    fill_screen(panel, BLACK);
    vTaskDelay(pdMS_TO_TICKS(300));

    // 30x30 squares in each corner
    fill_rect(panel,   0,   0,  29,  29, RED);     // top-left     = RED
    fill_rect(panel, 290,   0, 319,  29, GREEN);   // top-right    = GREEN
    fill_rect(panel,   0, 450,  29, 479, BLUE);    // bottom-left  = BLUE
    fill_rect(panel, 290, 450, 319, 479, WHITE);   // bottom-right = WHITE

    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "TEST 5: Expect TL=RED  TR=GREEN  BL=BLUE  BR=WHITE");
    ESP_LOGI(TAG, "        If mirrored horizontally: swap MADCTL bit 6 (MX)");
    ESP_LOGI(TAG, "        If mirrored vertically:   swap MADCTL bit 7 (MY)");
    ESP_LOGI(TAG, "        If rotated 90:            swap MADCTL bit 5 (MV)");
}

// TEST 6: Gradient — tests pixel-level accuracy across full screen
// Pass: smooth gradient across full screen
// Fail: banding or corruption = partial flush or pixel format issue
void test6_gradient(esp_lcd_panel_handle_t panel)
{
    ESP_LOGI(TAG, "TEST 6: Full screen gradient (tests pixel accuracy)");
    for (int y = 0; y < LCD_H; y++) {
        uint8_t val = (y * 255) / (LCD_H - 1);
        uint16_t colour = RGB565(val, 0, 255 - val);  // red->blue gradient
        for (int x = 0; x < LCD_W; x++) row_buf[x] = colour;
        esp_lcd_panel_draw_bitmap(panel, 0, y, LCD_W, y + 1, row_buf);
    }
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "TEST 6: Expect smooth red->blue gradient top to bottom");
    ESP_LOGI(TAG, "        Banding = RASET byte ordering issue");
    ESP_LOGI(TAG, "        Colour wrong hue = BGR/RGB issue");
}

// ─────────────────────────────────