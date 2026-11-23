# Gate 1 decisions (user, 2026-11-23)

The orchestrator asked these in-thread. These are the user's answers verbatim.

| Question | Answer |
|---|---|
| Scope | **Phase 1 + Phase 2 spike.** Build the Browser Source page, then run the 2-day macOS plugin spike in this run. |
| C-001 / C-002 presets and Pages | **Skip Pages for now.** Drop BS-006 (Pages workflow). Serve locally only; D5 stays open. Presets still come from the core's source tarball into the git-ignored `dist/resources/`, for local serving. |
| C-003 verification | **Agent check + user checklist.** The agent captures Chrome screenshots (fake audio device) and tries OBS on this Mac with the built-in mic. The user runs the BlackHole / VB-Cable checklist. Phase 1 is done only after that. |
| C-004 … C-020 | **Accept all recommended defaults** in `output-plan-concerns.md`. C-011 and C-012 fall away with BS-006. |
| D1 license | **GPL-2.0-or-later, plus a LICENSE file in this run.** Lock D1 in `PLAN.md`. Add the GPL-2.0 LICENSE text at the repo root. Add `docs/decisions.md`. The MIT core keeps its own license. |

## What this means for the plan

- **Remove:** BS-006.
- **Add:** a D1 task covering `LICENSE`, the D1 row in `PLAN.md`, and `docs/decisions.md`. The user approved these `PLAN.md` / docs edits, which covers C-013 for D1.
- **Add:** Phase 2 spike tasks: submodule, core-as-subdirectory build, rendering inside `video_render`, a GL-state audit, GL loader isolation, and the renderer check. Each spike answer is written back into `PLAN.md`.
- Spike concerns go back to the user at a second Gate 1 before any spike code is written.

## Mid-develop decision (user, 2026-11-23, raised by BS-004)

| Question | Answer |
|---|---|
| Meaning of `cycle=` | **Keep the engine's meaning.** `cycle` is how long each preset plays alone once its blend has finished, so presets change every `cycle + blend` seconds. This matches the core's web sample. The BS-007 README must explain it, for example: `cycle=30&blend=2` gives a change about every 32 s. |

## Pre-docs approvals (user, 2026-11-23, C-013)

The user approved all three for the docs pass:
- the `CLAUDE.md` routing table plus the *Setup* and "nothing builds yet" wording;
- the `PLAN.md` line-3 status;
- adding `.vscode/` to `.gitignore`.
