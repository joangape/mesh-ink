# E-paper display and touch

Sources:
- `src/board/t5_display_backend.h`, `src/board/t5_packed_framebuffer.h`
- `src/board/t5_touch_backend.h`, `src/hardware/io_coordinator.{h,cpp}`
- `sdkconfig.cache64`, `tools/patch_epdiy_shared_gpio_isr.py`
- LILYGO pinmap §2/§5

## Panel and driver

| Item | Value |
| --- | --- |
| Panel | ED047TC1, 4.7", 960×540 physical, 16 gray levels |
| Driver | EPDiy pinned at `403ba3b8…` in `platformio.ini`. `epd_init(&epd_board_v7, &ED047TC1, EPD_LUT_64K)` |
| Pixel clock | `epd_set_lcd_pixel_clock_MHz(17)`, the field-tested value. The `sdkconfig.cache64` comment about 20 MHz is stale |
| HV power | TPS65185 (0x68) plus PCA9535 port 1 (OE, MODE, PWRUP, VCOM, WAKEUP, PWR_GOOD, INT). EPDiy owns all of it |
| Waveform | `epd_hl_init(EPD_BUILTIN_WAVEFORM)` |
| Temperature | `epd_ambient_temperature()` is passed to every update. Waveform timing depends on it |

### Refresh modes (`meshink_display_native_refresh_mode`)

| MeshInk | EPDiy | Use |
| --- | --- | --- |
| `Direct` | `MODE_DU` | Fast monochrome. The normal UI path. Fonts are deliberately 1-bit (`src/fonts/README.md`) |
| `FastGray16` | `MODE_GL16` | Grayscale without a full flash |
| `Gray16` | `MODE_GC16` | Full grayscale clear. Slow and flashes |

- Use partial updates (`meshink_display_update_area`) for changed regions.
- Ghosting builds up with DU. The UI decides when a full GC16 is needed. Keep that
  policy in shared UI code, not in the board backend.

### Geometry and rotation

- Physical framebuffer: 960×540 at 4 bpp packed. Two pixels per byte: even
  physical X in the **low** nibble, odd X in the **high** nibble. That's
  259,200 bytes per buffer. EPDiy allocates front, back and difference buffers
  plus dirty tracking, which is a lot of PSRAM.
- **The panel is mounted rotated.** Application portrait is
  `EPD_ROT_INVERTED_PORTRAIT`. Application landscape is `EPD_ROT_LANDSCAPE`
  (`meshink_display_set_orientation`).
- **Inverted portrait:** logical (x,y) maps to physical (y, 539−x).
- **Fast paths:** `meshink_display_fill_rect` writes black and white directly into
  the packed framebuffer (`meshink_t5_packed::fill_logical_gray4`).
  `meshink_display_blit_gray4_dithered` is the Maps compositor. Grayscale falls back
  to EPDiy calls.
- Both fast paths are host-tested by `tests/test_t5_packed_framebuffer.cpp` and
  `tests/test_map_gray.cpp`. Change the packing only together with those tests.

### Why the cache64 build exists

`sdkconfig.cache64` sets `CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y`. Otherwise the LCD/DMA
path from PSRAM gets throttled and the pixel clock would be reduced automatically.
CI greps for this setting. The release env is `t5-unified-cache64`.

### Display vs flash arbitration (`hardware/io_coordinator.h`)

- An EPD refresh streams the framebuffer from PSRAM over LCD DMA. SPI-flash writes
  (NVS, SPIFFS) suspend the cache. MeshInk therefore never runs the two at once.
  The cache-suspension explanation is the ESP32-S3 platform rationale; the code
  header only says "flash/display arbitration".
- `meshink_display_poweron()` → `meshink_io_display_begin()`. It waits for any flash
  lease, and blocks new ones while it waits.
- Flash writers take `meshink_io_flash_try_begin()` or `meshink_io_flash_begin_wait()`
  and release with `MeshInkFlashGuard`. Leases are reentrant per task, for nested
  SafeFile saves inside a NodeDB save.
- Meshtastic config saves that hit a busy display get deferred in RAM
  (`meshink_io_defer_meshtastic_save`) and flushed when the EPD is idle.
- **Rule:** any new code that writes flash in a task that can run during UI
  refresh must take the flash lease.

### Display teardown for companion mode

Companion (BLE) mode releases EPDiy (`epd_deinit`) before `ESP32Board::begin()` takes
I2C. EPDiy's patched deinit removes only its own ISR handler and keeps the shared
service if the radio handler is alive.

## Touch — GT911

| Item | Value |
| --- | --- |
| Address | 0x5D, selected by the reset sequence: RST LOW and INT LOW, wait 10 ms, RST HIGH, wait 60 ms, INT becomes an input |
| Registers | product ID 0x8140, config version 0x8047, status 0x814E, first point 0x814F (8 bytes per point) |
| Status byte | bit 7 = frame ready, bit 4 = home key, bits 3:0 = point count. Write 0 to the status register after every frame |
| Power off | `meshink_touch_set_power(false)` holds RST LOW. Power-on reruns the address sequence |
| Coordinates | Raw portrait 540-wide. Landscape transform: x = raw_y, y = 539 − raw_x (`logical_point`) |
| I2C errors | Rate-limited log every 2 s: `[T5-TOUCH] ERROR GT911 read reg=0x…` |
| Diagnostics | `-DMESHINK_TOUCH_DIAGNOSTICS=1` prints product ID and config at boot. Also used by the host test |

Semantics the UI depends on (keep them, they're host-tested by `tests/test_t5_touch_backend.cpp`):

- **Single-contact reader:** keeps the last pressed point until the controller
  reports release. On a transient point-read failure it returns the cached
  position, never (0,0). A (0,0) would produce false swipes or wrong keys.
- **Contacts reader:** no new frame means repeat the previous frame. More than 2
  contacts is reported as `count=3`, so Maps suppresses gestures until every
  finger lifts.
- Changing orientation resets tracking.
- Gesture interpretation, keyboard stabilisation and debouncing live **outside** the
  backend (`map_gestures.h`, `keyboard_geometry.h`, UI).

**Touch sampling is independent of e-paper refresh.** Never block touch reads on a
display update.

### GT6972P caveat

LILYGO's repo includes `examples/touch_gt6972p*` (Goodix GT6972P with active pen, same
pins, probing 0x5D and 0x14, GPL-2.0 driver). MeshInk has no GT6972P support. If a
user's touch is dead with continuous GT911 read errors, ask for a
`MESHINK_TOUCH_DIAGNOSTICS=1` build log before assuming a software bug. Adding a
second controller means a new touch backend behind `hardware/touch.h`
(`MESHINK_TOUCH_BACKEND_HEADER`). Check the GPL-2.0 licence of LILYGO's driver
before reusing any of it.
