# esp-lcd-ili9486

![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.x-blue)
![Espressif Component Registry](https://img.shields.io/badge/Espressif-Component%20Registry-orange)
![License](https://img.shields.io/badge/license-MIT-green)

ESP-IDF v5.x compatible **`esp_lcd` panel driver** for the **ILI9486 320×480 SPI TFT display**.

This component integrates with Espressif’s `esp_lcd` abstraction layer and supports:

* Raw `esp_lcd` usage
* LVGL integration via `lvgl_port`

The driver handles the SPI-specific requirements of the ILI9486, including 16-bit command padding and RGB565 → RGB666 pixel conversion.

---

# Features

* Compatible with the `esp_lcd` panel API
* RGB565 → RGB666 conversion (required for SPI mode)
* Proper 16-bit padded command and parameter handling
* LVGL v9 compatible
* Configurable via Kconfig
* Includes working raw and LVGL examples
* Includes Unity hardware verification tests

---

# Installation

## Using ESP-IDF Component Manager (Recommended)

```bash
idf.py add-dependency "your-namespace/esp-lcd-ili9486^1.0.0"
```

Or in your project’s `idf_component.yml`:

```yaml
dependencies:
  your-namespace/esp-lcd-ili9486: "^1.0.0"
```

Then configure via:

```
idf.py menuconfig
Component config → ILI9486 Panel Driver
```

---

# Usage

This component implements the standard `esp_lcd` panel interface.

After:

1. Initializing the SPI bus
2. Creating panel IO with:

   ```
   .lcd_cmd_bits   = 16
   .lcd_param_bits = 16
   ```
3. Creating the panel via `esp_lcd_new_panel_ili9486()`

You can use:

* `esp_lcd_panel_draw_bitmap()`
* `esp_lcd_panel_mirror()`
* `esp_lcd_panel_swap_xy()`
* `esp_lcd_panel_disp_on_off()`

For complete working initialization flows, see the examples below.

---

# Examples

Fully working examples are provided:

* `examples/basic_init` – Raw `esp_lcd` usage (no LVGL)
* `examples/lvgl_demo` – LVGL integration using `lvgl_port`

Each example is self-contained and ready to build.

---

# Important Hardware Notes

The ILI9486 SPI interface has several non-obvious requirements:

## 1️⃣ 16-bit Command and Parameter Mode

Commands and parameters must be sent as 16-bit values (8-bit padded with `0x00`).

```c
.lcd_cmd_bits   = 16
.lcd_param_bits = 16
```

Without this, the display may ignore commands silently.

---

## 2️⃣ CASET / RASET Window Padding

When using `esp_lcd_panel_io_tx_color()`, coordinate parameters must be manually padded to 16-bit.
Failure results in drawing repeatedly to the same row.

---

## 3️⃣ RGB666 Required Over SPI

Although `COLMOD = 0x55` (RGB565) may appear accepted, pixel data must be transmitted as RGB666 (18-bit) over SPI.

This driver converts RGB565 → RGB666 internally.

---

## 4️⃣ LVGL Orientation Handling

When using `lvgl_port`, do not manually call `esp_lcd_panel_mirror()` after initialization.

Rotation must be controlled via `lvgl_port_display_cfg_t.rotation`.

---

## 5️⃣ Async Transfer Callback

If `.on_color_trans_done` is enabled without proper synchronization, `draw_bitmap()` may block indefinitely.

For raw usage, keep:

```c
.on_color_trans_done = NULL
```

---

# Configuration (Kconfig)

Available under:

```
Component config → ILI9486 Panel Driver
```

Configurable parameters include:

* SPI host
* GPIO pins
* Pixel clock
* Resolution
* Backlight polarity

---

# Running Tests

```bash
cd test_app
idf.py build flash monitor
```

Unity tests verify:

* Pixel correctness
* Window addressing
* Orientation
* Full-screen rendering

---

# Known Limitations

* SPI clock above 10 MHz not fully validated
* ILI9488 not tested
* Some module variants may require init sequence adjustments

---

# License

MIT License — see LICENSE file.
