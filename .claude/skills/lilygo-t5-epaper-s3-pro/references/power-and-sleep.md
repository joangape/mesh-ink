# Power, battery, frontlight and deep sleep

Primary source: `src/board/t5_power_backend.cpp`. Deep sleep: `src/board/target.cpp`
(`meshink_board_enter_deep_sleep_standby`, `meshink_board_return_to_retained_deep_sleep`,
`t5_enable_deep_sleep_wake_sources`). Generic contract: `src/hardware/power.h`, `power_types.h`.

## Power tree (from LILYGO pinmap + code)

- **Battery:** single-cell 3.7 V Li-ion, 1500 mAh.
- **Charger / power path:** BQ25896 (I2C 0x6B; the code also probes 0x6A).
- **Fuel gauge:** BQ27220 (I2C 0x55).
- **Shared LoRa + GPS 3V3 rail:** switched by PCA9535 IO0_0 `LORA_EN`. The original
  H752 has no rail gate (`t5_set_radio_gps_rail` returns true).
- **EPD high voltages:** TPS65185 (0x68), sequenced by EPDiy via PCA9535 port 1.
- **Frontlight:** PT4103 LED boost, EN = GPIO11 PWM.

## BQ25896 charger (what MeshInk reads/writes)

| Register | Use | Code |
| --- | --- | --- |
| REG0B bits 4:3 `CHRG_STAT` | 0 idle, 1/2 charging, 3 full | `meshink_power_read_charge_state` |
| REG0B bit 2 `PG_STAT` | external (VBUS) power present | `meshink_power_external_present` |
| REG09 bit 5 `BATFET_DIS` | ship mode = power off | `meshink_power_enter_ship_mode` |
| REG09 bit 3 `BATFET_DLY` | cleared, so ship mode is immediate | same |
| REG09 bit 2 `BATFET_RST_EN` | set, so a button press can restore power | same |

- **Power off:**
  1. Find the charger, then set `BATFET_DIS | BATFET_RST_EN` and clear `BATFET_DLY`.
  2. On battery alone, SYS power drops and execution stops.
  3. If execution continues, USB is present, so it falls back to deep sleep (BOOT wake).
     It never resumes the UI.
- **The user wakes from ship mode with the PWR button** (`T5_WAKE_INFO` strings:
  "PRESS PWR TO START AGAIN" / "ON USB: HOLD BOOT TO WAKE").
- **At boot, `meshink_power_recover_boot_path()`** clears a stale `BATFET_DIS` so a
  ship-mode flag can't strand the device.
- **Never write** charge current, charge voltage, input limit or watchdog registers.
  The board relies on the BQ25896 defaults.

## BQ27220 fuel gauge

- **Standard commands used:** 0x08 voltage (mV), 0x2C state of charge (%),
  0x3C design capacity, 0x0A battery status, 0x3A operation status, 0x12 full charge capacity.
- **Plausibility filters:** voltage must be 2500–5000 mV and SOC ≤100, otherwise
  the reading is treated as unavailable.
- **Bus timing:** wait at least 66 µs bus-free after each read at 400 kHz (TI
  SLUUBD4A). The code waits 70 µs.
- **`meshink_power_prepare_board()` → `gauge_apply_factory_profile_if_needed()`:**
  a one-time migration from the factory 3000 mAh setting to LILYGO's 1500 mAh CEDV
  profile (`lib/BQ27220/bq27220_data_memory.c`, H752-01).
  - It runs only when the gauge reports design capacity 3000, sane voltage and
    expected flags. Otherwise it leaves an unknown or replacement battery alone.
  - Sequence: unseal (0x0414/0x3672), full access (0xFFFF×2), CFGUPDATE (0x0090),
    then per-field MAC write with checksum and readback.
  - On failure it rolls back, exits CFGUPDATE (0x0092) and reseals (0x0030).
    Failures are logged as `[T5-ERROR] battery gauge …`.
  - Data memory only. No OTP writes.
  - Called from both `T5Board::begin()` (companion) and `beginLocal()` (UI).
- **Treat this as frozen.** Changing gauge data memory can make battery % wrong
  on every device.

## Critical-battery protection

- Threshold: `T5_CRITICAL_BATTERY_MV = 3300`.
- **Runtime:** sample every 5 s and require 3 consecutive low samples. External
  power resets the counter.
- **Boot and pre-deep-sleep:** two samples 80 ms apart, with up to 3 retries each.
  External power suppresses only if both samples agree it's present. Results:
  `Safe`, `Critical`, `ExternalPower` or `Unavailable`.
- When critical, the device enters ship mode with reason `LowBattery`.

## Frontlight

- GPIO11, LEDC channel 6, 5000 Hz, 8-bit. 0% = off; any non-zero percent maps to a
  duty of at least 1.
- LILYGO recommends PWM ≤ about 1 kHz for the PT4103 EN pin. 5 kHz is field-tested
  here. Treat a change as a hardware experiment.
- The frontlight is forced off in companion (BLE) mode (`T5Board::begin()`).

## Deep-sleep standby (MeshCore only)

The ESP32-S3 sleeps. The SX1262 stays powered (PCA9535 keeps `LORA_EN` on) and in
continuous RX.

**Wake sources** (`t5_enable_deep_sleep_wake_sources`):
- **EXT0:** GPIO0 BOOT, LOW.
- **EXT1:** GPIO10 DIO1, ANY_HIGH.
- **Timer:** 1 h battery check (`T5_DEEP_SLEEP_BATTERY_CHECK_US`).

**Entering sleep** (`meshink_board_enter_deep_sleep_standby`), in this order:
1. Defer if BOOT is held or DIO1 is already HIGH.
2. `radio.startReceive()` re-arms RX and re-applies the RX_DONE→DIO1 mapping.
   Defer on error, or if BUSY or DIO1 is high afterwards.
3. Arm the wake sources.
4. Drive NSS and RESET HIGH and `gpio_hold_en` both, so the radio isn't reset or
   selected while the GPIO domain sleeps. Then `gpio_deep_sleep_hold_en()`.
5. Final race check on BOOT and DIO1. On failure, undo the holds and wake sources.
6. Only then reset the RTC wake transcript and put the GNSS in low-work mode
   (BeiDou zero mask, L76K only and only if the user enabled it).
7. `Serial.flush()`, 20 ms, `esp_deep_sleep_start()`.

**Wake paths:**
- **Packet wake (EXT1):** `radio_resume_rx_wake()`. Release the holds (program
  HIGH first, then `gpio_hold_dis`, verify levels). Copy the FIFO with **raw SX1262
  SPI commands** (GetIrqStatus 0x12, GetRxBufferStatus 0x13, GetPacketStatus 0x14,
  ReadBuffer 0x1E) *before* RadioLib re-initialises. Then a clean `std_init` with
  TCXO and RF switch, and the saved packet is injected into the first `recvRaw()`.
  Reason: the ESP reset loses RadioLib's state while the radio's FIFO survives.
- **Button wake (EXT0):** `radio_resume_retained_wake()` (no packet capture).
- **Timer or short accidental BOOT wake:**
  `meshink_board_return_to_retained_deep_sleep()`. It doesn't touch the radio, re-arms
  the wake sources and sleeps again.
- `meshink_board_restore_deep_sleep_wake_pads()` releases the RTC-IO holds and
  `rtc_gpio_deinit`s DIO1/BOOT before normal GPIO use.

**RTC memory (`RTC_DATA_ATTR`) that survives deep sleep:**
- retained UI tab (magic "MUI2")
- GPS module and baud identity
- GNSS low-work magic "GPZ1"
- the wake-log transcript, replayed with the serial `wakelog` command

A cold boot must never trust these. Each is guarded by a magic number plus
`esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED`.

`CONFIG_ESP_SLEEP_PSRAM_LEAKAGE_WORKAROUND=y` is set in `sdkconfig.cache64`.

## Wireless off-state

`t5_wireless_backend.h` verifies the final state, not intermediate errors. WiFi must
be `ESP_ERR_WIFI_NOT_INIT`. The BT controller must be IDLE and Bluedroid UNINITIALIZED.
`force_*_off()` tear down in the right order (bluedroid disable → deinit → controller
disable → deinit).

## Rules for changes

- Measure, don't assume. Ask for current measurements or at least the
  `[T5-POWER]` / `[T5-DEEPSLEEP]` logs. Record them in `docs/agent/HARDWARE_TESTS.md`.
- Never add a sleep path that leaves the radio unable to receive in a mode that's
  meant to receive.
- Keep every deferral and race check. Each one fixed an observed wake failure
  (comments reference the deepsleep26 and pre-Heltec behaviour).
