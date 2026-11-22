# Example memory entry (demo)

> Part of the repo memory — see [`../MEMORY.md`](../MEMORY.md) for the index. This is a **demo
> entry** showing the format; delete it once you have at least one real entry.

**Fact:** `<one durable, reusable fact about this repo — e.g. "configure the build from /tmp; the
build tool hangs when its cwd is the repo root">`.

**Why:** `<what happened that made this worth writing down — the incident or constraint>`.

**How to apply:** `<the concrete command, setting, or rule a future session should follow>`.

**Analysis date:** `<yyyy-MM-dd>` — re-verify before trusting an old entry.

## Format notes

- One fact per file, kebab-case filename (`build-from-tmp.md`), one index line in `MEMORY.md`.
- Memory is for environment quirks, tooling workarounds and decisions — not for bugs (those go in
  `common-issues/`) and not for architecture (that goes in `docs/`).
- Update an entry in place when it changes; delete it when it stops being true.
