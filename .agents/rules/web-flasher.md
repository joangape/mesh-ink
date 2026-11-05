---
trigger: glob
globs: "web/**, stats-worker/**, tools/check_web_flasher.py, tools/patch_web_serial_polyfill.py, .github/workflows/web-flasher-pages.yml"
description: "Web flasher (GitHub Pages) and optional stats worker safety and privacy rules."
---

# Web flasher and stats worker

- The flasher writes real devices. Keep the safety checks: update image at `0x10000`, full-wipe image (exactly 16 MiB) at `0x0`, SHA-256 verification before writing, and explicit confirmation for wipes. Run `python3 tools/check_web_flasher.py` after any change.
- Plain static HTML/CSS/JS with no build step. Don't add frameworks or bundlers.
- Privacy: stats events carry no node IDs, serial output, firmware contents, filenames or persistent browser identifiers.
- URLs still target upstream `samo-nz/mesh-ink` (releases API, footer links). Repoint them only on explicit request.
