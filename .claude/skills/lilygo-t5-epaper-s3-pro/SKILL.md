---
name: lilygo-t5-epaper-s3-pro
description: Board-specific hardware knowledge for the LILYGO T5 E-Paper S3 Pro (H752-01) as used by MeshInk — pin map, shared I2C/SPI buses, PCA9535 LoRa/GPS rail, SX1262 TCXO/RF-switch init, EPDiy/ED047TC1 display, GT911 touch, BQ25896/BQ27220 power, PCF8563 RTC, L76K/MIA-M10Q GPS, deep-sleep wake, SD card, serial debug commands. Use when editing src/board/ or src/hardware/, adding a peripheral or GPIO use, changing power/sleep/radio/display/touch/GPS behaviour, reading T5 serial logs, or debugging a hardware symptom on the T5.
---

# LILYGO T5 E-Paper S3 Pro (H752-01) — MeshInk hardware skill

This skill is about the physical board, not the mesh protocols. It's built from
MeshInk's field-tested board code (authoritative for behaviour) and LILYGO's
official pin map/schematic notes (authoritative for wiring). Every fact cites its
source so it can be re-checked.

## Golden rules (read before touching hardware code)

1. **All electrical knowledge lives in `src/board/`.** Pins, I2C addresses, PMIC
   registers, bus setup. Everything else uses `src/hardware/*.h`. Enforced by
   `tools/check_architecture.py`.
2. **There are no free GPIOs.** Every MCU pin is committed. GPIO35–37 are taken by
   octal PSRAM. New hardware goes on the shared I2C bus (GPIO39/40) or the
   unconnected PCA9535 port-0 bits. See [references/pinmap.md](references/pinmap.md).
3. **Buses are shared.**
   - The I2C bus (GPIO39/40) carries touch, RTC, fuel gauge, charger, the
     PCA9535 IO expander and the e-paper power IC.
   - The SPI bus (FSPI 13/14/21) carries the SX1262 radio and the SD card.
   - EPDiy installs the I2C driver at cold boot, so don't reinstall it.
   - Never call `SD.end()` before the first `SD.begin()`. Never call it after
     RadioLib has configured SPI: that triggers a FreeRTOS queue assert.
4. **LoRa and GPS share one 3V3 rail** (PCA9535 IO0_0 `LORA_EN`). You can't power
   GPS off independently. Give the rail 1500 ms to settle before touching the
   SX1262. Port 1 of the PCA9535 belongs to EPDiy (display power). Only
   read-modify-write bit IO0_0.
5. **The GPIO ISR service is process-wide.** It's shared between EPDiy and the
   SX1262 DIO1 handler through the patched EPDiy (`MESHINK_SHARED_GPIO_ISR_PATCH_V3`).
   Never call `gpio_install_isr_service()` or Arduino `attachInterrupt()` on DIO1
   directly. Use `meshink_radio_native_attach_interrupt()`.
6. **SX1262 init order is fixed.** RadioLib `std_init` with the default TCXO
   voltage, then `setTCXO(2.4)`, then `setDio2AsRfSwitch(true)`. This matches
   LILYGO's sequence. Both protocols must use it.
7. **Display refresh and flash writes are mutually exclusive.** Use
   `hardware/io_coordinator.h` (`MeshInkFlashGuard`, `meshink_io_display_begin/end`).
8. **Never send unverified commands to the GPS.** No PCAS12 or PMTK161 sleep
   commands. L76K-only PCAS commands must never go to a MIA-M10Q.
9. **Power-off is BQ25896 ship mode** (REG09 BATFET_DIS). On USB power this falls
   back to deep sleep. Don't touch charger current/voltage registers. The fuel-gauge
   profile write is a guarded one-time migration. Don't extend it casually.
10. **Hardware behaviour is only proven on hardware.** Any change to sequencing,
    timing, wake or rails needs a `docs/agent/HARDWARE_TESTS.md` row. Ask the user
    for serial logs. The `[T5-*]` tags are listed below.

## Quick facts

| Item | Value | Source |
| --- | --- | --- |
| MCU | ESP32-S3-WROOM-1, 240 MHz, 16 MB QIO flash, 8 MB octal PSRAM (`qio_opi`) | `boards/T5-ePaper-S3.json`, LILYGO README |
| Display | ED047TC1 4.7", 960×540 physical, 16 gray, EPDiy v7 board, LCD pixel clock 17 MHz | `src/board/t5_display_backend.h` |
| Logical UI | 540×960 portrait = EPDiy `EPD_ROT_INVERTED_PORTRAIT`; landscape = `EPD_ROT_LANDSCAPE` | same |
| Touch | GT911 @ 0x5D, INT GPIO3, RST GPIO9 | `src/board/t5_touch_backend.h` |
| Radio | SX1262: NSS 46, DIO1 10, RST 1, BUSY 47; TCXO 2.4 V; DIO2 = RF switch; current limit 140 mA; RX boosted gain | `platformio.ini`, `src/board/target.cpp` |
| SPI | FSPI SCLK 14 / MISO 21 / MOSI 13, shared by SX1262 + SD (CS 12, 25 MHz) | `board_profile.h`, `t5_storage_backend.cpp` |
| I2C | SDA 39 / SCL 40, 400 kHz, port 0 | `t5_power_backend.cpp` |
| I2C devices | PCA9535 0x20, PCF8563 0x51, BQ27220 0x55, GT911 0x5D, TPS65185 0x68, BQ25896 0x6B (0x6A probed) | LILYGO pinmap §6, board code |
| GPS | UART `Serial1`: ESP RX 44 ← GPS TX, ESP TX 43 → GPS RX. L76K @ 9600 or MIA-M10Q @ 38400 | `platformio.ini`, LILYGO pinmap §4 |
| Frontlight | GPIO11 → PT4103 EN, LEDC ch 6, 5 kHz, 8-bit | `t5_power_backend.cpp` |
| Buttons | BOOT = GPIO0 (active LOW). Side button S3 on PCA9535 IO1_2 (unused by MeshInk) | `t5_buttons_backend.cpp`, LILYGO pinmap §3 |
| Battery | 1-cell Li-ion 1500 mAh; critical cut-off 3300 mV | `t5_power_backend.cpp` |
| Console | Native USB CDC (GPIO19/20), 115200, 4096-byte TX ring set before `Serial.begin` | `src/unified_main.cpp` |
| Download mode | Hold BOOT, tap RST, release BOOT | README, LILYGO README |

## Which reference to read

| Task | Read |
| --- | --- |
| Using or adding any pin, peripheral, I2C device or expander bit | [references/pinmap.md](references/pinmap.md), then run `scripts/pin_audit.py` |
| Battery, charging, power-off, frontlight, deep sleep, wake sources, RTC memory | [references/power-and-sleep.md](references/power-and-sleep.md) |
| SX1262 init, recovery, interrupts, retained-RX wake, the shared SPI bus with SD | [references/radio-sx1262.md](references/radio-sx1262.md) |
| E-paper refresh, rotation, framebuffer, touch, GT911/GT6972P | [references/display-touch.md](references/display-touch.md) |
| GPS modules, PCAS commands, RTC/time authority, SD/SPIFFS safety | [references/gps-rtc-storage.md](references/gps-rtc-storage.md) |
| Interpreting a user's serial log, or asking them for a capture | [references/debugging.md](references/debugging.md) |

## Checklist for any hardware-affecting change

1. Locate the owner. Is it in `src/board/` (T5 backend) or a generic contract in
   `src/hardware/`? A contract change needs `tests/*_mock_backend.h` and its
   `tests/test_*_contract.cpp` updated too.
2. Check pins and addresses against [references/pinmap.md](references/pinmap.md), then run
   this skill's `scripts/pin_audit.py` from the repo root:
   `python3 .claude/skills/lilygo-t5-epaper-s3-pro/scripts/pin_audit.py` (Claude Code), or the
   same path under `.agents/skills/` (Antigravity). Use `--gpio N` to ask what owns a pin,
   or `--map` for the full table. It exits 1 on a conflict.
3. Keep the boot order (`src/unified_main.cpp` `setup()`):
   1. pending SD backup/restore (`meshink_backup_run_pending_on_boot`, before any UI or radio)
   2. `ui_setup()`: EPDiy init (installs I2C), then the LoRa/GPS rail starts and
      its 1500 ms settle runs in parallel with the splash
   3. `mesh_protocol_setup()`, which runs:
      1. `T5Board::beginLocal()`: waits out the rest of the settle, prepares the fuel gauge
      2. `radio_init()`: RTC, then shared-SPI idle, then SX1262 init with TCXO and RF switch, then the GPS probe
   4. SD for Maps on demand
4. Keep both deep-sleep paths intact (fresh sleep and retained re-sleep), including
   their race checks. Deep-sleep standby is MeshCore-only.
5. Build `pio run -e t5-unified-cache64`. Also build `-nogps-test` if the change
   touches GPS-related code.
6. Run `python3 tools/check_architecture.py` and the host tests.
7. Add a `docs/agent/HARDWARE_TESTS.md` row. Say what to look for on the device
   and in the log.

## Known discrepancies and open questions

- **Frontlight PWM frequency:** LILYGO's pinmap says to keep `BL_EN` PWM at or
  below about 1 kHz for the PT4103. MeshInk uses 5 kHz, and that is field-tested.
  If low-brightness dimming is nonlinear or flickers, this is the first suspect.
  Don't change it without a hardware A/B test.
- **EPD pixel clock:** `sdkconfig.cache64` comments mention 20 MHz, but the code
  sets 17 MHz (`epd_set_lcd_pixel_clock_MHz(17)`). The code is authoritative.
- **GT6972P touch:** LILYGO publishes GT6972P examples (active pen, probing
  addresses 0x5D and 0x14) with the same pins. MeshInk only supports GT911 at
  0x5D. Persistent `[T5-TOUCH] ERROR GT911 read` on a unit could mean a different
  controller. Unverified: ask for the diagnostic build's product ID line.
- **H752 (original board):** `board_profile.h` keeps its pins, but no build env
  targets it. It has no PCA9535 rail and no GPS.
- **Pro Lite:** Same H752-01 PCB but without the SX1262 and GNSS. MeshInk
  classifies a failed radio init as `MissingHardwareVariant` when the PCA9535
  answers but no NMEA arrives (`t5_classify_radio_failure()`).

## Upstream references

- LILYGO repo (branch `H752-01`): <https://github.com/Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO>.
  Use `docs/pinmap.md`, `hardware/T5 E-paper S3 Pro V1.0 24-12-24.pdf`, and
  `examples/` (lora_send/recv, GPS, bq25896, bq27220, touch, rtc_pcf8563, io_extend).
- LILYGO wiki: <https://wiki.lilygo.cc/get_started/en/E-Paper/T5-E-Paper-S3-Pro/T5-E-Paper-S3-Pro.html>
- EPDiy: <https://github.com/vroland/epdiy>, pinned in `platformio.ini`.

## Maintaining this skill

The facts here were checked on 2026-11-10 against MeshInk `agent-config`
(base `47c4bb3`) and the LILYGO repo at `87436b5`. When board code changes,
update the matching reference file and keep the citations accurate. This skill
lives in both `.claude/skills/` and `.agents/skills/`. Keep them identical.
