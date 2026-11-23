# P4-001

## Subtasks executed
- Skip the `cmake/.CMakeBuildNumber` seed in `plugin/CMakeLists.txt` when the `CI` environment
  variable is truthy, so the vendored `buildnumber.cmake` takes its `GITHUB_RUN_ID` branch on CI.
- Add `plugin/release/` (the template's Linux `CPACK_OUTPUT_FILE_PREFIX`) to `.gitignore`.
- Evidence: fresh first configure without a counter file; a CI-like configure and build with
  `CI=true GITHUB_RUN_ID=4242`; `plugin/release/` ignored.

## Files changed
- `plugin/CMakeLists.txt` (seed condition)
- `.gitignore` (one line)

## What changed and why
The seed condition is now `NOT DEFINED PLUGIN_BUILD_NUMBER AND NOT "$ENV{CI}" AND NOT EXISTS
cmake/.CMakeBuildNumber`. `NOT "$ENV{CI}"` is the same truthiness test the template uses
(`if("$ENV{CI}")`), so the two can't disagree. Locally (no `CI`) the first-configure fix from Phase 2
still applies; on CI the template writes `GITHUB_RUN_ID` (or `1` when no run id is known), so the
empty-value bug that the seed works around can't occur there. The template stays verbatim.

## Verification
- `cmake --preset macos-arm64` after `mv build_macos_arm64 …-pre-p4 && rm -f cmake/.CMakeBuildNumber`:
  exit 0 on the first try, counter `1`.
- `CI=true GITHUB_RUN_ID=4242 cmake -S . -B build_ci_scratch -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0` and a build: `CFBundleVersion` = `4242` in the built
  `Info.plist`, `CURRENT_PROJECT_VERSION = 4242`. Scratch tree and the `4242` counter removed
  afterwards; normal reconfigure exit 0.
- `git check-ignore -v plugin/release/probe.deb` → `.gitignore:14:plugin/release/`.
- `git diff feature/plugin-phase-2-macos -- plugin/cmake` is empty.

## Evidence
- `evidence/P4-001-build-number.md`: commands and output for (a), (b), (c).
- The macOS CI job also asserts `CFBundleVersion == GITHUB_RUN_ID` on every run (TC-C07), so a
  regression fails CI.

## Known limitations / follow-ups
- A `.CMakeBuildNumber` restored into a CI workspace (e.g. by a cache that included `plugin/cmake/`)
  would make the template read and increment it instead of using the run id. No cache in the
  workflows includes it.
- Doc gaps (document-writer, P4-012): `docs/concerns.md` "build-number seed overrides CI numbers"
  is resolved; `.claude/common-issues/plugin-build.md` *Known trade-off* line is now out of date;
  `docs/structure.md` should list `plugin/release/` as generated and git-ignored (Linux CPack
  output).

## Handoff for downstream review
- Acceptance criteria addressed: CI skips the seed; local first configure still passes; CI build
  number is the run id; `plugin/release/` ignored.
- Areas to scrutinize: `NOT "$ENV{CI}"` inside a multi-line `if()`; behaviour when `CI` is set to
  `false` (treated as unset, same as the template).
- How to run relevant tests: see the evidence file.
