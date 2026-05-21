# Changelog

All notable changes to this project will be documented in this file.

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