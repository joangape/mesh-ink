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
