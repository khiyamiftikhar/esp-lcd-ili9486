# Changelog

All notable changes to this project will be documented in this file.

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