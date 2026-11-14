# Feature backlog

Status: `idea` → `planned` → `in-progress` → `done` (or `dropped`).

Each item should state which protocols it affects (MeshCore / Meshtastic / both / neither),
whether it needs a new protocol capability, and whether it needs hardware verification.

## Template

```markdown
### F-000 — Title
- **Status:** idea
- **Protocols:** both
- **Why:** user-facing motivation
- **Acceptance criteria:**
  - [ ] observable behaviour 1
  - [ ] host test / gate added or updated
  - [ ] hardware check recorded as HW-###
- **Notes / links:** files, decisions (D-###)
```

## Items

### F-001 — WiFi Mode and Weather Station Screen (Meteo)
- **Status:** in-progress
- **Protocols:** neither (dedicated WiFi boot mode)
- **Why:** Allow user to connect device to WiFi and check weather forecast for their last known GPS coordinates without running LoRa radio or exhausting DRAM.
- **Acceptance criteria:**
  - [x] More > Settings > WiFi row toggles WiFi mode (with reboot confirmation prompt)
  - [x] WiFi Settings screen for SSID and Password entry via on-screen keyboard
  - [x] In WiFi mode, 3-tab bottom navigation (`METEO`, `MAPS`, `MORE`) with 180px tabs
  - [x] Meteo screen retrieves Open-Meteo data for last known GPS coordinates (or Berlin fallback)
  - [x] Architecture and UI regression gates pass (`check_architecture.py`, `check_ui_regressions.py`)
  - [x] Builds cleanly on release target `t5-unified-cache64`
  - [ ] Hardware verification HW-003 performed on physical device
- **Notes / links:** `src/board/t5_wireless_backend.h`, `src/hardware/wireless_types.h`, `src/unified_main.cpp`, `src/ui_onboarding.cpp`, decision D-003.
