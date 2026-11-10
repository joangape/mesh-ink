# Agent state artifacts

Working memory shared by Claude Code (`CLAUDE.md`) and Antigravity (`AGENTS.md`)
across sessions. Plain Markdown, committed with the code. Humans can edit these too.

| File | Purpose | Update rule |
| --- | --- | --- |
| [PROGRESS.md](PROGRESS.md) | Session handoff: current focus, recent work, next steps, blockers | Rewrite "Current focus" and "Next steps" at the end of every non-trivial session. Prepend to "Log" |
| [BACKLOG.md](BACKLOG.md) | Planned features with acceptance criteria | Add items when the user requests features. Move status `idea → planned → in-progress → done` |
| [DECISIONS.md](DECISIONS.md) | Append-only design decisions for this fork | Add an entry when a non-obvious choice is made. Never rewrite old entries; supersede them |
| [HARDWARE_TESTS.md](HARDWARE_TESTS.md) | What has been verified on a real T5 vs. only host/CI | Add a row for every change needing on-device confirmation. The user fills in results |

## Conventions

- Dates are absolute (`YYYY-MM-DD`). IDs: backlog `F-###`, decisions `D-###`, hardware tests `HW-###`.
- Cross-reference IDs (e.g. "implements F-002, see D-003, needs HW-004").
- Keep entries short. Link to commits, files or docs rather than duplicating them.
- Never claim hardware verification that the user hasn't reported.

## Installed skills (provenance)

Installed identically in `.claude/skills/` (Claude Code) and `.agents/skills/`
(Antigravity), Third-party skills are vendored without modification and are MIT licensed (LICENSE copied into each).
`lilygo-t5-epaper-s3-pro` is written for this project: update it when board code changes.

| Skill | Source | Commit |
| --- | --- | --- |
| `lilygo-t5-epaper-s3-pro` (+ `references/`, `scripts/pin_audit.py`) | **Project-authored** from MeshInk `src/board/` + [LILYGO T5S3-4.7-e-paper-PRO](https://github.com/Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO) `docs/pinmap.md` | LILYGO `87436b5`; checked 2026-11-10 |
| `esp32` (+ `references/`, `scripts/`) | [ezrover/esp32-ai-agent-skill](https://github.com/ezrover/esp32-ai-agent-skill) `skills/esp32` | `124d298e4ccaa737dbf6d26bbae47722e17f3097` |
| `systematic-debugging` | [obra/superpowers](https://github.com/obra/superpowers) `skills/systematic-debugging` | `22f0b463e7a5e579904dd6211a520321035526f7` |
| `test-driven-development` | obra/superpowers `skills/test-driven-development` | same |
| `verification-before-completion` | obra/superpowers `skills/verification-before-completion` | same |

To update, re-clone the source at a new commit, review the diff, copy into **both**
skill directories, and update the commit column.
