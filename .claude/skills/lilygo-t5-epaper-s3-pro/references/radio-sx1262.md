# SX1262 radio on the T5

Sources:
- `src/board/target.cpp`: `radio_init`, `radio_resume_retained`,
  `t5_prepare_native_radio_spi`, `T5RadioHal`
- `src/board/t5_radio_backend.cpp`
- `src/hardware/radio.h` and `radio_types.h`
- `docs/official-meshtastic-core.md`

## Electrical facts

| Item | Value | Source |
| --- | --- | --- |
| Pins | NSS 46, DIO1 10, RESET 1, BUSY 47; SPI 14/21/13 (FSPI) | `platformio.ini` `P_LORA_*` |
| TCXO | 2.4 V; `std_init` runs first at RadioLib's default 1.6 V, then `setTCXO(2.4)` | `radio_init()`, LILYGO H752-01 examples |
| RF switch | DIO2 (`setDio2AsRfSwitch(true)`) | same |
| PA / LNA | `SX126X_CURRENT_LIMIT=140`, `SX126X_RX_BOOSTED_GAIN=1` | `platformio.ini` |
| Default radio | 917.375 MHz, BW 62.5, SF8, CR5, 22 dBm (MeshCore build defaults) | `platformio.ini` `LORA_*` |
| Power | PCA9535 IO0_0 `LORA_EN` rail, shared with GPS; 1500 ms settle | `enableRadioGpsRail()` |
| Native descriptor for Meshtastic | `{P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, 2.4f, true}` | `meshink_radio_native_module_config()` |

The default frequency and preset are regional and regulatory. Don't change them
as a side effect of another change.

## Init sequence (`radio_init`) — up to 3 attempts

0. `meshink_rtc_begin()`. Then on each attempt:
1. **Attempt 1:** `t5_radio_shared_bus_idle(true)`. This ends SPI, drives LoRa CS
   and SD CS HIGH, and releases the RESET hold.
2. **Attempt 2:** detach DIO1 and idle the bus. Then a hardware reset: RESET LOW
   20 ms, HIGH, wait 120 ms.
3. **Attempt 3:** detach DIO1 and idle the bus. Power-cycle the shared rail: off
   250 ms, then on with a 1500 ms settle. This is only safe because it still
   happens before SD is mounted.
4. On each attempt: `radio.std_init(&radio_spi)`, then `setTCXO(2.4)`, then
   `setDio2AsRfSwitch(true)`. Any failure makes the attempt fail. Wait 500 ms
   between attempts.
5. Logs `[T5-INIT] radio=SX1262 OK` or `[T5-ERROR] SX1262 radio initialization failed after recovery attempts`.
6. `meshink_gps_probe_after_radio(ready)`. GPS identification depends on the rail
   being up.

On failure, `t5_classify_radio_failure()` distinguishes:
- `RadioFault`: PCA9535 present and NMEA seen.
- `MissingHardwareVariant`: PCA9535 present but no NMEA, which is likely a Pro Lite
  (no SX1262 or GNSS fitted).
- `Unknown`: PCA9535 absent.

## Two protocol owners, one radio

- **MeshCore** uses `radio_driver` (`MeshInkSX1262Wrapper` over `CustomSX1262`)
  via `meshink_radio_initialize()`.
- **Official Meshtastic** builds its own upstream `SX1262Interface`. It uses
  `meshink_radio_native_module_config()`, `meshink_radio_native_spi_bus()` and
  `meshink_radio_prepare_native_spi_bus()`. It must never call
  `meshink_radio_initialize()`.
- Only one protocol runs per boot. There must be exactly one radio owner.

## Interrupts (DIO1) — process-wide ISR service

- ESP-IDF has one GPIO ISR service per process.
- **Cold UI boot:** EPDiy installs it, for the panel's interrupt.
- **Headless deep-sleep wake:** the radio installs it first.
- `T5RadioHal` overrides RadioLib's `attachInterrupt` to use
  `gpio_isr_handler_add` under MeshInk's own bookkeeping.
- MeshInk exports two hooks:
  - `meshink_epdiy_existing_gpio_isr_service()`: returns true while a radio
    handler is live.
  - `meshink_epdiy_note_gpio_isr_service(bool)`.
- `tools/patch_epdiy_shared_gpio_isr.py` patches the pinned EPDiy
  `epd_board_v7.c` at build time (marker `MESHINK_SHARED_GPIO_ISR_PATCH_V3`, which
  upgrades V1/V2 in place). EPDiy then shares the service and never uninstalls it
  while the radio handler is alive.
- **Rule:** never call `attachInterrupt(P_LORA_DIO_1, …)` or
  `gpio_install_isr_service()` from protocol code. Native code goes through
  `meshink_radio_native_attach_interrupt()`, which refuses to steal a pin another
  radio owner holds.
- **Missed-edge recovery:** `meshink_board_service_asserted_radio_irq()`. If DIO1
  is HIGH with no edge seen, it calls the registered callback. The callback runs
  the normal `readData()`/`finishTransmit()` path, which clears the IRQ.

## Shared SPI with SD

- SD runs at 25 MHz on the same bus with CS 12. Deselect both CS pins before
  touching the rail or re-initialising the radio.
- `t5_prepare_native_radio_spi()` is the native-Meshtastic equivalent of the
  MeshCore bus preparation: RTC begin, bus idle, DIO1/BUSY as inputs, `radio_spi.begin`.

## Raw SX1262 access (deep-sleep wake only)

- `t5_sx1262_raw_read()` works directly on `radio_spi` at 1 MHz, MODE0. It waits
  on BUSY (50 ms timeout) and expects the status byte between the command and the data.
- Use raw access only before RadioLib has rebuilt its state after deep sleep.
  Everywhere else, go through RadioLib or the protocol stack.
- Packet RSSI = −status[2]/2. SNR = (int8)status[1]/4. These match RadioLib.

## Identity entropy

`radio_new_identity()` seeds MeshCore identity generation from SX1262 radio noise
(`RadioNoiseListener`). Don't replace it with `esp_random()` without a decision record.
