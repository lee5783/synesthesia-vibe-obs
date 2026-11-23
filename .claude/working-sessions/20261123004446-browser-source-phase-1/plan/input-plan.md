# Planner input — delegation envelope

## Task (verbatim)

> work on this @PLAN.md

Orchestrator scope: `PLAN.md` **Phase 1 — Browser Source** (items 1–6 and its *Done when*), per
`inputs/requirements.md`. The user confirms scope at Gate 1.

## Session

- Session root: `.claude/working-sessions/20261123004446-browser-source-phase-1/`
- Step directory: `<root>/plan/`
- Log contract: one line appended to `<root>/session.log` before the first read —
  `printf '%s  planner  %s\n' "$(date -u +%FT%TZ)" "<message>" >> <root>/session.log` (done).

## Read paths

- `<root>/inputs/requirements.md`
- `PLAN.md`
- root `CLAUDE.md`
- `.claude/harness/HARNESS.md`
- `.claude/harness/developer.md` (evidence rules)
- `.gitignore`

## Write paths

- `<root>/plan/input-plan.md` (this file)
- `<root>/plan/output-plan-summary.md`
- `<root>/plan/output-plan-tasks.md`
- `<root>/plan/output-plan-test-cases.md`
- `<root>/plan/output-plan-concerns.md`

## Skills (using)

- None beyond the planner role file (packet: "Skills to load beyond your own instructions: none").

## Rules (read)

- root `CLAUDE.md` (non-negotiable rules, editing rules, file header, naming, no comments, no unit
  tests, public-repo hygiene)
- `PLAN.md` (Phase 1, D1, D5, risks, evidence and testing)
- `.claude/harness/HARNESS.md` (roles, artifact contract)
- `.claude/harness/developer.md` (evidence requirements, screenshot rules)
- `.gitignore` (`browser-source/dist/` and bulk evidence are ignored)

## Research performed

- Code intelligence (Rule 2 step 1): CodeGraph is configured but the index is empty (no source in
  the repo) — orchestrator established this at Gate 0; nothing to query. `docs/` has only
  `README.md`. `.claude/COMMON-ISSUES.md` not relevant (no code yet).
- Downloaded `synesthesia-vibe-web-0.7.0.zip` + `SHA256SUMS.txt` from release `v0.7.0` into the
  session scratchpad, verified the sha256 (`e9046312…5bc6`, OK), unzipped and read every JS
  wrapper as data. Nothing from the zip was executed.
- Read the core repo at tag `v0.7.0` through `gh api`: `README.md` (web section),
  `samples/web/main.js` (reference integration), `.github/scripts/build-web-sample.sh`
  (manifest format, preset hosting note), `NOTICE` (preset and Emscripten licences),
  `resources/` listing.
- Probed the local environment: OBS 33.0.3 and Google Chrome are installed; no loopback audio
  device (BlackHole etc.) is present; `actionlint`, `python3`, `shasum`, `sha256sum`, `unzip`,
  `curl` are available. This repo is public, default branch `master`, GitHub Pages **not**
  enabled (API 404).

## Revision 2 — after Gate 1 (2026-11-23)

**Trigger.** Coordinator message: revise the plan per `<root>/plan/output-plan-gate-1-decisions.md`.
The session log line was written before reading it.

**Additional reads**

- `<root>/plan/output-plan-gate-1-decisions.md`
- `docs/README.md`
- Core at tag `v0.7.0` via `gh api`, treated as data:
  - `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`, `vcpkg-configuration.json`
  - `src/synesthesia-vibe/CMakeLists.txt`
  - `include/synesthesia-vibe/synesthesia-vibe.h`
  - the tag's source tarball, unpacked into the scratchpad, with a grep census of GL calls in
    `src/render/gl` and `src/synesthesia-vibe`. Nothing was executed.
- `obsproject/obs-plugintemplate@master` via `gh api`: tree, `buildspec.json`,
  `CMakePresets.json`, `CMakeLists.txt`, `cmake/common/buildspec_common.cmake`.
- Local environment probes:
  - toolchain: Xcode, CMake, Ninja, clang, vcpkg
  - OBS: the `OBS.app` frameworks and entitlements, `global.ini` renderer, the latest OBS log

**Changes**

- Summary: scope is now Phase 1 + D1 + Phase 2 spike. Added *Phase 2 spike facts*; revised the
  risks and impact.
- Tasks:
  - BS-006 removed.
  - BS-007 changed to local serving only.
  - LIC-001 (developer) and LIC-002 (document-writer; `docs/**` is reserved for that role) added.
  - SPK-001…SPK-010 added, blocked on the second Gate 1.
  - BS-001's definition was left unchanged because it is in progress.
- Test cases: Pages rows removed, defaults made concrete, TC-029 reused for `audio=none`, and
  TC-036…TC-050 added (D1 + spike).
- Concerns:
  - C-001…C-020 resolved or moot per the decisions file.
  - New: C-021 and the "Second Gate 1 — spike" section (S-001…S-014), each with a recommended
    default.

**Skills (using):** none beyond the planner role file.
**Rules (read):** unchanged, plus the Gate 1 decisions file.
