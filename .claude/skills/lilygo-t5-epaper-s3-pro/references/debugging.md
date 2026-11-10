# Debugging on the T5: serial logs, commands, symptoms

## Getting a log from the user

- **Port:** native USB CDC at 115200 (`/dev/cu.usbmodem*` on macOS, `/dev/ttyACM*` on
  Linux). With PlatformIO: `pio device monitor -b 115200`. To capture the full boot,
  start the monitor first, then press RESET. The USB CDC port re-enumerates on reset,
  so the first lines can be missed. Ask for a second capture if the log starts mid-boot.
- **Serial commands** (`src/unified_main.cpp`, type the command and press Enter):
  - `help`: lists the commands.
  - `screenshot` or `shot`: arms capture mode. Each further Enter saves
    `/SHOTnnnn.BMP` to the SD card (`[T5-SHOT] saved …`). Disconnecting the serial
    port ends capture mode. **Use this to verify UI changes:** ask the user for the BMP.
  - `wakelog`: replays the RTC-retained deep-sleep wake transcript. It's also
    replayed automatically when a serial host connects.
- **Debug builds:**
  - Verbose everything: add `-DT5_DIAGNOSTICS=1` (default 0 in `t5-unified`).
  - One subsystem: `-DT5_LOG_BOARD=1`, `T5_LOG_GPS`, `T5_LOG_UI`, `T5_LOG_MESH` or
    `T5_LOG_MAP` (`src/t5_logging.h`). `T5_TRACE` output is prefixed `[T5]`.
  - Touch: `-DMESHINK_TOUCH_DIAGNOSTICS=1`.
  - Already on in `t5-unified`: `MESHINK_MESHTASTIC_HW_TEST_LOG=1` and
    `T5_GPS_ACQUISITION_DIAG=1`.
- **Never flash a debug build** onto the user's device without asking. Point them
  to the web flasher's custom-BIN path, or to `esptool write-flash 0x10000` for an
  update image.

## Log tags

| Tag | Subsystem / where |
| --- | --- |
| `[T5-INIT]` | Boot milestones: `radio=SX1262 OK`, `rtc=PCF8563 OK`, `battery-gauge=OK voltage=…` |
| `[T5-ERROR]` / `[T5-WARN]` | Board-level failures. Always grep these first |
| `[T5-DEEPSLEEP]`, `[T5-WAKELOG]` | Sleep entry/deferral, wake pads, raw FIFO capture, retained re-sleep |
| `[T5-POWER]`, `[T5-SHUTDOWN]` | Critical-battery checks, ship mode |
| `[T5-GPS]` | Module and baud detection, first fix, mask restore |
| `[T5-TOUCH]` | GT911 identity and rate-limited I2C errors |
| `[T5-STORAGE]` | SPIFFS mount and autoformat refusal |
| `[T5-EPD]`, `[T5-UI]`, `[T5-MAP]`, `[T5-PMT]` | Display, UI, maps, PMTiles |
| `[T5-STORE]`, `[T5-BACKUP]`, `[T5-RESTORE]` | Message store, SD backup/restore |
| `[T5-MESH]`, `[MT-*]`, `[MeshInk/MT]` | Protocol runtimes (MeshCore and Meshtastic) |
| `[MeshInk/IO]` | Flash/display arbitration (`display waited …ms for flash`, deferred Meshtastic saves) |
| `[T5-CMD]`, `[T5-SHOT]` | Serial terminal and screenshots |

## Symptom → first suspects

| Symptom | Check | Likely area |
| --- | --- | --- |
| `SX1262 radio initialization failed after recovery attempts` | `PCA9535 power-rail …` errors? Radio failure class? | Rail (PCA9535 0x20 not answering), Pro Lite without a radio, or a SPI bus conflict with SD |
| `TCXO 2.4V setup failed` / `DIO2 RF-switch setup failed` | Order of `std_init` → TCXO → RF switch | Someone changed the init sequence or the TCXO voltage |
| Radio OK but no RX after a while | `[T5-DEEPSLEEP] DIO1 level recovery` lines; ISR service messages | DIO1 handler lost: an EPDiy deinit or a second `attachInterrupt` broke the shared ISR service |
| FreeRTOS `xQueue…` assert after a backup/restore or opening Maps | Was `SD.end()` called? | SD teardown on the shared SPI bus (development-notes test.57) |
| Device won't deep-sleep | `sleep deferred: BOOT is still held` / `DIO1 already asserted` / `BUSY asserted` | Usually a correct deferral. Repeated DIO1 means unhandled radio IRQs |
| Wakes from deep sleep but the message is missing | `raw wake packet captured BEFORE RadioLib init` present? | Raw FIFO capture order. RadioLib must not touch the chip first |
| Touch dead | `[T5-TOUCH] ERROR GT911 read …` repeating | I2C bus stuck, reset sequence changed, or a GT6972P unit (see display-touch.md) |
| Touch offset or mirrored in landscape | `meshink_touch_set_orientation` called with the display rotation change? | Orientation mismatch between display and touch |
| Ghosting / gray artefacts | Refresh mode choices | DU overuse. Needs an occasional GC16 |
| Display glitches during saves | `[MeshInk/IO]` lines | A flash write bypassed the lease |
| No GPS fix ever | `[T5-GPS] SEARCHING module=… baud=… NMEA=…` | NMEA=0 means wrong baud or the rail is off. Masks zero means the parked state wasn't restored |
| Battery % nonsense | `[T5-ERROR] battery gauge profile …` | Gauge migration failed. Don't retry blindly; the gauge may be in an unknown state |
| `autoformat refused` | — | Correct safety behaviour. Never "fix" it by formatting |
| Won't power off on USB | Expected. Ship mode can't cut USB-fed SYS, so it falls back to deep sleep | — |

## Bring-up / bricked-looking device

- **Download mode:** hold BOOT, tap RST, release BOOT. Then esptool works even if
  the firmware crash-loops.
- **Release images:** `update.bin` at 0x10000 keeps data. `full-wipe.bin` at 0x0
  erases **everything**. Never suggest the full wipe while data or backups might
  still matter (see `docs/development-notes.md` on preserving restore evidence).
- **Crash backtraces:** CI keeps the matching `firmware.elf` per commit SHA.
  Decode against that exact ELF, not a rebuild.
