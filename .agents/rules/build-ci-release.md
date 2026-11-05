---
trigger: glob
globs: "platformio.ini, tools/**, .github/**, CMakeLists.txt, src/CMakeLists.txt"
description: "PlatformIO config, build tools, CI workflows and release safety."
---

# Build, CI and release

- `platformio.ini` env graph: `t5-pro` (base) → `t5-unified` → `t5-unified-cache64` (release) → `-warnings`, `-nogps-test`, `-ram-audit`. Sources are opt-in through `build_src_filter`, so **a new `.cpp` must be added to `build_src_filter`** for every env that needs it.
- Project sources must compile warning-free under `-Wall -Wextra` (`t5-unified-cache64-warnings` + `tools/check_build_warnings.py`).
- Pre-build scripts in `tools/` must stay idempotent and fail closed.
- A new host test must be added to **both** `.github/workflows/testing-firmware.yml` and `t5-ui-binary.yml` where applicable, and to the test table in `CLAUDE.md` / `AGENTS.md`.
- Release triggers: a `v*` tag, or a `main` commit message containing `[publish-release]`, creates a public GitHub release. Never use either without explicit instruction. Version strings must agree (`T5_FIRMWARE_VERSION`, `T5_UI_VERSION`, README title, release notes).
- Workflow branch names (`mesh-agnostic-refine`, `testing`, `feature/...`) are upstream's. Ask before repurposing them on this fork.
