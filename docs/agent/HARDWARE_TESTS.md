# Hardware test log

Host tests and CI builds don't prove behaviour on the device. Every change that affects
radio, display, touch, power/standby, GPS, storage/restore or visible UI gets a row here.
The agent adds the row with status `pending`; the person with the T5 records the result.

Device: LILYGO T5 E-Paper S3 Pro (H752-01/V2 reference).

| ID | Date added | Change / commit | Protocol | What to verify | Status | Result notes |
| --- | --- | --- | --- | --- | --- | --- |
| HW-001 | 2026-11-05 | Baseline: upstream 3.1.0 (`47c4bb3`) built from this fork | both | Flash `update.bin`, boot, send/receive one message on each protocol | pending | |
| HW-002 | 2026-11-10 | MORE > DEVICE INFO page (uncommitted, agent-config branch) | both | MORE shows 7 rows without touching the tab bar; every row taps correctly; DEVICE INFO opens, values look right (battery vs. USB, serial, flash 16 MB, PSRAM 8 MB, GPS, SD), back returns to MORE | pending | Builds OK; not flashed yet |
| HW-003 | 2026-11-12 | WiFi Mode and Meteo Screen (F-001) | neither | Enable WiFi in Settings -> Confirm -> Device reboots into 3-tab layout (METEO, MAPS, MORE). Enter SSID/password in WiFi Settings, connect to AP. Meteo screen fetches and displays temperature, humidity, wind, and GPS coordinates. Turn off WiFi in Settings reboots back to LoRa mesh. | pending | Builds OK on t5-unified-cache64; pending hardware test |
| HW-004 | 2026-11-12 | Pin CPU at 240 MHz in WiFi mode (`set_cpu_target`) | neither | In WiFi mode, tap REFRESH WEATHER 10+ times; device must not hang after `[T5-METEO] parsed OK` (previous build froze at the next `[T5-EPD] begin`) | pending | Builds OK; not flashed. If it still hangs, suspect blocking HTTP in loop / heap |
| HW-005 | 2026-11-12 | Meteo refresh also syncs clock from HTTP `Date` (Auto time mode only) | neither | With Date & Time mode = Auto and no GPS fix, set clock wrong, tap REFRESH WEATHER: status-bar time corrects, log shows `clock synced from HTTP Date`, time mode stays Auto. In Manual mode the clock must not change. | pending | Also fixes `--:--` clock in WiFi mode (RTC now started in WiFi boot path, status bar no longer gated on mesh_is_ready). Date parser host-verified; builds OK; not flashed. Timezone offset still applied from the user's timezone setting |

Status values: `pending`, `pass`, `fail`, `partial`, `n/a`.
