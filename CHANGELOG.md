# Changelog
 
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