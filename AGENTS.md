# AGENTS.md — MeshInk (fork)

Guidance for Google Antigravity (and other AGENTS.md-aware agents) in this
repository. Claude Code uses `CLAUDE.md`. The two files are maintained separately on
purpose: when you change a project fact here (commands, invariants, layout), make
the same change in `CLAUDE.md`.

## What this project is

MeshInk is standalone firmware for the **LILYGO T5 E-Paper S3 Pro** (ESP32-S3,
16 MB flash, octal PSRAM, 540×960 e-paper, GT911 touch, SX1262 LoRa, GPS). It is a
touchscreen mesh messenger that runs **MeshCore or Meshtastic**, one at a time,
selected at boot. C++ / Arduino-on-ESP-IDF via PlatformIO. Version: `3.1.0`.

This is a **fork** (`origin` = `joangape/mesh-ink`). Upstream is
`samo-nz/mesh-ink`. The web flasher, README links and `web/usage-stats.js` still
point to upstream — do not repoint them unless asked.

## Repository map

| Path | Contents |
| --- | --- |
| `src/unified_main.cpp` | Boot/loop composition root for the production firmware |
| `src/ui_onboarding.cpp` (~8k lines) | The entire e-paper UI: screens, keyboard, maps, settings |
| `src/protocol/mesh_protocol.{h,cpp}` | **Protocol-neutral facade** + dispatcher (slot 1 MeshCore, slot 2 Meshtastic) |
| `src/protocol/mesh_protocol_backend.h` | `MeshInkProtocolBackend` contract each protocol helper implements |
| `src/protocol/meshcore_protocol.cpp`, `local_mesh_runtime.*`, `companion_runtime.*`, `meshcore_adapter.h` | MeshCore helper |
| `src/protocol/meshtastic_*.cpp` | Official-Meshtastic helper (in-process PhoneAPI, worker, radio, UI data) |
| `src/hardware/*.h` | Generic hardware contracts (display, touch, radio, gps, rtc, power, storage, …) |
| `src/board/*` | T5-specific backends — the **only** place for pins, buses, chip drivers |
| `src/message_store.*`, `backup_*`, `device_settings_snapshot.*` | Message journal, SD backup/restore, NVS snapshot |
| `src/map_tiles.*`, `pmtiles_reader.*`, `map_gestures.h`, `map_gray.h` | Offline maps |
| `src/ui_data.h` | `UiDataProvider` — the data model the UI consumes |
| `src/fonts/` | Generated Inter raster fonts + emoji (OFL licensed) |
| `lib/MeshCore`, `lib/Meshtastic` | **Pinned upstream git submodules — never edit** |
| `include/meshtastic_compat/` | Headless, pin-free Meshtastic variant shims |
| `tests/` | Host-side C++ tests (`test_*.cpp`), mock backends, stubs |
| `tools/` | PlatformIO pre-build scripts and `check_*.py` static CI gates |
| `web/` | GitHub Pages web flasher (Web Serial + esptool-js) |
| `stats-worker/` | Optional Cloudflare Worker + D1 for flasher counters |
| `docs/` | Architecture, porting and historical development notes |
| `docs/agent/` | **Agent state artifacts** (see below) |

Note: `docs/architecture.md` still mentions the removed "Leaf" Meshtastic library.
`docs/official-meshtastic-core.md` is the current description of the Meshtastic integration.

## Commands

### First-time setup (submodules are NOT checked out after a fresh clone)

```sh
git submodule update --init --recursive
python3 -m pip install platformio Pillow   # pio is not installed on this machine yet
```

### Firmware build (needs submodules + PlatformIO; first build downloads ESP-IDF toolchain)

```sh
pio run -e t5-unified-cache64            # production/release target (what CI ships)
pio run -e t5-unified-cache64-warnings   # warnings audit; then:
python3 tools/check_build_warnings.py <build-log>
```

Other envs: `t5-unified` (base), `t5-unified-cache64-nogps-test`, `t5-unified-cache64-ram-audit`,
`t5-diagnostic`. Default env `t5-pro` is only a base — don't build it to validate changes.
Flashing / serial monitor require physical hardware; **never flash or erase a device
unless the user explicitly asks**.

### Host tests (fast, no hardware, no submodules needed) — mirror CI exactly

Pattern: `g++ -std=<std> -Wall -Wextra -Werror <includes> <sources> -o <out> && <out>`.
Put binaries in a temp dir, not the repo.

| Test | std | Extra flags / sources |
| --- | --- | --- |
| `tests/test_pmtiles_reader.cpp` | c++17 | `-Itests/pmtiles_stubs -Isrc '-DMESHINK_STORAGE_BACKEND_HEADER="storage_backend.h"' src/pmtiles_reader.cpp -lz` |
| `test_channel_key`, `test_backup_format`, `test_backup_integrity`, `test_backup_filename`, `test_setup_short_name`, `test_backup_restore_scratch`, `test_meshtastic_atomic_log`, `test_map_gestures`, `test_utf8_display`, `test_map_gray`, `test_t5_packed_framebuffer` | c++17 | `-Isrc` |
| `tests/test_ui_layout.cpp`, `tools/test_keyboard_geometry.cpp` | **c++11** | `-Isrc` |
| `test_touch_contract`, `test_gps_contract`, `test_board_contract`, `test_rtc_contract`, `test_performance_contract` | c++17 | `-Itests -Isrc` |
| `tests/test_t5_touch_backend.cpp` | c++17 | `-DMESHINK_TOUCH_DIAGNOSTICS=1 -Itests/touch_backend_stubs -Isrc` |

All 20 pass on macOS Apple clang. The source of truth is
`.github/workflows/testing-firmware.yml` — if a test is added there, add it here.

### Static gates (Python 3, stdlib only)

```sh
python3 tools/check_architecture.py        # protocol/board boundary enforcement
python3 tools/check_ui_regressions.py      # UI wiring; asserts on literal source tokens
python3 tools/check_utf8_data_integrity.py
python3 tools/check_compose_utf8_wrap.py
python3 tools/check_backup_integration.py
python3 tools/check_web_flasher.py
# These three need initialized submodules:
python3 tools/check_release_integration.py
python3 tools/check_meshcore_pin.py
python3 tools/check_official_meshtastic_pin.py
```

Many gates assert on **exact source strings** (e.g. `"if(ready)ui_mesh_ready()"`,
`title="CONFIGURE LORA"`). If a refactor trips one, check whether the behaviour it
protects still holds. Only update the gate once you're sure it does, and say so explicitly.

## Architecture invariants (enforced by CI — do not break)

1. **Upstream cores are never patched.** `lib/MeshCore` and `lib/Meshtastic` stay at
   their pinned commits. Adaptation lives in MeshInk code or in idempotent
   build-time scripts under `tools/` that touch only temporary checkouts.
2. **Shared code is protocol-neutral.** `ui_onboarding.cpp`, `unified_main.cpp` and
   `ui_data.h` go through `protocol/mesh_protocol.h` and `UiDataProvider` only. No
   `MeshCore`/`MESHCORE`/`local_mesh_`/`companion_runtime.h` tokens there (logo sprite
   symbols excepted). The generic protocol layer files must not name a backend.
3. **Only `src/board/` touches hardware.** No `pinMode`/`digitalWrite`/`gpio_*`/`i2c_*`,
   `<SPI.h>`/`<Wire.h>`/`<SD.h>`, `P_LORA_*`/`T5_PIN_*`, GT911 or CPU-clock calls
   anywhere else. Use `src/hardware/*.h` contracts. CPU policy goes through `hardware/performance.h`.
4. **MeshCore companion-example internals** are reachable only from
   `src/companion_runtime.cpp` and `src/meshcore_adapter.h`.
5. **MeshInk owns shared product services**: device name, message journal, unread
   totals, GPS receiver, RTC/time policy. These must not move back into the backend contract.
6. Protocol-specific behaviour is exposed through **capability flags** and helper-owned
   settings rows, not `if (protocol == X)` in shared UI.
7. New boards are added as a board package selected at compile time
   (`docs/HARDWARE_PORTING.md`), never as scattered board conditionals.

## Engineering constraints

- **Memory**: internal DRAM is tight; large buffers go to PSRAM (see `psram_scratch<>`
  usage). Avoid big stack objects in loop/task context. Avoid heap churn in hot paths.
  Keep BLE and large UI allocations from coexisting unless measured.
- **E-paper**: refreshes are slow and visible. Coalesce, refresh only changed regions.
  Keep touch sampling independent of refresh. Every drawn tap target must match its
  hit-test geometry (`ui_layout.h`, `keyboard_geometry.h`).
- **Text is UTF-8** end to end. Respect code-point boundaries when wrapping, editing
  or clipping (`utf8_display.h`, `keyboard_utf8_input.h`).
- **Persistent data is user data.** Message journals, NVS settings, backup archive
  format and restore transactions must stay backward compatible. Never add code that
  silently deletes or reformats user data. Restore paths must stay crash-safe and roll back cleanly.
- **Radio**: never silently disable LoRa receive in a mode meant to receive. Region
  and preset defaults are regulatory — don't change them casually.
- Match the existing style: `snake_case` free functions with module prefixes
  (`mesh_protocol_*`, `meshink_radio_*`), `#pragma once`, terse formatting, explanatory
  comments on *why*. Raw allocation patterns already in the code are acceptable. Don't
  impose generic rules from skills (e.g. "never raw new/delete") on existing code.

## Release and CI safety

- A push to `main` builds **and packages** release images. A commit message on `main`
  containing **`[publish-release]`**, or pushing a `v*` tag, **creates a public GitHub
  release**. Never use either without the user's explicit instruction.
- The version lives in `platformio.ini` (`T5_FIRMWARE_VERSION` and `T5_UI_VERSION`),
  `README.md` and the release notes in `.github/workflows/t5-ui-binary.yml`. Bump them together.
- Don't push, open PRs (especially to upstream `samo-nz/mesh-ink`) or retag without being asked.

## Verification standard

"Done" means you've run the relevant host tests and gates and seen them pass, plus a
firmware build when C++ under `src/` changed and the toolchain is available. Report
what you ran. **CI or a build passing is not proof the feature works on hardware.**
Say plainly what still needs testing on a real T5. Record it in
`docs/agent/HARDWARE_TESTS.md` (see below).

## Agent state artifacts — `docs/agent/`

Read these at the start of any non-trivial session. Update them before you finish:

- `PROGRESS.md` — current focus, what was done, next steps, blockers (session handoff).
- `BACKLOG.md` — planned features with acceptance criteria and status.
- `DECISIONS.md` — append-only decision records for fork-specific design choices.
- `HARDWARE_TESTS.md` — what was verified on real hardware vs. only host/CI.
- `README.md` — how the artifacts and installed skills are organised (with provenance).

## Skills and rules

- Workspace skills in `.agents/skills/`: `lilygo-t5-epaper-s3-pro` (**this board's** pins, buses, power,
  radio, display, touch, GPS, deep sleep, serial debugging; project-authored. Prefer it over the
  generic `esp32` skill for anything board-specific), `esp32` (ESP32/ESP-IDF/PlatformIO reference),
  `systematic-debugging`, `test-driven-development`, `verification-before-completion`.
  Vendored at pinned commits. **This file and `.agents/rules/` take precedence over generic skill advice.**
  For TDD here, "tests" means the host tests in `tests/`, built with the commands above.
  Cross-references like `superpowers:test-driven-development` inside the skills point to
  the skill of the same name in `.agents/skills/`.
- Workspace rules in `.agents/rules/` use `trigger: glob` and activate when you work on
  matching files. Keep this file under Antigravity's 24,000-byte per-rule limit.
- Use the terminal for builds and tests. Don't use browser automation against the web
  flasher with real hardware attached.
