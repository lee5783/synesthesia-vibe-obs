# Code review input — Gate 2

## Task (verbatim from the orchestrator)

Review all work from this run — PLAN.md Phase 1 Browser Source (BS-001…BS-005, BS-007, BS-008;
BS-006 removed), LIC-001, and the Phase 2 macOS spike (SPK-001…SPK-008) — against the plan and
decisions. Plan rows BS-009 and SPK-009.

## Session

- Root: `.claude/working-sessions/20261123004446-browser-source-phase-1/`
- Step directory: `review/`
- Output: `review/output-review-findings.md`

## Skills (using)

- `shader-programming` (loaded; for the GL state guard / render path review)
- Not loaded, per CLAUDE.md (manual-only): `cpp-coding-standards`, `cpp-testing`

## Rules (read)

- Root `CLAUDE.md` (no code comments, sibling style, file headers, no unit tests unless asked,
  public-repo hygiene)
- `.gitignore` (and its diff)
- `.claude/COMMON-ISSUES.md` (index holds only the example area; nothing catalogued)

## Artifacts to read

- `inputs/requirements.md`
- `plan/output-plan-summary.md`, `output-plan-tasks.md`, `output-plan-test-cases.md`,
  `output-plan-concerns.md`
- `plan/output-plan-gate-1-decisions.md`, `plan/output-plan-gate-1-spike-decisions.md` (incl.
  mid-develop sections)
- `develop/output-{BS-001..005,BS-007,BS-008,LIC-001,SPK-001..008}.md`
- `develop/evidence/**/*.md`
- `session.log`

## Change set (uncommitted on master)

- `git diff`: `PLAN.md`, `.gitignore`
- `git diff --cached`: `.gitmodules`, `external/synesthesia-vibe` gitlink
- Untracked: `LICENSE`, `browser-source/{fetch-dist.sh,index.html,obs-page.js,README.md}`,
  `plugin/**` (excluding git-ignored `plugin/.deps`, `plugin/build_*`; `plugin/cmake/**` and
  `plugin/src/plugin-support.*` checked for byte-identity against obs-plugintemplate `3e7d7ac3`),
  the session dir (artifacts only)
- Ignored for review: `.vscode/settings.json` (not part of this run; do-not-commit note only)

## Focus areas

- `fetch-dist.sh`: atomic swap/trap, extraction safety, checksum logic, portability.
- `obs-page.js` / `index.html`: settings, preset matching, playlist failure, audio device
  selection, deferred start, unplug, error reporting.
- `plugin/`: threading, `gl_state_guard` vs the SPK-006 table, flip/sRGB/blend, dt, lifetime,
  renderer check return value, exported symbols, ExternalProject/CMake, build-number guard.
- S-003 instrumentation leftovers (10 s LOG_INFO line, synthetic tone, `preset_path` without UI).
- `PLAN.md` accuracy; D4 "Locked" set by the developer (note only); links into
  `.claude/working-sessions`; stale status line (BS-010 owns it).
- Public-repo hygiene across every file to be committed, incl. session artifacts (websocket
  password, credentials-file path, scratchpad paths, `/Users/` paths, device names).
- Evidence sufficiency per Gate 1.5 (Phase 1 audio-in-OBS pending the user checklist by decision).

## Output

Findings by severity (blocker / should-fix / nit) with `file:line`, and the must-fix list before
docs. No fixes.

## Round 2 — fix verification (2026-11-23)

- Task (verbatim): verify the developer's fix pass against your own findings, and report whether
  Gate 2 is now satisfied.
- Read: `review/output-review-findings.md` (round 1, line 52 redacted by the orchestrator),
  `develop/output-REVIEW-FIX.md`, `develop/evidence/REVIEW-FIX/review-fix-report.md`,
  `develop/output-SPK-006.md`, current `plugin/src/visualizer-source.cpp`,
  `plugin/src/plugin-main.cpp`, `plugin/CMakeLists.txt`, `PLAN.md`.
- Skills (using): `shader-programming`. Rules (read): root `CLAUDE.md`.
- Checks: B1, B2, S4, S1 (PLAN wording incl. task 4 and Risks edits), S2, N3/N4 null guards, the N1
  active-unit change in `gl_state_guard`, no new comments, sibling style, the PLAN "unit 7 → 0"
  correction.
- Hygiene edit owned by the reviewer: neutralise the local folder-name mentions in the round-1
  findings (old lines 34, 53, 252, 255, 259).
- Output: a "Round 2" section appended to `review/output-review-findings.md`. No code fixes. An OBS
  runtime re-check runs in parallel and is not awaited.
