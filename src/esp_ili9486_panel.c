// ─── ili9486_panel.c ────────────────────────────────────────────────────────
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_types.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_interface.h"
#include "esp_ili9486_panel.h"

static const char *TAG = "ili9486";

#define ILI9486_CMD_SWRESET  0x01
#define ILI9486_CMD_SLPOUT   0x11
#define ILI9486_CMD_COLMOD   0x3A
#define ILI9486_CMD_MADCTL   0x36
#define ILI9486_CMD_DISPON   0x29
#define ILI9486_CMD_CASET    0x2A
#define ILI9486_CMD_RASET    0x2B
#define ILI9486_CMD_RAMWR    0x2C
#define ILI9486_CMD_INVON    0x21
#define ILI9486_CMD_INVOFF   0x20

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    int x_gap;
    int y_gap;
    uint8_t madctl;
    bool invert_color;
} ili9486_panel_t;

static esp_err_t panel_ili9486_del(esp_lcd_panel_t *panel);
static esp_err_t panel_ili9486_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_ili9486_init(esp_lcd_panel_t *panel);
static esp_err_t panel_ili9486_draw_bitmap(esp_lcd_panel_t *panel,
                                            int x_start, int y_start,
                                            int x_end,   int y_end,
                                            const void *color_data);
static esp_err_t panel_ili9486_invert_color(esp_lcd_panel_t *panel, bool invert);
static esp_err_t panel_ili9486_mirror(esp_lcd_panel_t *panel, bool mx, bool my);
static esp_err_t panel_ili9486_swap_xy(esp_lcd_panel_t *panel, bool swap);
static esp_err_t panel_ili9486_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_ili9486_disp_on_off(esp_lcd_panel_t *panel, bool on);

static esp_err_t ili9486_send(esp_lcd_panel_io_handle_t io,
                               int cmd, const uint8_t *data, size_t len)
{
    return esp_lcd_panel_io_tx_param(io, cmd, data, len);
}

static esp_err_t ili9486_send_madctl(esp_lcd_panel_io_handle_t io, uint8_t madctl)
{
    esp_err_t ret = esp_lcd_panel_io_tx_param(io, ILI9486_CMD_MADCTL, NULL, 0);
    if (ret != ESP_OK) return ret;
    return esp_lcd_panel_io_tx_color(io, -1, &madctl, 1);
}

static void ili9486_send_init_sequence(esp_lcd_panel_io_handle_t io, uint8_t madctl)
{
    ili9486_send(io, ILI9486_CMD_SWRESET, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(120));

    ili9486_send(io, ILI9486_CMD_SLPOUT, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(20));

    ili9486_send(io, 0xB0, (uint8_t[]){0x00}, 1);
    ili9486_send(io, 0xB1, (uint8_t[]){0xB0, 0x11}, 2);
    ili9486_send(io, 0xB4, (uint8_t[]){0x02}, 1);
    ili9486_send(io, 0xB6, (uint8_t[]){0x02, 0x22}, 2);
    ili9486_send(io, 0xB7, (uint8_t[]){0xC6}, 1);
    ili9486_send(io, 0xC0, (uint8_t[]){0x0D, 0x0D}, 2);
    ili9486_send(io, 0xC1, (uint8_t[]){0x41}, 1);
    ili9486_send(io, 0xC5, (uint8_t[]){0x00, 0x18}, 2);

    ili9486_send(io, 0xE0,
        (uint8_t[]){0x0F,0x1F,0x1C,0x0C,0x0F,0x08,0x48,0x98,
                    0x37,0x0A,0x13,0x04,0x11,0x0D,0x00}, 15);
    ili9486_send(io, 0xE1,
        (uint8_t[]){0x0F,0x32,0x2E,0x0B,0x0D,0x05,0x47,0x75,
                    0x37,0x06,0x10,0x03,0x24,0x20,0x00}, 15);

    /* RGB565 — 16-bit pixels, no conversion buffer needed */
    esp_lcd_panel_io_tx_param(io, ILI9486_CMD_COLMOD, NULL, 0);
    //esp_lcd_panel_io_tx_color(io, -1, (uint8_t[]){0x55}, 1);
    esp_lcd_panel_io_tx_param(io, -1, (uint8_t[]){0x55}, 1);
    //ili9486_send(io, ILI9486_CMD_COLMOD, (uint8_t[]){0x55}, 1);

    ili9486_send_madctl(io, madctl);

    ili9486_send(io, ILI9486_CMD_DISPON, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
}

esp_err_t esp_lcd_new_panel_ili9486(esp_lcd_panel_io_handle_t io,
                                    const esp_lcd_panel_dev_config_t *cfg,
                                    esp_lcd_panel_handle_t *ret_panel)
{
    ESP_RETURN_ON_FALSE(io && cfg && ret_panel, ESP_ERR_INVALID_ARG, TAG, "invalid arg");

    ili9486_panel_t *ili = heap_caps_calloc(1, sizeof(*ili), MALLOC_CAP_DEFAULT);
    ESP_RETURN_ON_FALSE(ili, ESP_ERR_NO_MEM, TAG, "no memory for panel");

    ili->io             = io;
    ili->reset_gpio_num = cfg->reset_gpio_num;
    ili->madctl         = 0x08;
    ili->invert_color   = false;

    if (cfg->reset_gpio_num >= 0) {
        gpio_config_t rst_conf = {
            .mode         = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << cfg->reset_gpio_num,
        };
        gpio_config(&rst_conf);
    }

    ili->base.del          = panel_ili9486_del;
    ili->base.reset        = panel_ili9486_reset;
    ili->base.init         = panel_ili9486_init;
    ili->base.draw_bitmap  = panel_ili9486_draw_bitmap;
    ili->base.invert_color = panel_ili9486_invert_color;
    ili->base.mirror       = panel_ili9486_mirror;
    ili->base.swap_xy      = panel_ili9486_swap_xy;
    ili->base.set_gap      = panel_ili9486_set_gap;
    ili->base.disp_on_off  = panel_ili9486_disp_on_off;

    *ret_panel = &ili->base;
    return ESP_OK;
}

static esp_err_t panel_ili9486_del(esp_lcd_panel_t *panel)
{
    ili9486_panel_t *ili = __containerof(panel, ili9486_panel_t, base);
    free(ili);
    return ESP_OK;
}

static esp_err_t panel_ili9486_reset(esp_lcd_panel_t *panel)
{
    ili9486_panel_t *ili = __containerof(panel, ili9486_panel_t, base);
    if (ili->reset_gpio_num >= 0) {
        gpio_set_level(ili->reset_gpio_num, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(ili->reset_gpio_num, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return ESP_OK;
}

static esp_err_t panel_ili9486_init(esp_lcd_panel_t *panel)
{
    ili9486_panel_t *ili = __containerof(panel, ili9486_panel_t, base);
    ili9486_send_init_sequence(ili->io, ili->madctl);
    return ESP_OK;
}

static esp_err_t panel_ili9486_draw_bitmap(
    esp_lcd_panel_t *panel,
    int x_start, int y_start,
    int x_end,   int y_end,
    const void *color_data)
{
    ili9486_panel_t *ili = __containerof(panel, ili9486_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili->io;

    x_start += ili->x_gap;
    x_end   += ili->x_gap;
    y_start += ili->y_gap;
    y_end   += ili->y_gap;

    // ILI9486 uses 32-bit (4-byte) coordinates, so each value needs 2 bytes
    // sent as 00 HH 00 LL format — total 8 bytes per command
    uint8_t caset[] = {
        0x00, (uint8_t)(x_start >> 8), 0x00, (uint8_t)(x_start & 0xFF),
        0x00, (uint8_t)((x_end - 1) >> 8), 0x00, (uint8_t)((x_end - 1) & 0xFF),
    };
    uint8_t raset[] = {
        0x00, (uint8_t)(y_start >> 8), 0x00, (uint8_t)(y_start & 0xFF),
        0x00, (uint8_t)((y_end - 1) >> 8), 0x00, (uint8_t)((y_end - 1) & 0xFF),
    };

    // CASET (0x2A): combined send doesnt works —so  split command + data in 2 transactions
    // CS stays low for each separately: 0x2A and then 00 HH 00 LL 00 HH 00 LL]
    esp_lcd_panel_io_tx_param(io, ILI9486_CMD_CASET, NULL,0);
    esp_lcd_panel_io_tx_param(io, -1, caset, 8);

    // RASET (0x2B): this LCD requires split send — command and data separately
    // CS pulses low twice: [0x2B] then [00 HH 00 LL 00 HH 00 LL]
    // Both calls are blocking (tx_param) so bus is fully free before RAMWR
    esp_lcd_panel_io_tx_param(io, ILI9486_CMD_RASET, NULL, 0);
    esp_lcd_panel_io_tx_param(io, -1, raset, 8);

    // RAMWR (0x2C): also requires split send on this LCD
    // Command is blocking. Pixel data is async DMA — on_color_trans_done
    // CB fires when complete. Nothing must touch the SPI bus until then.
    ESP_LOGD(TAG, "Drawing bitmap: (%d, %d) - (%d, %d), size: %d bytes",
             x_start, y_start, x_end, y_end, (x_end - x_start) * (y_end - y_start) * 2);
    size_t len = (x_end - x_start) * (y_end - y_start) * 2;
    esp_lcd_panel_io_tx_param(io, ILI9486_CMD_RAMWR, NULL, 0);
    return esp_lcd_panel_io_tx_color(io, -1, color_data, len);
}

static esp_err_t panel_ili9486_invert_color(esp_lcd_panel_t *panel, bool invert)
{
    ili9486_panel_t *ili = __containerof(panel, ili9486_panel_t, base);
    int cmd = invert ? ILI9486_CMD_INVON : ILI9486_CMD_INVOFF;
    return esp_lcd_panel_io_tx_param(ili->io, cmd, NULL, 0);
}

static esp_err_t panel_ili9486_mirror(esp_lcd_panel_t *panel, bool mx, bool my)
{
    ili9486_panel_t *ili = __containerof(panel, ili9486_panel_t, base);
    if (mx) ili->madctl |=  0x40; else ili->madctl &= ~0x40;
    if (my) ili->madctl |=  0x80; else ili->madctl &= ~0x80;
    return ili9486_send_madctl(ili->io, ili->madctl);
}

static esp_err_t panel_ili9486_swap_xy(esp_lcd_panel_t *panel, bool swap)
{
    ili9486_panel_t *ili = __containerof(panel, ili9486_panel_t, base);
    if (swap) ili->madctl |=  0x20; else ili->madctl &= ~0x20;
    return ili9486_send_madctl(ili->io, ili->madctl);
}

static esp_err_t panel_ili9486_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    ili9486_panel_t *ili = __containerof(panel, ili9486_panel_t, base);
    ili->x_gap = x_gap;
    ili->y_gap = y_gap;
    return ESP_OK;
}

static esp_err_t panel_ili9486_disp_on_off(esp_lcd_panel_t *panel, bool on)
{
    ili9486_panel_t *ili = __containerof(panel, ili9486_panel_t, base);
    int cmd = on ? ILI9486_CMD_DISPON : 0x28;
    return esp_lcd_panel_io_tx_param(ili->io, cmd, NULL, 0);
}