---
description: Never modify pinned upstream MeshCore/Meshtastic submodules; adapt via MeshInk code or build scripts.
paths:
  - "lib/**"
  - "include/meshtastic_compat/**"
  - "tools/prepare_meshtastic_library.py"
  - "tools/official_meshtastic_library.json"
  - "tools/select_crypto_provider.py"
  - ".gitmodules"
---

# Upstream submodules (lib/MeshCore, lib/Meshtastic)

- Never edit, patch, or commit changes inside `lib/MeshCore` or `lib/Meshtastic`. Never change their pinned commit unless the user explicitly asks for an upstream bump.
- Fix incompatibilities in MeshInk code: adapters in `src/protocol/`, `src/meshcore_adapter.h`, the headless shims in `include/meshtastic_compat/`, or **idempotent, fail-closed** PlatformIO pre-scripts in `tools/` that change only the temporary build checkout.
- Meshtastic sources are compiled through a curated whitelist in `tools/official_meshtastic_library.json`, written by `tools/prepare_meshtastic_library.py`. Add upstream sources there only if they're truly needed. Upstream display, BLE, WiFi, power-FSM, GPS and main loop are deliberately excluded.
- `include/meshtastic_compat/variant.h` must stay headless and pin-free.
- Crypto provider is chosen only via `[meshink-crypto]` in `platformio.ini`.
- When bumping a pin (only on request): update `include/meshcore_version.h` / `include/meshtastic_official_version.h` and any `APP_VERSION` flag, then run `tools/check_meshcore_pin.py`, `tools/check_official_meshtastic_pin.py` and `tools/check_release_integration.py`. Expect a hardware retest.
