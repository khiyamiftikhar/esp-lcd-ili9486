# Changelog

## [2.0.0] - 2026-11-24

Supports ESP-IDF v5.5 and v6.x from the same sources. Hardware-tested on
ESP32-C3 with ESP-IDF v6.0.3; builds on v5.5.2 and v6.1.1.

### Breaking

- **Remove the horizontal-mirror workaround from your app.** Versions 1.x
  needed `esp_lcd_panel_mirror(panel, true, false)` (or LVGL
  `rotation.mirror_x = true`) to show a correct image. The root cause is fixed
  (see below), so keeping that call now mirrors the image.

### Added

- **ESP-IDF v6.x support.**
- Per-target defaults in every example: `sdkconfig.defaults` holds shared
  settings, `sdkconfig.defaults.esp32` and `sdkconfig.defaults.esp32c3` hold
  the pins (ESP32-C3 pin set tested). README instructions for building per
  target. `https` and `ipcam` had no defaults files before.
- `https` and `ipcam` use the 1.5 MB single-app partition table
  (`CONFIG_PARTITION_TABLE_SINGLE_APP_LARGE`). The default 1 MB app partition
  overflows on ESP32-C3 and was 98% full on ESP32.
- `test_app`: same per-target defaults as the examples. The test init now
  matches `basic_init` (8-bit command/parameter bits, queue depth 1, MISO
  from Kconfig); it still used 16-bit bits, which would pad the init
  parameters twice. Outdated hints in the test cases updated.
- `test_app`: colours were byte-swapped (RED showed as blue, GREEN as red,
  BLUE as green). `RGB565()` now swaps bytes like `basic_init`, and the
  helpers wait for the last DMA row before the pixel buffer is reused.

### Fixed

- **Init settings now actually reach the panel.** `ili9486_send()` sent each
  command and its parameters combined, as plain bytes. This panel needs the
  command and parameters in separate transactions, with every parameter sent
  as a 16-bit word (`0x00`, value), the same format CASET/RASET already used.
  Before, the power, VCOM, gamma and display-function settings never arrived
  and each panel ran on its factory defaults. On panels with poor defaults
  this gave washed-out colours: dark and mid levels too bright, greyish
  blacks, tinted white.
- **Horizontal mirror root cause.** Display Function Control (`0xB6`, SS bit)
  now takes effect, so the image is correct without a mirror call.
- Examples: MISO was hard-coded to GPIO19, which is USB D+ on ESP32-C3 and
  broke the USB-Serial-JTAG console. All examples now use
  `CONFIG_ILI9486_PIN_MISO`.
- `test_app/CMakeLists.txt`: `EXTRA_COMPONENT_DIRS` paths were missing a `/`,
  so the driver and test components could not be found.

### Changed

- Component no longer depends on the legacy `driver` component, which IDF v6
  deprecates. It now `REQUIRES esp_lcd` and `PRIV_REQUIRES esp_driver_gpio`.
- Examples depend on `esp_driver_spi` / `esp_driver_gpio` instead of `driver`,
  and set `rgb_ele_order` instead of `rgb_endian` / `LCD_RGB_ENDIAN_RGB`,
  which IDF v6 removed.
- Mirror workaround removed from `basic_init`, `https` and `lvgl_demo`.
- `.gitignore` ignores per-target build folders (`build_*/`) and generated
  `sdkconfig.*` files, but keeps `sdkconfig.defaults*` tracked. Stray
  `sdkconfig.old` files removed from `https` and `ipcam`.

---

## [1.1.0] - 2026-05-30

### Added

- `examples/ipcam` — streams a live JPEG from an IP camera over HTTP and renders
  it to the panel in 480×320 landscape mode, row by row, with no framebuffer.
  Demonstrates SPI panel init, DMA-backed row writes, FreeRTOS async decode, and
  HTTP streaming with an input prefetch buffer.
- `examples/https` — fetches the classic Lenna 512×512 test image over HTTPS and
  displays it with automatic scale selection and pan control. Demonstrates TLS,
  JPEG scale/pan, and correct resource lifetime across async decode callbacks.

### Changed

- **Pixel format switched from RGB666 to RGB565.** COLMOD is now `0x55` (16-bit)
  instead of `0x66` (18-bit). The RGB565→RGB666 conversion buffer and
  `rgb565_to_rgb666()` function have been removed. Pixel data is sent directly to
  the panel, halving the per-frame memory and CPU overhead.
- **CASET / RASET / RAMWR now use split transactions.** Each command is sent via
  `tx_param` (command only), followed by a separate `tx_param` for the coordinate
  data, then `tx_color` for pixel data. The previous mixed `tx_param` + `tx_color`
  approach was unreliable on this panel.
- **`trans_queue_depth` reduced from 10 to 1** in the panel IO config. Queuing
  multiple transactions ahead caused pixel data to be sent before CASET/RASET
  completed, corrupting the framebuffer address window.
- **Mirror now set via `esp_lcd_panel_mirror(true, false)`** instead of a raw
  MADCTL `0x48` write in the example init code. The driver's init sequence handles
  the BGR bit correctly; the manual override is no longer needed.

### Removed

- `rgb565_to_rgb666()` conversion function and its 25 600-byte static conversion
  buffer (`s_conv_buf`).
- `flush_count` debug variable from `panel_ili9486_draw_bitmap`.
- Manual `esp_lcd_panel_io_tx_param(s_io_handle, 0x36, ...)` MADCTL override from
  `examples/basic_init`.

---

## [1.0.4] - 2026-05-23

### Fixed

- MADCTL default value changed from `0x48` to `0x08` (BGR=1 only).
  MX bit is now exclusively owned by `panel_ili9486_mirror()`, set when
  LVGL requests `mirror_x = true`. This prevents the MX bit being
  double-applied and gives clean separation between the hardware BGR
  quirk and LVGL rotation state. Final wire value remains `0x48`.

### Changed

- `examples/lvgl_demo`: replaced the basic monochrome demo with a
  full colour verification sequence:
  - Full-screen primary colour flashes (red, green, blue, white) to
    catch any R/B swap or missing channel at a glance
  - Rainbow horizontal stripe panel across the full display width
  - Styled button and label widgets with explicit colour assignments
  - Colour-cycling progress bar (red → green → blue passes) to verify
    continuous flushing and per-channel accuracy over time
  - Diagnostic log hints printed on completion to help identify
    byte-swap, RASET, or LVGL task issues

---

## [1.0.3] - 2026-05-21

### Fixed

- MADCTL register never reaching display due to lcd_param_bits=16 SPI
  padding dropping single-byte parameters, causing Red/Blue channel swap
  and green-tinted white on screen
- Introduced ili9486_send_madctl() to send MADCTL parameter via
  tx_color(), bypassing 16-bit word-packing
- MADCTL now sent explicitly during init sequence instead of being
  deferred to first mirror()/swap_xy() call

---

## [1.0.2] - 2026-03-28

### Added

- Added ESP32 connections to wiring diagram
- Added chip support details in README

---

## [1.0.1] - 2026-03-28

### Added

- Added sdkconfig.defaults
- Added connection diagram

---

## [1.0.0] - 2026-03-10

### Added

- Initial release
