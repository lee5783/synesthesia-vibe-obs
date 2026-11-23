# P2-001

## Subtasks executed
- Added one configure guard to `plugin/CMakeLists.txt`: macOS host + Xcode generator +
  `CMAKE_OSX_ARCHITECTURES` exactly `arm64`, else a single `FATAL_ERROR` naming the detected
  values and pointing to the `macos-arm64` configure and build presets.
- Kept the `.CMakeBuildNumber` seed guard unchanged.
- Rechecked the refused cases and a from-scratch `macos-arm64` configure and build.

## Files changed
- `plugin/CMakeLists.txt`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-P2-001.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-001/build-guards.md`

## What changed and why
`docs/concerns.md` recorded that the plugin build is macOS / arm64 / Xcode only and nothing
enforced it. The template's universal `macos` / `macos-ci` presets failed late at link, and a
non-Xcode generator silently dropped `XCODE_ATTRIBUTE_EXPORTED_SYMBOLS_FILE`, which brings back
cross-image weak-symbol coalescing. The guard sits directly after `cmake_minimum_required`, before
the build-number seed and the template bootstrap. A refused configure therefore writes nothing into
the source tree and never starts the libobs download/build. `CMAKE_HOST_SYSTEM_NAME`,
`CMAKE_GENERATOR` and a cache-set `CMAKE_OSX_ARCHITECTURES` are all known before `project()`. The
message layout follows the template's `message(FATAL_ERROR "..." "...")` shape. Universal
(P2-001b) is not implemented, per C-016.

## Verification
- Refused (exit 1, same message, `CMakeLists.txt:15`): `-G Ninja` with arm64,
  `-G Xcode` with `arm64;x86_64`, with `x86_64`, and with no architecture; `cmake --preset macos`;
  `cmake --preset macos-ci`. `.CMakeBuildNumber` byte-identical before and after.
- From scratch (old `plugin/build_macos_arm64` moved to the session scratchpad, `.CMakeBuildNumber`
  removed, `.deps` reused): `cmake --preset macos-arm64` passed first try (65.9 s), then
  `cmake --build --preset macos-arm64` gave `** BUILD SUCCEEDED **` (30 s). All 105 warnings come
  from `external/synesthesia-vibe/`; none from `plugin/src`. A re-configure of the fresh tree also
  passes.
- `lipo -info`: arm64 only. `nm -gU`: exactly the 7 `obs_module_*` names, identical to
  `plugin/exported-symbols.txt`. Ad-hoc signed.

## Evidence
- `evidence/P2-001/build-guards.md`: verbatim guard output for each refused case, the tails of the
  from-scratch configure and build, the warning breakdown, `lipo -info`, `nm -gU`, `codesign -dv`.

## Known limitations / follow-ups
- `CMAKE_OSX_ARCHITECTURES` must come from the cache (preset or `-D`). Setting it only through the
  `CMAKE_OSX_ARCHITECTURES` environment variable is refused, because CMake applies that variable
  at `project()`, after the guard. Harmless (it points to the preset), but worth knowing.
- The template's `windows-*` / `ubuntu-*` presets are now refused by the same guard (they could
  not build before either). Phase 3/4 will have to relax it per OS.
- Refused preset configures still leave a stub `plugin/build_macos/` (CMakeCache only). It is
  git-ignored by `plugin/build_*/`.
- Doc gaps for document-writer: `docs/concerns.md` "Plugin build is macOS / arm64 / Xcode only,
  and nothing enforces it" is now enforced (keep the universal/Linux follow-ups);
  `docs/integrations.md` *Local build and install* could note that other generators, architectures
  and the template presets stop at configure with the guard's message; consider a
  `.claude/common-issues/plugin-build.md` entry quoting the message.
- No unit test needed (CMake configure guard).

## Handoff for downstream review
- Acceptance criteria addressed: from-scratch `macos-arm64` configure and build pass on the first
  try with no `plugin/src` warnings; `cmake --preset macos` fails at configure with the new
  message; `lipo` arm64; `nm -gU` the 7 names; the seed guard is kept; no file under
  `plugin/cmake/**` or `CMakePresets.json` changed.
- Areas to scrutinize: the guard's position before the seed/bootstrap (relies on
  `CMAKE_HOST_SYSTEM_NAME` being set before `project()`, as the template's `osconfig.cmake`
  already does); the quoted `"${CMAKE_OSX_ARCHITECTURES}"` comparison, which handles an empty
  value and a `;`-list.
- How to run: from `plugin/`, `cmake -S . -B /tmp/x -G Ninja` (refused) and
  `cmake --preset macos` (refused; delete the stub `build_macos/` afterwards).
