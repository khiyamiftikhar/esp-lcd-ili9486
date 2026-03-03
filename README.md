# esp-lcd-ili9486

![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.x-blue)
![Espressif Component Registry](https://img.shields.io/badge/Espressif-Component%20Registry-orange)
![License](https://img.shields.io/badge/license-MIT-green)

An **ESP-IDF v5.x compatible `esp_lcd` panel driver** for the **ILI9486 320×480 SPI TFT display**.

This driver integrates with the official `esp_lcd` abstraction layer and supports both raw panel usage and LVGL via `lvgl_port`.

The ILI9486 has several SPI-specific hardware requirements that are not obvious from the datasheet. These are documented below to prevent common integration issues.

---

# Features

* Compatible with the official `esp_lcd` panel API
* Works with `lvgl_port` (LVGL v9 tested)
* RGB565 → RGB666 pixel conversion (required for SPI mode)
* Correct 16-bit padded CASET/RASET handling
* Kconfig configurable pins, clock, and resolution
* Raw usage example
* LVGL demo example
* Unity hardware verification test suite

---

# Supported Hardware

* ESP32 (tested)
* ESP-IDF v5.x
* ILI9486 320×480 SPI TFT modules (3.5" RPi-style connector type)

Other ESP32 variants should work if SPI is supported.

---

# Installation

## Via ESP-IDF Component Manager (Recommended)

```bash
idf.py add-dependency "your-namespace/esp-lcd-ili9486^1.0.0"
```

Or add to your project’s `idf_component.yml`:

```yaml
dependencies:
  your-namespace/esp-lcd-ili9486: "^1.0.0"
```

After adding the dependency:

```
idf.py menuconfig
Component config → ILI9486 Panel Driver
```

---

## Manual Installation

Clone into your project's `components/` directory:

```bash
git clone https://github.com/your-username/esp-lcd-ili9486 components/esp-lcd-ili9486
```

---

# Basic Usage

## 1. Create SPI Panel IO

```c
esp_lcd_panel_io_handle_t io_handle;

esp_lcd_panel_io_spi_config_t io_config = {
    .dc_gpio_num       = PIN_NUM_DC,
    .cs_gpio_num       = PIN_NUM_CS,
    .pclk_hz           = 5 * 1000 * 1000,   // 5 MHz recommended
    .lcd_cmd_bits      = 16,                // required for ILI9486 SPI
    .lcd_param_bits    = 16,                // required for ILI9486 SPI
    .spi_mode          = 0,
    .trans_queue_depth = 10,
};

ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
    (esp_lcd_spi_bus_handle_t)SPI2_HOST,
    &io_config,
    &io_handle));
```

---

## 2. Create and Initialize the Panel

```c
esp_lcd_panel_handle_t panel;

esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = PIN_NUM_RST,
    .bits_per_pixel = 16,   // Input format (RGB565)
};

ESP_ERROR_CHECK(esp_lcd_new_panel_ili9486(
    io_handle,
    &panel_config,
    &panel));

ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
```

---

## 3. Draw Pixels (Raw Mode)

```c
uint16_t row[320];
memset(row, 0xFF, sizeof(row));  // white row

esp_lcd_panel_draw_bitmap(panel, 0, 0, 320, 1, row);
```

---

# LVGL Integration

Control orientation through LVGL rotation settings rather than calling `esp_lcd_panel_mirror()` manually.

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
        .mirror_x = true,
        .mirror_y = false,
    },
    .flags = {
        .swap_bytes = false,
    }
};

lvgl_port_add_disp(&disp_cfg);
```

---

# Configuration (Kconfig)

Available under:

```
Component config → ILI9486 Panel Driver
```

| Option                 | Default | Description              |
| ---------------------- | ------- | ------------------------ |
| ILI9486_SPI_HOST       | 1       | SPI peripheral           |
| ILI9486_PIN_MOSI       | 23      | MOSI GPIO                |
| ILI9486_PIN_MISO       | -1      | MISO GPIO (-1 if unused) |
| ILI9486_PIN_CLK        | 18      | CLK GPIO                 |
| ILI9486_PIN_CS         | 5       | CS GPIO                  |
| ILI9486_PIN_DC         | 21      | DC GPIO                  |
| ILI9486_PIN_RST        | 22      | Reset GPIO               |
| ILI9486_PIN_BL         | 4       | Backlight GPIO           |
| ILI9486_BL_ACTIVE_HIGH | y       | Backlight polarity       |
| ILI9486_PIXEL_CLK_HZ   | 5000000 | SPI clock                |
| ILI9486_H_RES          | 320     | Horizontal resolution    |
| ILI9486_V_RES          | 480     | Vertical resolution      |

---

# Hardware Notes

## 1. 16-bit Command and Parameter Mode

Commands and parameters must be sent as 16-bit values (8-bit padded with leading 0x00):

```c
.lcd_cmd_bits   = 16,
.lcd_param_bits = 16,
```

Without this, commands may be ignored.

---

## 2. CASET / RASET Manual Padding

When using `esp_lcd_panel_io_tx_color()`, parameters must be manually padded to 16-bit.
Failure to do so results in drawing only the top row repeatedly.

---

## 3. RGB666 Pixel Format Required

Although RGB565 may appear accepted, SPI mode requires RGB666 (18-bit).
This driver automatically converts RGB565 → RGB666 internally.

---

## 4. LVGL Rotation Handling

`lvgl_port_add_disp()` configures panel mirroring internally.
Do not manually call `esp_lcd_panel_mirror()` after initialization.

---

## 5. Async Transfer Callback

If using raw panel mode (without LVGL), ensure:

```c
.on_color_trans_done = NULL
```

Otherwise `draw_bitmap()` may block indefinitely.

---

# Running Tests

```bash
cd test_app
idf.py build flash monitor
```

Unity tests include:

* Single pixel verification
* Full screen colors
* Horizontal and vertical color bars
* Corner orientation markers
* Gradient rendering test

---

# Repository Structure

```
esp-lcd-ili9486/
├── CMakeLists.txt
├── Kconfig
├── idf_component.yml
├── include/
├── src/
├── examples/
├── test/
└── test_app/
```

---

# Known Limitations

* Some init commands are currently sent as mixed transactions due to `lcd_cmd_bits=16` behaviour.
* SPI clock above 10 MHz untested.
* ILI9488 not validated.

---

# Versioning

This component follows semantic versioning:

* Patch → bug fixes
* Minor → backward-compatible improvements
* Major → breaking API changes

---

# License

MIT License — see LICENSE file.
