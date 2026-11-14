# Progress / session handoff

## Current focus

WiFi Mode and Meteo Screen (F-001) fully implemented and compiled. All host tests,
architecture checks and UI regression gates passing. Waiting on hardware verification
on physical LILYGO T5 device.

## Next steps

1. Physical hardware test (HW-003): flash `firmware.bin` or `update.bin`, enable WiFi in
   Settings, verify reboot into 3-tab UI, connect to local AP, and check weather fetching.
2. (Optional future enhancement): Add multi-day weather forecast or weather icon bitmaps.

## Blockers / open questions

- PlatformIO works via `python3 -m platformio` (not on PATH). First build verified 2026-11-10.
- Should this fork's web flasher / release links be repointed from `samo-nz` to `joangape`? (Not decided. Links unchanged.)

## Log

### 2026-11-12 — WiFi Mode and Meteo Screen (F-001)

- Added boot-isolated WiFi mode with NVS persistence (`"wifi_mode"` in `"t5-ui"`).
- Implemented `t5_wireless_backend.h` WiFi connection loop and non-blocking Open-Meteo HTTP client
  (plain HTTP over port 80 to avoid mbedTLS RAM overhead).
- In WiFi mode, Meshtastic and MeshCore runtimes and LoRa initialization are bypassed on boot.
- UI transitions into 3-tab navigation (`METEO`, `MAPS`, `MORE` at 180px each).
- Added `WifiSettings` screen with on-screen keyboard support for SSID/password entry and status display.
  Fixed keyboard invocation (`draw_keyboard()` backdrop, HIDE/SPACE/DONE key actions, cursor indicator,
  and bottom nav occlusion suppression during editing).
- Password field displays plain text characters (with cursor `_` when focused) instead of asterisks `*`.
  Expanded `wifi_pass` buffer to 64 bytes to accommodate full 63-char WPA keys.
- Added `Meteo` screen displaying current temperature, apparent temperature, high/low, humidity,
  wind speed, condition description, and coordinates from last known GPS fix (`"map_fix_v2"`).
- All 20 host tests pass; `check_architecture.py` and `check_ui_regressions.py` pass.
- Release firmware target `t5-unified-cache64` builds cleanly (RAM 43.3%, flash 39.6%).

### 2026-11-10 — DEVICE INFO page (first feature, "hello world")

- Added `Screen::DeviceInfo` and a `DEVICE INFO` row in MORE (before HELP) in `src/ui_onboarding.cpp`.
  Read-only cards: POWER (battery %/mV, charge, source), DEVICE (chip, eFuse-MAC serial, flash, PSRAM),
  STATUS (core, node, mesh, GPS, SD, firmware). No new hardware access outside existing contracts.
- MORE now has up to 7 rows, so `more_row_y/more_row_h` use a tighter pitch when count > 6.
- Ran: check_architecture, check_ui_regressions, check_utf8_*, check_compose_utf8_wrap, test_ui_layout: all pass.
- Built `t5-unified-cache64` OK (RAM 41.1%, flash 34.9%). `pio` is not on PATH; use `python3 -m platformio`.
- NOT done: flashing and any on-device check.

### 2026-11-10 — LilyGO T5 hardware skill

- No LilyGO- or T5-specific skill exists online (searched skills.sh, GitHub, registries).
  Wrote `lilygo-t5-epaper-s3-pro` from `src/board/` plus LILYGO's official pinmap/schematic
  notes. It has 6 reference files and `scripts/pin_audit.py` (passes on the repo; a
  negative test detects planted conflicts).
- Discrepancies found and documented (not changed): frontlight PWM 5 kHz vs LILYGO's
  ≤1 kHz advice; stale 20 MHz pixel-clock comment in `sdkconfig.cache64` (the code uses 17 MHz);
  LILYGO ships GT6972P touch examples, but MeshInk supports only GT911.
- Unused board features that could become backlog items: RTC alarm interrupt (GPIO2,
  can wake from deep sleep), side button S3 (PCA9535 IO1_2).

### 2026-11-05 — Agent configuration

- Added `CLAUDE.md`, `AGENTS.md`, `.claude/rules/*`, `.agents/rules/*`, vendored skills,
  `docs/agent/*`.
- Baseline: all 20 host C++ tests pass on macOS (Apple clang). Static gates pass except
  the three that need initialised submodules.
