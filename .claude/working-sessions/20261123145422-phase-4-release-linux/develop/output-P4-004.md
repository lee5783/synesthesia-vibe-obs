# P4-004

## Subtasks executed
1. Guard relaxed to exactly two combinations: Darwin + Xcode + `CMAKE_OSX_ARCHITECTURES` `arm64`, or
   Linux + Ninja + host processor `x86_64`. Message names both.
2. Apple-only settings under `if(OS_MACOS)`: triplet `arm64-osx`, overlay triplets,
   `CMAKE_OSX_*`, the framework list, `XCODE_ATTRIBUTE_EXPORTED_SYMBOLS_FILE`,
   `GL_SILENCE_DEPRECATION`, the macOS loader source, and the `distribution.in` `configure_file`.
3. Linux branch: triplet `x64-linux`, `-DCMAKE_POSITION_INDEPENDENT_CODE=ON` and
   `-DCMAKE_INSTALL_LIBDIR=lib` to the core ExternalProject; links the core's installed
   `libprojectM_ns-eel2.a` and `libprojectM_eval.a` (declared as byproducts for Ninja),
   `Threads::Threads`, `${CMAKE_DL_LIBS}`, and on the plugin `OpenGL::OpenGL` + `OpenGL::EGL`.
4. Export control: new `plugin/exported-symbols.map` (same 7 names, `global:` / `local: *;`),
   `-Wl,--version-script=…`, `-Wl,--exclude-libs,ALL`, plus `-Wl,--no-undefined`; the map is a
   `LINK_DEPENDS` so editing it relinks.
5. `CPACK_PACKAGE_CONTACT "lee5783 <lee5783@users.noreply.github.com>"` on Linux, before
   `include(defaults)` (C-008).

## Files changed
- `plugin/CMakeLists.txt`
- `plugin/exported-symbols.map` (new; standard header as a `/* */` block, laid out like the XML
  header of `plugin/packaging/macos/distribution.in`)

## What changed and why
- The host processor comes from `cmake_host_system_information(QUERY OS_PLATFORM)`, because
  `CMAKE_HOST_SYSTEM_PROCESSOR` is still empty before `project()` (checked with a scratch project).
- The archives come from the core's **install** dir rather than the inner `vcpkg_installed/`: the
  core's own `install()` copies them to `${CMAKE_INSTALL_LIBDIR}` on non-Apple, and pinning
  `CMAKE_INSTALL_LIBDIR=lib` makes the path independent of GNUInstallDirs' distro detection. Order:
  core, ns-eel2, eval (ns-eel2 is the shim over eval). vcpkg's Linux toolchain already builds them
  with `-fPIC`.
- `CMAKE_POSITION_INDEPENDENT_CODE` is a cache variable the core's `FORCE`-cleared flags don't
  touch, so its targets get `-fPIC` through the target property (C-015).
- `CPACK_DEBIAN_PACKAGE_MAINTAINER` is set by the template from the empty buildspec `email`, and
  CPackDeb falls back to `CPACK_PACKAGE_CONTACT` when it's empty.
- `--no-undefined` is extra over the plan: it turns a missing library into a link error instead of
  a load failure inside OBS.

## Verification
macOS only (no Linux host):
- Clean `cmake --preset macos-arm64 && cmake --build --preset macos-arm64`: exit 0; Release builds.
- `nm -gU`: 7 names; `otool -L` and all imports unchanged (P4-003 evidence).
- `cmake --preset macos` and `cmake -S . -B build_ninja_scratch -G Ninja` both fail with the new
  guard message; `.CMakeBuildNumber` unchanged (3 before, 3 after); stub dirs removed.
- `grep hostArchitectures build_macos_arm64/distribution` → `hostArchitectures="arm64"`.
- `git diff feature/plugin-phase-2-macos -- plugin/cmake` and `-- external/synesthesia-vibe`: empty.

## Evidence
- `evidence/P4-004-guard-and-cmake.md`
- Linux evidence comes from the CI job (P4-005).

## Known limitations / follow-ups
- **The Linux branch has never been configured.** Expected first-run risks: the core under GCC 13
  (C-013 stop rule applies), the archive file names, the PPA libobs CMake config, and GCC `-Werror`
  on plugin sources that only clang has compiled so far.
- If a new optional `obs_module_*` entry point is added, it goes into both
  `exported-symbols.txt` and `exported-symbols.map`.
- The Linux minimum OBS is the PPA libobs on the build day (C-009); recorded by the CI job.
- Doc gaps (P4-012): `.claude/common-issues/plugin-build.md` and `docs/integrations.md` quote the old
  guard message verbatim; `docs/structure.md` (`exported-symbols.map`, `build_x86_64/`,
  `plugin/release/`); `docs/conventions.md` *Binary exports* (two export files) and the header rule
  for `.map` (`/* */`) and YAML (`#`); `docs/integrations.md` Linux build and package commands:
  `cd plugin && cmake --preset ubuntu-x86_64 -DCMAKE_INSTALL_PREFIX=/usr && cmake --build --preset
  ubuntu-x86_64 && cmake --build build_x86_64 --target package`.

## Handoff for downstream review
- Acceptance criteria addressed: guard with two combinations, Apple settings under `OS_MACOS`,
  Linux triplet/PIC/archives/GL/EGL/dl/threads, version script + `--exclude-libs,ALL`, CPack
  contact.
- Areas to scrutinize: the parenthesised `NOT (…) AND NOT (…)` guard; `BUILD_BYPRODUCTS` naming
  install-step outputs (needed by Ninja; the target-level dependency orders the link after the
  install step); `--no-undefined` interplay with libobs.
- How to run relevant tests: macOS commands above; Linux via CI job **Build / Linux x86_64**.
