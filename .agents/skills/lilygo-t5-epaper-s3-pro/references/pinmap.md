# T5 E-Paper S3 Pro (H752-01) pin map and bus ownership

Sources:
- **[L]** LILYGO `docs/pinmap.md` (compiled from `hardware/T5 E-paper S3 Pro V1.0 24-12-24.pdf`),
  repo `Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO` @ `87436b5`
- **[B]** `src/board/board_profile.h`
- **[P]** `platformio.ini` `[env:t5-pro]` / `[env:t5-unified]` build flags
- **[E]** EPDiy `epd_board_v7`. MeshInk never names these pins.
- **[S]** Espressif ESP32-S3 datasheet (module-level facts)

## 1. MCU GPIO — every pin is committed

| GPIO | Function | Owner in MeshInk | Notes | Src |
| --- | --- | --- | --- | --- |
| 0 | BOOT button | `t5_buttons_backend.cpp`, deep-sleep EXT0 wake (LOW) | **Strapping** (download mode when LOW at reset). `INPUT_PULLUP` | B, L, S |
| 1 | SX1262 RESET | `target.cpp` (`P_LORA_RESET`) | Held HIGH via `gpio_hold_en` during deep sleep | P, L |
| 2 | RTC INT (PCF8563) | **unused** | RTC-capable GPIO, so it could become an alarm wake source (EXT1) | L |
| 3 | GT911 INT | `t5_touch_backend.h` | **Strapping** (JTAG source). Driven LOW during the GT911 reset to select 0x5D | B, L, S |
| 4 | EPD CKH | EPDiy | | E, L |
| 5,6,7 | EPD D0,D1,D2 | EPDiy | | E, L |
| 8 | EPD D7 | EPDiy | | E, L |
| 9 | GT911 RST | `t5_touch_backend.h` | Held LOW = touch powered down (`meshink_touch_set_power(false)`) | B, L |
| 10 | SX1262 DIO1 (IRQ) | `target.cpp` (`P_LORA_DIO_1`) | Deep-sleep EXT1 wake ANY_HIGH | P, L |
| 11 | Frontlight `BL_EN` → PT4103 EN | `t5_power_backend.cpp` | LEDC ch 6, 5 kHz, 8-bit. LILYGO advises ≤1 kHz (see SKILL.md) | B, L |
| 12 | SD CS | `t5_storage_backend.cpp` | Must be HIGH whenever the radio uses SPI | B, L |
| 13 | SPI MOSI | shared SX1262 + SD | FSPI host (`SPIClass radio_spi(FSPI)`) | B, P, L |
| 14 | SPI SCLK | shared | | B, P, L |
| 15,16,17,18 | EPD D3,D4,D5,D6 | EPDiy | | E, L |
| 19, 20 | USB D−/D+ | USB-Serial/JTAG console | Never reuse | L, S |
| 21 | SPI MISO | shared | | B, P, L |
| 26–32 | SPI flash / PSRAM bus | ROM/IDF | Never touch (`CONFIG_SPIRAM_CS_IO=26`, `CLK_IO=30`) | S, `sdkconfig.cache64` |
| 33–37 | Octal PSRAM data | ROM/IDF | **Unavailable with `qio_opi`**. LILYGO lists 35–37 as "no net", but they're still not usable | S, L |
| 38 | PCA9535 INT | **unused** | Not RTC-capable, so it can't wake from deep sleep | L |
| 39 | I2C SDA | shared bus (port 0) | | B, P, L |
| 40 | I2C SCL | shared bus | | B, P, L |
| 41 | EPD STH | EPDiy | | E, L |
| 42 | EPD LE(H) | EPDiy | | E, L |
| 43 | UART0 TXD → GPS RX | `Serial1` TX | Repurposed UART0 pins; the console is USB CDC | P, L |
| 44 | UART0 RXD ← GPS TX | `Serial1` RX | | P, L |
| 45 | EPD STV | EPDiy | **Strapping** (VDD_SPI voltage). Never add pull-ups | E, L, S |
| 46 | SX1262 NSS | `target.cpp` | **Strapping** (ROM log / boot mode). Driven HIGH only after boot | P, L, S |
| 47 | SX1262 BUSY | `target.cpp` | Input | P, L |
| 48 | EPD CKV | EPDiy | | E, L |

### GPS macro naming trap

`platformio.ini` defines `PIN_GPS_TX=44` and `PIN_GPS_RX=43`, named from the **GPS
module's** point of view. `HardwareSerial::setPins(rx, tx)` is called as
`Serial1.setPins(PIN_GPS_TX, PIN_GPS_RX)`, so the ESP's RX is 44 and its TX is 43.
That's correct, despite how it reads (comment in `T5Board::begin()`). LILYGO's
`docs/pin_define.md` uses the opposite naming (`BOARD_GPS_RXD 44`, `BOARD_GPS_TXD 43`),
but the wiring is the same.

## 2. PCA9535PW I/O expander (I2C 0x20, INT → GPIO38) [L]

| Bit | Net | Dir | Owner | Rule |
| --- | --- | --- | --- | --- |
| IO0_0 | `LORA_EN`, the shared LoRa + GPS 3V3 rail | out | `t5_set_radio_gps_rail()` in `target.cpp` | Read-modify-write **only this bit**. Write the output latch (reg 0x02) before the config (reg 0x06) so the rail can't glitch. Verify by reading back |
| IO0_1–IO0_7 | NC | — | free | Not connected on the PCB. Usable only with hardware rework |
| IO1_0 | `EP_OE` | out | EPDiy | Never write |
| IO1_1 | `EP_MODE` | out | EPDiy | Never write |
| IO1_2 | `BUTTON` (side button S3) | in | **unused** | Readable via input port 1 (reg 0x01). Reading clears the PCA interrupt |
| IO1_3 | `TPS_PWRUP` | out | EPDiy | Never write |
| IO1_4 | `VCOM_CTRL` | out | EPDiy | Never write |
| IO1_5 | `TPS_WAKEUP` | out | EPDiy | Never write |
| IO1_6 | `TPS_PWR_GOOD` | in | EPDiy | |
| IO1_7 | `TPS_INT` | in | EPDiy | |

PCA9535 registers: 0x00/0x01 input, 0x02/0x03 output, 0x04/0x05 polarity, 0x06/0x07 config
(1 = input). The rail code touches only 0x02 and 0x06, bit 0.

The PCA9535 keeps its outputs while the ESP32 deep-sleeps. That's why the SX1262 can
keep listening in deep-sleep standby.

## 3. I2C bus (port 0, SDA 39 / SCL 40, 400 kHz)

| Addr | Device | Owner | Notes |
| --- | --- | --- | --- |
| 0x20 | PCA9535PW expander | `target.cpp` (bit IO0_0), EPDiy (port 1) | |
| 0x51 | PCF8563TS RTC | `T5RTCClock` in `target.cpp` | Coin-cell backup via J12. LILYGO's README once said PCF85063; corrected 2026-08-26 |
| 0x55 | BQ27220 fuel gauge | `t5_power_backend.cpp` | Wait ≥66 µs bus-free after each read (code waits 70 µs) |
| 0x5D | GT911 touch | `t5_touch_backend.h` | Address selected by the INT-LOW-during-reset sequence |
| 0x68 | TPS65185 / TPS651851 EPD PMIC | EPDiy | **Never write** |
| 0x6B | BQ25896 charger | `t5_power_backend.cpp` | Code also probes 0x6A |

Bus ownership:
- **Cold interactive boot:** EPDiy's `epd_init()` installs the I2C driver. Board code
  then uses raw `i2c_master_write_read_device(I2C_NUM_0, …)` and must not reinstall it.
  `T5Board::beginLocal()` explains: "Reinstalling the legacy I2C driver here would abort".
- **Companion (BLE) mode:** EPDiy is released first. Then MeshCore's
  `ESP32Board::begin()` takes the bus.
- **Headless deep-sleep wake:** `meshink_power_begin_minimal_bus()` installs a
  driver only if none exists. On IDF 4.4, `ESP_FAIL` means one already exists, so
  it reuses it and never deletes it.

New I2C devices:
- Must not collide with the addresses above.
- Must use 20–50 ms timeouts like the existing code.
- Must tolerate the bus being busy during EPD refreshes.

## 4. SPI bus (FSPI: SCLK 14, MISO 21, MOSI 13)

- **SX1262:** NSS 46. RadioLib on `radio_spi`.
- **microSD:** CS 12, at `T5_STORAGE_SPI_HZ = 25 MHz`.
- **Before powering the rail or re-initialising the radio:** call
  `t5_radio_shared_bus_idle()`. It ends SPI, drives both CS pins HIGH, and releases
  the RESET hold. This matches LILYGO's LoRa examples.
- **SD mount rules** (`t5_storage_backend.cpp`):
  - reuse an existing mount
  - never call `SD.end()` before the first `SD.begin()`
  - `SD.end()` after RadioLib configured SPI can hit a null FreeRTOS queue assert
    (development-notes test.57)

## 5. Expansion options (ranked)

1. **I2C device** on 39/40 at a free address. No new pins. Respect the shared-bus timing.
2. **Existing unused nets:** RTC INT (GPIO2, RTC-capable, so it can be a wake source),
   PCA9535 INT (GPIO38), side button (PCA IO1_2).
3. **PCA9535 IO0_1–IO0_7:** not connected. Needs soldering. Slow (I2C), so not for
   interrupts or fast signals.
4. **Anything else** means taking a pin from an existing function. Don't do it without
   an explicit hardware decision recorded in `docs/agent/DECISIONS.md`.

Run `scripts/pin_audit.py` after any pin-related change.
