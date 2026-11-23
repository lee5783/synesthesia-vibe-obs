# .claude/COMMON-ISSUES.md — Known Issues Index

Entry point for known/catalogued issues in this repo — **check here before diagnosing a bug or
gotcha from scratch**; it may already be understood, have a documented workaround, or be
intentionally deferred (check ticket status before "fixing" it).

Don't load every area file into context at once — read only the one relevant to what you're
touching, via `Read`/`Grep`, same as any other file.

| Area | Read |
|---|---|
| Plugin build and core submodule (template build-number bug, `git submodule status` tag label) | [`common-issues/plugin-build.md`](common-issues/plugin-build.md) |
| Plugin runtime in OBS (GL state cache desync, `gl_state_guard`) | [`common-issues/plugin-runtime.md`](common-issues/plugin-runtime.md) |
| Browser Source in OBS (macOS mic permission → `NotFoundError`, first-run blank source) | [`common-issues/browser-source.md`](common-issues/browser-source.md) |
| Verification tooling (Chrome fake audio silent, `osascript` quit -128) | [`common-issues/verification-tooling.md`](common-issues/verification-tooling.md) |

Tech debt and planned fixes that are not quick gotchas live in `docs/concerns.md`.

## Adding a new entry

- Put it under the area file it matches, or create a new `common-issues/<area>.md` + add a row
  above if it doesn't fit an existing one.
- State the symptom, the root cause (if known), and how to verify it's still current — this is
  dated analysis, not live monitoring, so a later reader needs to know when to distrust it.

## Why this exists instead of just fixing things as they come up

The same gotcha costs a session real time the first time it's hit — a flaky test, a build step
that only fails in a specific environment, a footgun in a shared utility. Writing it down once,
in one place indexed by area, means the next session (or the next agent) recognizes it in seconds
instead of re-diagnosing it from a stack trace. It also protects against "fixing" something that
was actually a deliberate, tracked trade-off — the entry is the place to say so.
