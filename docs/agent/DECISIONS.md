# Decision log

Append-only. To change a decision, add a new entry that supersedes the old one, and
mark the old one `Superseded by D-###`.

## Template

```markdown
### D-000 — Title (YYYY-MM-DD)
- **Status:** accepted
- **Context:** what forced a choice
- **Decision:** what we chose
- **Consequences:** trade-offs, follow-ups
```

## Entries

### D-001 — Separate agent instruction files per tool (2026-11-05)

- **Status:** accepted
- **Context:** Claude Code reads `CLAUDE.md`. Antigravity reads `AGENTS.md`/`GEMINI.md` and `.agents/rules/`.
- **Decision:** Keep complete, independent `CLAUDE.md` and `AGENTS.md`, plus parallel rule sets in
  `.claude/rules/` (`paths:` frontmatter) and `.agents/rules/` (`trigger: glob`). Shared state
  lives in `docs/agent/`.
- **Consequences:** Project facts must be updated in both files. Skills are duplicated in
  `.claude/skills/` and `.agents/skills/`.

### D-002 — Keep upstream invariants in the fork (2026-11-05)

- **Status:** accepted
- **Context:** Fork of `samo-nz/mesh-ink`, which enforces never-patched upstream cores and
  protocol/board boundaries through CI gates.
- **Decision:** New features follow the same invariants so upstream changes can still merge
  in and features stay upstreamable.
- **Consequences:** Protocol-specific features go through capability flags in the facade.

### D-003 — Boot-Mode Isolation for WiFi and Meteo Service (2026-11-12)

- **Status:** accepted
- **Context:** Meshtastic and MeshCore require large amounts of internal RAM (DRAM), hardware timers,
  and exclusive radio usage. Running WiFi concurrently with active mesh engines risks DRAM exhaustion,
  stack overflows, and watchdog resets.
- **Decision:** WiFi operates as an isolated boot mode (`wifi_mode` persisted in NVS `"t5-ui"`). Switching
  between Mesh and WiFi mode requests user confirmation and reboots the ESP32-S3. In WiFi mode, LoRa
  and protocol core runtimes are completely skipped during startup. Open-Meteo API is fetched over plain HTTP
  (avoiding mbedTLS stack allocation in DRAM).
- **Consequences:** Mesh operations (LoRa, BLE companion, packet routing) do not run while in WiFi mode.
  The bottom navigation bar transitions cleanly to 3 tabs (`METEO`, `MAPS`, `MORE`), preserving offline
  mapping and general settings.
