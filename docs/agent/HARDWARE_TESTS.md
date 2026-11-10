# Hardware test log

Host tests and CI builds don't prove behaviour on the device. Every change that affects
radio, display, touch, power/standby, GPS, storage/restore or visible UI gets a row here.
The agent adds the row with status `pending`; the person with the T5 records the result.

Device: LILYGO T5 E-Paper S3 Pro (H752-01/V2 reference).

| ID | Date added | Change / commit | Protocol | What to verify | Status | Result notes |
| --- | --- | --- | --- | --- | --- | --- |
| HW-001 | 2026-11-05 | Baseline: upstream 3.1.0 (`47c4bb3`) built from this fork | both | Flash `update.bin`, boot, send/receive one message on each protocol | pending | |
| HW-002 | 2026-11-10 | MORE > DEVICE INFO page (uncommitted, agent-config branch) | both | MORE shows 7 rows without touching the tab bar; every row taps correctly; DEVICE INFO opens, values look right (battery vs. USB, serial, flash 16 MB, PSRAM 8 MB, GPS, SD), back returns to MORE | pending | Builds OK; not flashed yet |

Status values: `pending`, `pass`, `fail`, `partial`, `n/a`.
