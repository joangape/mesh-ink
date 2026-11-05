# Progress / session handoff

## Current focus

Fork set up for feature development. Agent configuration created (instructions, rules,
skills, state artifacts). No feature work started yet.

## Next steps

1. Initialise submodules: `git submodule update --init --recursive`.
2. Install PlatformIO (`python3 -m pip install platformio Pillow`) and do a first
   `pio run -e t5-unified-cache64` to confirm the toolchain works on this Mac.
3. Re-run the three submodule-dependent gates (`check_release_integration.py`,
   `check_meshcore_pin.py`, `check_official_meshtastic_pin.py`).
4. Capture the first features in `BACKLOG.md`.

## Blockers / open questions

- PlatformIO not yet installed locally. Firmware builds are unverified on this machine.
- Should this fork's web flasher / release links be repointed from `samo-nz` to `joangape`? (Not decided. Links unchanged.)

## Log

### 2026-11-05 — Agent configuration

- Added `CLAUDE.md`, `AGENTS.md`, `.claude/rules/*`, `.agents/rules/*`, vendored skills,
  `docs/agent/*`.
- Baseline: all 20 host C++ tests pass on macOS (Apple clang). Static gates pass except
  the three that need initialised submodules.
