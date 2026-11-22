# Example area — build & environment (demo entry)

> Part of the known-issues corpus — see [`../COMMON-ISSUES.md`](../COMMON-ISSUES.md) for the
> index. This is a **demo entry** showing the format; replace it with real, project-specific
> gotchas and delete this file once you have at least one.

- **Symptom:** a fresh clone fails to build/install dependencies with `<example error text>`.
- **Root cause:** `<why this happens — e.g. "a config file is gitignored and supplied
  out-of-band" or "a lockfile isn't committed, so dependency resolution isn't reproducible across
  machines">`.
- **Fix / workaround:** `<the concrete steps that resolve it>`.
- **How to verify this is still current:** `<e.g. "check whether the lockfile is now committed"
  or "re-run a clean clone + install and see if the error still reproduces">` — this is dated
  analysis, not live monitoring, so a stale entry should be corrected or removed once it no longer
  reflects reality.

## Format notes

- One file per **area** (build/environment, a specific subsystem, testing, deployment, …), not
  one file per issue — group related gotchas so a reader scanning one area sees all of them.
- State the symptom before the cause — a reader arrives with a symptom, not a diagnosis.
- Keep entries short; link to `docs/concerns.md` or a `.claude/how-x-works/**` page
  instead of re-explaining architecture inline.
- Distinguish a **bug to fix** from a **known, accepted trade-off** — the latter deserves a note
  ("intentionally deferred, see `<ticket>`") so nobody re-"fixes" it by surprise.
