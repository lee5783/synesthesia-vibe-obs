# Planner input: delegation envelope

## Task (verbatim)

> start phase 4 on top of this

Orchestrator scope: `PLAN.md`, **Phase 4 — Release and Linux (medium)**, items 1–4. Work starts
from the Phase 2 branch (`feature/plugin-phase-2-macos`, commit `a7b62ed`, PR #2 still open) on
`feature/phase-4-release-linux`. Full scope, known context and standing constraints are in
`<root>/inputs/requirements.md`.

## Session

- Session root: `.claude/working-sessions/20261123145422-phase-4-release-linux/`
- Step directory: `<root>/plan/`
- Branch: `feature/phase-4-release-linux`, stacked on the unmerged `feature/plugin-phase-2-macos`.
- Log contract: append one line to `<root>/session.log` before the first read, with
  `printf '%s  planner  %s\n' "$(date -u +%FT%TZ)" "<message>" >> <root>/session.log`.
  Done at `2026-11-23T07:55:10Z`.

## Read paths (as delegated)

- `<root>/inputs/requirements.md`
- `PLAN.md`: Decisions, Repository layout, Phase 2 (spike answers, *Spike results*, *Tasks after
  the spike* item 7, *Done when*, *Done-when state*), Phase 3, Phase 4, Risks, Upstream asks
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/plan/output-plan-gate-1-decisions.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/output-pull-request.md` (PR #2 body)
- `docs/integrations.md`, `docs/stack.md`, `docs/structure.md`, `docs/concerns.md`,
  `docs/decisions.md`, `docs/README.md`
- `.claude/common-issues/plugin-build.md`, `plugin-runtime.md`, `verification-tooling.md`
- `plugin/CMakeLists.txt`, `plugin/CMakePresets.json`, `plugin/buildspec.json`,
  `plugin/exported-symbols.txt`, `plugin/packaging/macos/distribution.in` (listed only),
  `plugin/cmake/linux/{compilerconfig,defaults,helpers}.cmake`,
  `plugin/cmake/common/{osconfig,buildnumber,bootstrap}.cmake` (relevant lines),
  `plugin/cmake/macos/*` (listed; behaviour already documented in `docs/integrations.md`)

## Extra sources consulted (and why)

- **CodeGraph** (MCP `codegraph_explore`, reachable): `opengl_get_proc`, `gl_state_guard`,
  `with_core_gl_state`, `visualizer_create`, `renderer_is_opengl` / `obs_module_load`,
  `sv_init_gl`. Used to see what the macOS GL path hard-codes.
- `plugin/src/visualizer-source.cpp` lines 1–40, plus a grep for platform conditionals. The
  include block (`<OpenGL/gl3.h>`) and `opengl_framework_path` are outside the symbols CodeGraph
  returned.
- Core submodule, read only, to judge whether a Linux build is feasible:
  `external/synesthesia-vibe/CMakeLists.txt` (flag FORCE, GL-dev tree),
  `src/synesthesia-vibe/CMakeLists.txt` (Apple-only archive merge), `vcpkg.json`,
  `vcpkg-configuration.json`, `triplets/`, and the `README.md` *Platforms* section ("Windows and
  Linux are not release targets").
- Existing macOS inner build tree `plugin/build_macos_arm64/synesthesia-vibe-core/build/build.ninja`,
  checked for compile flags. Finding: 65 of 75 core compile units have no `-O` flag (C-029).
- obs-plugintemplate `.github/` at the pinned commit `3e7d7ac3`, fetched read-only with `gh api`
  into the scratchpad (not the repo): `workflows/build-project.yaml`, `workflows/push.yaml`,
  `scripts/build-ubuntu`, `scripts/utils.zsh/setup_ubuntu`, `scripts/package-ubuntu`, and the
  full `.github` tree listing.
- Local tooling check: `actionlint` and `gh` (logged in) are present. `docker`, `podman`,
  `colima`, `act` and `lima` are missing, so there is no local Linux and no local workflow dry run.

## Skills (using)

- None beyond the planner role (`.claude/harness/planner.md`). The packet asked for none.

## Rules (read)

- Root `CLAUDE.md`: Rule 1 (provider-agnostic), Rule 2 (CodeGraph first, then `docs/`, then
  COMMON-ISSUES, then source), no code comments, the standard header with author `lee5783`
  (`#` lines for CMake, shell and YAML), no unit tests unless asked, kebab-case names, sibling
  style, public repo (no secrets, no personal email, no machine paths), and vendored template
  files kept verbatim.
- `.claude/harness/planner.md`: artifact contract and task format.
- `.claude/harness/developer.md`: the *Evidence* section, so each task's evidence expectation
  matches what the developer role must produce.
- `.claude/harness/HARNESS.md`: the *Stuck / long-running task capture* pointer (not triggered).
- Standing constraints from `inputs/requirements.md`: obs-websocket only, no OBS config edits,
  no profile/canvas/fps changes without asking, a separate scene collection, and outward-facing
  git actions (push, tag, release, PR, forum) only when the user asks.

## Write paths

- `<root>/plan/input-plan.md` (this file)
- `<root>/plan/output-plan-summary.md`
- `<root>/plan/output-plan-tasks.md`
- `<root>/plan/output-plan-test-cases.md`
- `<root>/plan/output-plan-concerns.md`
