# Progress / session handoff

## Current focus

Fork set up for feature development. Agent configuration created (instructions, rules,
skills, state artifacts). No feature work started yet.

## Next steps

1. Initialise submodules: `git submodule update --init --recursive`.
2. Install PlatformIO (`python3 -m pip install platformio Pillow`) and do a first
   `pio run -e t5-unified-cache64` to confirm the toolchain works on this Mac.
3. Re-run the three submodule-dependent gates (`check_release_integration.py`,
   `check_meshcore_pin.py`, `check_official_meshtastic_pin.py`).
4. Capture the first features in `BACKLOG.md`.

## Blockers / open questions

- PlatformIO works via `python3 -m platformio` (not on PATH). First build verified 2026-11-10.
- Should this fork's web flasher / release links be repointed from `samo-nz` to `joangape`? (Not decided. Links unchanged.)

## Log

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
