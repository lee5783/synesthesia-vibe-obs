# SPK-002

## Subtasks executed
- Step 1, subdirectory probe: wrote a throwaway parent in `build/core-subdir-probe/CMakeLists.txt`
  (git-ignored) that calls `add_subdirectory()` on the submodule with `SYNESTHESIA_VIBE_GL_DEV=ON`,
  `SYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS=OFF`, `SYNESTHESIA_VIBE_BACKEND=gl` and the vcpkg
  toolchain. Configured it twice: once as given, then one retry with `VCPKG_MANIFEST_DIR` (S-014
  allows one). Both failed, and every failure is recorded.
- Step 2, fallback: configured the core on its own into `build/core-macos-arm64`, built
  `SynesthesiaVibeCore`, installed into `build/core-install`. Recorded the artifacts, their sizes,
  `lipo -info`, symbols and timings.
- Extra check: linked a C program against the installed prefix and ran it
  (`sv_validate_preset_memory` → `SV_OK`). This shows the merged `.a` is self-contained.
- Checked that CMake 4.4.5 accepts `cmake_minimum_required(3.25)` and `cmake_policy(SET CMP0091 OLD)`.
  Both are accepted; the policy line gives a deprecation warning only.
- Chose how the plugin drives the build: `ExternalProject_Add` (S-005, as the second Gate 1
  decided), with the recipe written into `PLAN.md`.
- Wrote the answer and a draft of upstream ask #1 into `PLAN.md`. Nothing was filed.

## Files changed
- `PLAN.md`:
  - Under **Building the core**, the SPK-001 sentence now points at the new answer, and two
    sub-bullets were added: the subdirectory verdict and the working build.
  - Upstream ask #1 is marked confirmed, with draft text.
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md`: SPK-002
  section appended.
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-SPK-002.md` (this file)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-002/SPK-002-report.md`
- Under the same `evidence/SPK-002/` (git-ignored): `SPK-002-subdirectory-configure-1.log`,
  `SPK-002-subdirectory-configure-2.log`, `SPK-002-core-build.log`.
- Throwaway build outputs, all under the git-ignored `build/`:
  - `build/core-subdir-probe/` (the probe parent `CMakeLists.txt` and its logs)
  - `build/core-macos-arm64/`
  - `build/core-install/`
  - `build/core-link-smoke/`
  - `build/core-{configure,build,install}.log`
- Nothing inside `external/synesthesia-vibe/`.

## What changed and why
Q1's answer is **no**: the core does not build as a subdirectory. Three things break, in this
order:
1. vcpkg manifest mode looks for `vcpkg.json` at the parent's root, so `find_package(projectM-Eval)`
   fails. `VCPKG_MANIFEST_DIR` works around it, but then `glfw3` and `gtest` get built too.
2. The core's `FORCE` writes wipe the parent's `CMAKE_CXX_FLAGS`.
3. `${CMAKE_SOURCE_DIR}` paths resolve against the parent. Generation fails with 6 missing source
   files and 6 targets that end up with no sources.

None of this can be fixed without editing the core, so the fallback is the path. The fallback
(separate configure, build and install) works:
- It takes 9 s to configure with a warm vcpkg cache (about 93 s cold) and 10 s to build.
- It produces `build/core-install/lib/libSynesthesiaVibeCore.a`: 4.6 MB, arm64, with projectM-Eval
  and ns-eel2 merged in, plus `include/synesthesia-vibe/synesthesia-vibe.h`.
- The submodule stays clean, including ignored files, because vcpkg's `vcpkg_installed/` defaults
  to the out-of-tree build dir.

## Verification
- Probe (expected to fail): `cmake -S build/core-subdir-probe -B build/core-subdir-probe/out -G Ninja …`
  exited 1 both times (attempt 1 at configure, attempt 2 at generate).
- Fallback commands (from the repo root), all exit 0:
  ```
  cmake -S external/synesthesia-vibe -B build/core-macos-arm64 -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET=arm64-osx \
    -DVCPKG_OVERLAY_TRIPLETS="$PWD/external/synesthesia-vibe/triplets" \
    -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 -DCMAKE_BUILD_TYPE=Release \
    -DSYNESTHESIA_VIBE_GL_DEV=ON -DSYNESTHESIA_VIBE_BACKEND=gl -DSYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS=OFF \
    -DCMAKE_INSTALL_PREFIX="$PWD/build/core-install"
  cmake --build build/core-macos-arm64 --target SynesthesiaVibeCore
  cmake --install build/core-macos-arm64
  ```
- `lipo -info build/core-install/lib/libSynesthesiaVibeCore.a` →
  `Non-fat file: … is architecture: arm64`.
- Link smoke test: `./build/core-link-smoke/smoke` → `sv_validate_preset_memory status=0 error=''`,
  exit 0, `minos 12.0`, `sdk 27.2`.
- `git -C external/synesthesia-vibe status --short --ignored` → empty. `git status` shows only
  the pre-existing entries plus the `PLAN.md` modification.
- No existing test suite covers this area. The core's GoogleTest suite is part of the desktop
  tools, which this task turns off.

## Evidence
- `evidence/SPK-002/SPK-002-report.md`: the full write-up with log excerpts. It covers both probe
  attempts and their errors, the parent-flags-wiped lines, the vcpkg package list, the fallback
  commands, the timing table, the install log, `ls -l`, `lipo -info`, `ar`/`nm` counts, the smoke
  test and the clean-submodule check.
- `evidence/SPK-002/SPK-002-subdirectory-configure-{1,2}.log` and `SPK-002-core-build.log`: the
  raw logs with paths scrubbed. They are git-ignored by the repo's evidence rule; the key lines
  are quoted in the report.
- This is a build-system task with no UI, so the evidence is log output.

## Known limitations / follow-ups
- The core installs no CMake package config. SPK-003 must declare an imported target
  (`IMPORTED_LOCATION <prefix>/lib/libSynesthesiaVibeCore.a`, include dir `<prefix>/include`) and
  pass the frameworks. The smoke test needed none for one validation call; the full API will
  likely need at least OpenGL/Foundation. Use the core's `INTERFACE` list (Foundation,
  AVFoundation, CoreAudio, OpenGL, AppKit, CoreVideo, IOSurface, QuartzCore) until SPK-003 shows
  which are needed.
- The vcpkg manifest builds `glfw3` and `gtest` even with desktop tools off (+~20 s on a cold
  build). This is harmless, and it is in the draft upstream ask.
- `ExternalProject_Add` needs `VCPKG_ROOT` in the environment at plugin configure time, plus an
  explicit `CMAKE_GENERATOR Ninja` because the template's outer generator is Xcode. If that clashes
  with the template, S-005's fallback is a `plugin/build-core.sh` running the three commands
  above.
- 105 compiler warnings in the core build (87 `-Wdeprecated-declarations` on the macOS 26 SDK),
  none fatal. Don't build the core with `-Werror`.
- arm64 only (S-004). A universal build needs a second `x64-osx` build plus `lipo`.
- The library exports 2522 global `glad_gl*` symbols. This is fine on macOS (two-level namespace),
  but it matters for upstream ask #3 on Linux; SPK-005 checks it.
- D4 was not edited (the task row didn't ask). The result supports D4's "submodule built from
  source by the plugin's CMake", via ExternalProject; SPK-008 sets D4's state.
- Doc gap for document-writer (SPK-010): put the core build recipe and the `VCPKG_ROOT`
  requirement in `docs/integrations.md`, and the `build/core-*` layout in `docs/structure.md`.
- Upstream ask #1 draft text is in `PLAN.md` *Upstream asks*; SPK-008 copies it into
  `SPK-008-upstream-asks.md`. Not filed.

## Handoff for downstream review
- Acceptance criteria addressed:
  - subdirectory yes/no with concrete reasons (no; three causes, quoted);
  - a working build path with exact commands, artifacts, sizes, arch and time;
  - the ExternalProject choice;
  - the CMake 4.4.5 policy check;
  - the upstream ask #1 draft;
  - evidence for both steps.
- Areas to scrutinize:
  - the `PLAN.md` wording and its fenced block inside a list item;
  - that no absolute machine paths appear in `PLAN.md` or the session files (checked with grep);
  - that the submodule is clean.
- How to reproduce: run the three fallback commands from the repo root with `VCPKG_ROOT` set; the
  probe parent is in `build/core-subdir-probe/CMakeLists.txt`.
