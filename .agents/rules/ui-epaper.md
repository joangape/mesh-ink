---
trigger: glob
globs: "src/ui_*, src/keyboard_*, src/map_*, src/utf8_display.h, src/message_limits.h, src/fonts/**, src/*_logo.h, tests/test_ui_layout.cpp, tests/test_map_*.cpp, tests/test_utf8_display.cpp, tools/test_keyboard_geometry.cpp, tools/check_ui_regressions.py, tools/check_compose_utf8_wrap.py"
description: "E-paper UI, keyboard, touch and map rendering conventions and their regression gates."
---

# E-paper UI

- Target is 540×960 monochrome/grayscale e-paper, portrait and landscape. Refreshes are slow: coalesce redraws, refresh only changed regions, and give explicit press feedback.
- Tap targets: drawing geometry and hit-testing must come from the same helpers (`ui_layout.h`, `keyboard_geometry.h`, `map_gestures.h`). Adjust `test_ui_layout.cpp` / `test_keyboard_geometry.cpp` (built with **-std=c++11**) when geometry changes.
- `tools/check_ui_regressions.py` asserts on literal tokens in `ui_onboarding.cpp`. When it fails, decide whether behaviour actually regressed. Don't just edit the token.
- Text is UTF-8. Wrap, edit, backspace and clip on code-point boundaries using `utf8_display.h` / `keyboard_utf8_input.h`. Only glyphs present in `src/fonts/` render.
- `src/fonts/*.h` and `src/meshink_logo_bitmap.h` are generated (`tools/generate_logo.py` builds the logo). Don't hand-edit generated data.
- List screens use the shared 5-row paging with a `PAGE X OF Y` footer and arrow/swipe navigation.
- `ui_onboarding.cpp` is ~8k lines. Read the relevant section before editing and keep changes localised.
- UI changes always need visual confirmation on the device. Log it in `docs/agent/HARDWARE_TESTS.md`.
