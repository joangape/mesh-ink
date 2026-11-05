---
trigger: glob
globs: "src/protocol/**, src/ui_onboarding.cpp, src/ui_onboarding.h, src/unified_main.cpp, src/ui_data.h, src/local_mesh_runtime.*, src/companion_runtime.*, src/meshcore_adapter.h"
description: "Protocol-neutral facade rules for shared UI/boot code and protocol helpers."
---

# Protocol boundary

- Shared code (`ui_onboarding.*`, `unified_main.cpp`, `ui_data.h`) talks to networks only through `protocol/mesh_protocol.h` and `UiDataProvider`. It must not mention `MeshCore`, `MESHCORE`, `local_mesh_`, `meshcore_adapter.h`, `companion_runtime.h` or Meshtastic internals (except the existing logo sprite symbols).
- `mesh_protocol.{h,cpp}` and `mesh_protocol_backend.h` stay backend-agnostic.
- A new feature that needs protocol support should:
  1. add a generic facade function or capability flag in `mesh_protocol.h` / `mesh_protocol_backend.h`;
  2. implement it in **both** helpers (`meshcore_protocol.cpp`, `meshtastic_protocol.cpp`) or advertise the capability as unsupported;
  3. gate the UI on `mesh_protocol_has(capability)`, never on protocol identity.
- Helper-specific settings appear as helper-owned rows in Protocol Settings, which reuses the 5-row paging pattern.
- Shared services stay MeshInk-owned and out of the backend contract: device name, message journal, unread totals, GPS receiver, RTC/time.
- MeshCore companion-example includes are allowed only in `companion_runtime.cpp` and `meshcore_adapter.h`.
- Slots: MeshCore = slot 1 (`meshink_protocol_backend_slot_1`), Meshtastic = slot 2. Switching protocol means persisting the ID and rebooting.
- Run `python3 tools/check_architecture.py` after every change here.
