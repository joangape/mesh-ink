# GPS, RTC and storage

Sources:
- GPS and RTC: `src/board/target.cpp` (GPS section ~L670–1350, `T5RTCClock` ~L100–300)
- Storage: `src/board/t5_storage_backend.cpp`, `partitions.csv`
- LILYGO pinmap §4/§5

## GPS

- **Two possible modules on the same footprint, visually indistinguishable:**
  - **L76K** at 9600 baud. Uses Quectel/CASIC `$PCAS` commands.
  - **u-blox MIA-M10Q** at 38400 baud.
- **Detection is by baud and valid NMEA.** The code tries the current baud, then
  swaps 9600 ↔ 38400. 9600 is classified as L76K, anything else as MIA-M10Q. The
  identity (`gps_supported_identity`) is retained in RTC memory across deep sleep.
  LILYGO's `examples/GPS` probes the same way.
- **UART:** `Serial1`, ESP RX 44 / TX 43. See the naming trap in [pinmap.md](pinmap.md).
- **Power:** shared rail with LoRa. GPS can't be switched off while LoRa is on.
  "GPS off" means the MeshCore provider is stopped and the UART is drained. No
  receiver command is sent.

### PCAS commands MeshInk sends (L76K only, `gps_send_pcas`)

| Command | Meaning in MeshInk |
| --- | --- |
| `PCAS03,1,0,0,0,1,0,0,0,0,0,,,0,0` | Compact NMEA: GGA and RMC only. Applied automatically every boot |
| `PCAS03,1,1,1,1,1,1,1,1,0,0,,,0,0` | Full NMEA (diagnostic, `T5_GPS_FULL_NMEA_DIAGNOSTIC`) |
| `PCAS04,<mask>` | Constellation set: generic bitmask 1..7 = GPS/BDS/GLO |
| `PCAS04,2` plus `PCAS15,2,00000000` and `PCAS15,3,00000000` | "Parked" low-work state (BeiDou-only with zero satellite masks). Used for GPS mode disabled and deep-sleep power save |
| `PCAS15,2,FFFFFFFF` / `PCAS15,3,FFFFFFFF` | Restore satellite masks |

- **The L76K keeps PCAS15 masks across an ESP reset** while its backup domain
  (V_BCKP) stays powered, and PCAS15 has no readback. MeshInk records "masks
  zeroed" in RTC memory (magic "GPZ1") and in NVS namespace `t5-gnss`. It restores
  the masks at startup (`[T5-GPS] startup GNSS satellite masks restored`).
- **Forbidden:**
  - PCAS12 and PMTK161 sleep/standby commands. Testing coincided with losing fixes
    (comment near `gps_command_sleeping`).
  - Any PCAS command to a MIA-M10Q or an unknown receiver.
  - u-blox UBX configuration. MeshInk sends none today; a MIA-M10Q runs on its defaults. Adding UBX is new, hardware-tested work.
- **Fix rate** stays at 1 Hz. Constellation changes apply live.
- **GPS time authority:** the first valid fix is trusted at once if GPS authority is
  stale (>24 h) or was never set. After that, RTC corrections are rate-limited to
  hourly.
- **Diagnostics:**
  - `-DT5_GPS_ACQUISITION_DIAG=1` (on in `t5-unified`) logs `[T5-GPS] SEARCHING …` and `FIRST FIX`.
  - `-DT5_LOG_GPS=1` adds traces.
- `env:t5-unified-cache64-nogps-test` builds with `T5_UI_HAS_GPS=0` to check that
  everything works without a GPS.

## RTC — PCF8563TS (0x51)

- **Registers:** read 7 bytes from 0x02 (seconds…years, BCD). Bit 7 of seconds =
  **VL** (voltage-low), which means the time is invalid. Years 2000–2099 only;
  writes outside that range are refused with a warning.
- **Coin-cell backup:** J12 (LILYGO). Without a cell the RTC loses time on power-off.
- **INT on GPIO2:** not used by MeshInk. It's a possible alarm/wake feature, since
  GPIO2 is RTC-capable for EXT1.
- **`T5RTCClock` policy:**
  - `begin()` runs only once the shared I2C bus is active. Headless deep-sleep
    startup skips it and defers the hardware write.
  - Time sources: GPS (authoritative while fresh), companion app (accepted only when
    expected within ±3 s and GPS isn't authoritative), manual, or hardware RTC.
  - In Manual mode, every external set is rejected. Unsolicited writes are refused
    once the RTC is valid.
  - Modes: Auto / Manual. Metadata lives in NVS namespace `t5-rtc`.
  - MeshInk owns time policy (architecture invariant 5). Protocols consume it and
    don't set it.

## Storage

### Internal flash (`partitions.csv`)

| Name | Offset | Size |
| --- | --- | --- |
| nvs | 0x9000 | 0x5000 (20 KiB) |
| otadata | 0xE000 | 0x2000 |
| app0 / app1 | 0x10000 / 0x610000 | 6 MiB each |
| spiffs | 0xC10000 | 0x3F0000 (~3.9 MiB) |

- `update.bin` is flashed at 0x10000 (app0). `full-wipe.bin` is the whole 16 MiB at 0x0.
- Changing the partition table breaks the update path for existing users.
- **`meshink_storage_mount_internal_safe()`:** if a SPIFFS mount fails, it formats
  **only if every byte of the partition is 0xFF** (a first install). Populated or
  corrupt flash is never auto-formatted (`autoformat refused`). Keep this.
- NVS namespaces seen in board code: `t5-rtc`, `t5-gnss`.

### microSD (SPI CS 12, 25 MHz, FAT with LFN, fast-seek)

- **Content:** `/maps/...` (PNG tiles, PMTiles), backups (`meshink-*-YYYY-MM-DD-HH-MM.BAK`
  or `MTBK0001.BAK` / `MCBK0001.BAK`), screenshots `/SHOTnnnn.BMP`.
- **Mount rules:**
  - reuse an existing mount
  - configure the shared SPI pins before `SD.begin`
  - deselect SD CS
  - no `SD.end()` before the first begin, or after RadioLib has the bus
- **The 32-bit seek limit** means PMTiles data beyond 4 GiB isn't readable.
- `CONFIG_FATFS_USE_FASTSEEK=y` and `CONFIG_FATFS_MAX_LFN=96` are set in
  `sdkconfig.cache64` and checked by CI.
