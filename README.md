# esp-lcd-ili9486

An `esp_lcd`-compatible panel driver for the **ILI9486 320×480 TFT display** over SPI, targeting **ESP-IDF v5.x**.

This driver was written because no working IDF v5 compatible driver existed for this display. The ILI9486 has several hardware quirks over SPI that are not obvious and not documented in most example code — these are documented here so you do not have to rediscover them.

---

## Features

- Compatible with the `esp_lcd` panel API (`esp_lcd_panel_draw_bitmap`, `esp_lcd_panel_mirror`, etc.)
- Works with `lvgl_port` for LVGL integration
- RGB565 → RGB666 pixel conversion (required by this display over SPI)
- Correct 16-bit padded CASET/RASET window addressing
- Kconfig configurable pins, clock speed, and resolution
- Unity test app for hardware verification
- Basic raw draw example
- LVGL demo example

---

## Hardware Quirks (Read This First)

The ILI9486 over SPI has several non-obvious requirements that will cause a blank or partially working display if not handled correctly. These were discovered through logic analyser captures comparing a working legacy driver against the esp_lcd abstraction.

### 1. 16-bit Command and Parameter Mode

This display module requires **16-bit SPI commands and parameters** — every command byte and every parameter byte must be padded with a leading `0x00`. Configure the panel IO accordingly:

```c
esp_lcd_panel_io_spi_config_t io_config = {
    .lcd_cmd_bits   = 16,
    .lcd_param_bits = 16,
    // ...
};
```

Without this, the display receives garbled commands and silently ignores them. The display may still respond to reset and sleep-out (which are single-byte commands) giving a false impression that SPI is working.

### 2. CASET/RASET Require Manual Padding in tx_color

`esp_lcd_panel_io_tx_color()` sends raw bytes without applying `lcd_param_bits` padding. This means CASET and RASET window coordinates must be manually padded to 16-bit when sent via `tx_color`:

```c
// Each coordinate byte must be padded with 0x00
uint8_t raset[] = {
    0x00, (y_start >> 8) & 0xFF,
    0x00,  y_start       & 0xFF,
    0x00, (y_end   >> 8) & 0xFF,
    0x00,  y_end         & 0xFF,
};
esp_lcd_panel_io_tx_color(io, -1, raset, 8);  // 8 bytes, not 4
```

Without this, every `draw_bitmap` call writes to the same single row — only the top strip of the display updates regardless of the Y coordinates passed.

### 3. RGB666 Pixel Format Required

Despite the display accepting `COLMOD = 0x55` (RGB565) without error, pixel data sent as RGB565 over SPI appears as a dim grey screen. The ILI9486 requires **RGB666 (18-bit, 3 bytes per pixel)** over SPI. The driver converts RGB565 → RGB666 internally in `draw_bitmap`.

### 4. LVGL Orientation vs Panel MADCTL

When using `lvgl_port`, do not call `esp_lcd_panel_mirror()` directly after `esp_lcd_panel_init()`. The `lvgl_port_add_disp()` call internally calls `esp_lcd_panel_mirror()` based on the rotation config, overwriting any value you set. Control orientation exclusively through the `lvgl_port_display_cfg_t` rotation config:

```c
.rotation = {
    .swap_xy  = false,
    .mirror_x = true,   // set here, not via esp_lcd_panel_mirror()
    .mirror_y = false,
},
```

### 5. Async Callback and Raw Usage

The `on_color_trans_done` callback registered in `io_config` makes `tx_color` asynchronous. When using the driver without LVGL (raw panel calls only), remove this callback or `draw_bitmap` will block indefinitely waiting for a signal that never comes:

```c
.on_color_trans_done = NULL,  // for raw usage without LVGL
```

---

## Repository Structure

```
esp-lcd-ili9486/
├── CMakeLists.txt
├── Kconfig.projbuild
├── LICENSE
├── examples
│   ├── basic_init
│   │   ├── CMakeLists.txt
│   │   ├── main
│   │   │   ├── CMakeLists.txt
│   │   │   ├── main.c
│   │   │   ├── panel_init.c
│   │   │   ├── panel_init.h
│   │   │   ├── tests.c
│   │   │   └── tests.h
│   │   └── sdkconfig
│   └── lvgl_demo
│       ├── CMakeLists.txt
│       ├── dependencies.lock
│       ├── main
│       │   ├── CMakeLists.txt
│       │   ├── idf_component.yml
│       │   ├── main.c
│       │   ├── panel_lvgl_init.c
│       │   ├── panel_lvgl_init.h
│       │   ├── tests.c
│       │   └── tests.h
│       └── sdkconfig
├── include
│   └── esp_ili9486_panel.h
├── layout
├── src
│   └── esp_ili9486_panel.c
├── test
│   ├── CMakeLists.txt
│   ├── panel_init.c
│   ├── panel_init.h
│   └── test_esp_ili9486_panel.c
└── test_app
    ├── CMakeLists.txt
    ├── main
    │   ├── CMakeLists.txt
    │   └── test_main.c
    ├── sdkconfig
```

---

## Installation

### Via IDF Component Manager

Add to your project's `idf_component.yml`:

```yaml
dependencies:
  your-namespace/esp-lcd-ili9486: "^1.0.0"
```

### Manually

Clone into your project's `components/` folder:

```bash
git clone https://github.com/your-username/esp-lcd-ili9486 components/esp-lcd-ili9486
```

---

## Usage

### 1. Create Panel IO

```c
esp_lcd_panel_io_handle_t io_handle;

esp_lcd_panel_io_spi_config_t io_config = {
    .dc_gpio_num        = PIN_NUM_DC,
    .cs_gpio_num        = PIN_NUM_CS,
    .pclk_hz            = 5 * 1000 * 1000,   // 5 MHz recommended
    .lcd_cmd_bits       = 16,                 // required for this display
    .lcd_param_bits     = 16,                 // required for this display
    .spi_mode           = 0,
    .trans_queue_depth  = 10,
};

esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST,
                          &io_config, &io_handle);
```

### 2. Create and Init Panel

```c
esp_lcd_panel_handle_t panel;

esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = PIN_NUM_RST,
    .bits_per_pixel = 16,
};

esp_lcd_new_panel_ili9486(io_handle, &panel_config, &panel);
esp_lcd_panel_reset(panel);
esp_lcd_panel_init(panel);
esp_lcd_panel_disp_on_off(panel, true);
```

### 3. Draw (Raw)

```c
uint16_t pixels[320];  // one row of RGB565
memset(pixels, 0xFF, sizeof(pixels));  // white
esp_lcd_panel_draw_bitmap(panel, 0, 0, 320, 1, pixels);
```

### 4. LVGL Integration

```c
const lvgl_port_display_cfg_t disp_cfg = {
    .io_handle     = io_handle,
    .panel_handle  = panel,
    .buffer_size   = 320 * 80,
    .double_buffer = true,
    .hres          = 320,
    .vres          = 480,
    .monochrome    = false,
    .color_format  = LV_COLOR_FORMAT_RGB565,
    .rotation = {
        .swap_xy  = false,
        .mirror_x = true,    // adjust for your module orientation
        .mirror_y = false,
    },
    .flags = {
        .swap_bytes = false,  // driver handles conversion internally
    }
};
lvgl_port_add_disp(&disp_cfg);
```

---

## Configuration (Kconfig)

| Option | Default | Description |
|---|---|---|
| `ILI9486_SPI_HOST` | 1 (SPI2) | SPI peripheral |
| `ILI9486_PIN_MOSI` | 23 | MOSI GPIO |
| `ILI9486_PIN_MISO` | -1 | MISO GPIO (-1 = unused) |
| `ILI9486_PIN_CLK` | 18 | CLK GPIO |
| `ILI9486_PIN_CS` | 5 | CS GPIO |
| `ILI9486_PIN_DC` | 21 | DC GPIO |
| `ILI9486_PIN_RST` | 22 | RST GPIO |
| `ILI9486_PIN_BL` | 4 | Backlight GPIO |
| `ILI9486_BL_ACTIVE_HIGH` | y | Backlight polarity |
| `ILI9486_PIXEL_CLK_HZ` | 5000000 | SPI clock (5 MHz safe) |
| `ILI9486_H_RES` | 320 | Horizontal resolution |
| `ILI9486_V_RES` | 480 | Vertical resolution |

---

## Running Tests

```bash
cd test_app
idf.py build flash monitor
```

The Unity test menu appears on the serial monitor. Tests cover:
- Single pixel draw
- Full screen solid colours
- Horizontal colour bars (RASET verification)
- Vertical colour bars (CASET verification)
- Corner orientation markers (MADCTL verification)
- Full screen gradient (pixel accuracy)

---

## Tested With

- ESP32 (ESP-IDF v5.x)
- ILI9486 320×480 SPI TFT module (3.5 inch, RPi style connector)
- LVGL v9

---

## Known Issues / TODO

- [ ] `ili9486_send()` sends CMD+DATA as a single MIX transaction due to `lcd_cmd_bits=16` behaviour. Some init commands may be silently ignored by the display as a result. Orientation is currently controlled via `lvgl_port` rotation config as a workaround. A proper fix using separate `tx_param` and `tx_color` transactions is planned.
- [ ] SPI clock above 10 MHz untested
- [ ] ILI9488 (similar panel) not tested but may work with minor changes

---

## License

MIT — see [LICENSE](LICENSE)