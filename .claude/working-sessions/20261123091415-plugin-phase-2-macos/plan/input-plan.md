# Planner input — delegation envelope

## Task (verbatim)

> Start phase 2

Orchestrator scope: `PLAN.md` **Phase 2 — Native plugin, macOS**: *Tasks after the spike* items
1–7, plus the Phase 2 follow-ups recorded in *Spike results* and `docs/concerns.md`. See
`<root>/inputs/requirements.md`.

## Session

- Session root: `.claude/working-sessions/20261123091415-plugin-phase-2-macos/`
- Step directory: `<root>/plan/`
- Branch: `feature/plugin-phase-2-macos` (off master after PR #1).
- Log contract: one line appended to `<root>/session.log` before the first read —
  `printf '%s  planner  %s\n' "$(date -u +%FT%TZ)" "<message>" >> <root>/session.log` (done).

## Read paths

- `<root>/inputs/requirements.md`
- `PLAN.md` (Phase 2: spike answers, *Spike results*, *Tasks after the spike*, *Done when*, *Risks*,
  *Upstream asks*)
- `docs/README.md`, then `docs/architecture.md`, `docs/structure.md`, `docs/integrations.md`,
  `docs/concerns.md`, `docs/conventions.md`, `docs/testing.md`, `docs/decisions.md`
- `.claude/COMMON-ISSUES.md` → `common-issues/plugin-build.md`, `common-issues/plugin-runtime.md`,
  `common-issues/verification-tooling.md`
- `.claude/how-x-works/plugin-render-frame.md`
- Previous run: `.claude/working-sessions/20261123004446-browser-source-phase-1/plan/output-plan-gate-1-spike-decisions.md`
  (how OBS testing is driven) and its `output-plan-tasks.md` (table shape). SPK handoffs were not
  needed.
- CodeGraph (Gate 0, reachable): `plugin/src/visualizer-source.cpp`, `plugin/src/plugin-main.cpp`,
  core `include/synesthesia-vibe/synesthesia-vibe.h`, `src/synesthesia-vibe/{preset,context,audio}.cpp`,
  `src/audio/audiobuf.cpp`.
- libobs 31.1.1 source in `plugin/.deps/obs-studio-31.1.1/` for API facts (`obs.h`,
  `obs-source.h`, `obs-hotkey.h`, `obs-properties.h`, `media-io/audio-io.{h,c}`,
  `media-io/audio-resampler.h`, `util/task.h`, `obs-source.c`), and the sibling plugins
  `plugins/obs-filters/compressor-filter.c` (sidechain audio source pattern) and
  `plugins/image-source/obs-slideshow.c` (folder, random, hotkey pattern), plus
  `frontend/widgets/OBSBasic.cpp` (macOS module search path).
- `plugin/CMakeLists.txt`, `plugin/CMakePresets.json`, `plugin/buildspec.json`,
  `plugin/exported-symbols.txt`, `plugin/data/locale/en-US.ini`,
  `plugin/cmake/macos/{helpers.cmake,xcode.cmake,defaults.cmake}`,
  `plugin/cmake/macos/resources/{create-package.cmake.in,distribution.in}`.
- Environment probes (read-only): `security find-identity -p codesigning -v`,
  `xcrun notarytool --version`, `codesign -d --entitlements - /Applications/OBS.app`.

## Write paths

- `<root>/plan/input-plan.md` (this file)
- `<root>/plan/output-plan-summary.md`
- `<root>/plan/output-plan-tasks.md`
- `<root>/plan/output-plan-test-cases.md`
- `<root>/plan/output-plan-concerns.md`

No gate file was present in `<root>/plan/`; none was created.

## Skills (using)

- None beyond the planner role file (the packet named none).

## Rules (read)

- Root `CLAUDE.md` (Rules 1–2, code intelligence, editing source files: no comments, no unit
  tests unless asked, sibling style, file header, naming, public-repo hygiene).
- `PLAN.md`.
- Standing constraints from `inputs/requirements.md`: OBS driven only through obs-websocket (no GUI
  or accessibility automation; stop and ask if it is unavailable); OBS config files never edited;
  the user's profile, canvas and fps never changed without asking; tests in a separate scene
  collection; no secrets, credential paths or machine paths in repo files.

## Context supplied by the orchestrator (not re-derived)

- OBS 33.0.3 (OpenGL renderer) on this Mac; the user's profile is 1080×1920 portrait at 30 fps.
- obs-websocket on localhost, authentication currently off (the user may re-enable it).
- The plugin is not installed in OBS right now.
- macOS microphone permission for OBS is off.
- Toolchain: Xcode 27.2, CMake 4.4.5, Ninja, vcpkg at `$VCPKG_ROOT`.
