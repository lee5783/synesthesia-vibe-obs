# SPK-003

## Subtasks executed
- Pinned `obsproject/obs-plugintemplate` at `3e7d7ac3b5342cd7d9b88890b9c70b472d1520fc`
  (2025-12-09). Copied `cmake/**`, `src/plugin-support.c.in` and `src/plugin-support.h` into
  `plugin/` byte-for-byte, with no headers added and comments kept (S-006). Left out `.github/`,
  `build-aux/`, `.clang-format`, `.gersemirc`, the template `.gitignore`, `LICENSE`, `README.md` and
  `src/plugin-main.c`.
- `buildspec.json` keeps the template's dependency pins (obs-studio 31.1.1, obs-deps/Qt6
  2025-07-11) and sets the S-007 identity: `synesthesia-vibe-obs`, "SynesthesiaVibe Visualizer",
  `0.1.0`, `lee5783`, the repo URL, an empty email, and bundle ID
  `io.github.lee5783.synesthesia-vibe-obs`.
- `CMakePresets.json`: added a `macos-arm64` configure preset and build preset (they inherit
  `macos`, override `CMAKE_OSX_ARCHITECTURES=arm64`, and use binary dir `build_macos_arm64`). The
  template presets are unchanged (S-004). The generator stays Xcode, which the template requires on
  macOS.
- `plugin/CMakeLists.txt` is based on the template's. It adds:
  - `ExternalProject_Add(synesthesia-vibe-core)`, using SPK-002's recipe: Ninja inner generator,
    the vcpkg toolchain from `$ENV{VCPKG_ROOT}`, `arm64-osx`, `GL_DEV=ON`, `BACKEND=gl`, desktop
    tools OFF, Release, `--target SynesthesiaVibeCore`, install into
    `build_macos_arm64/synesthesia-vibe-core/install`, and `BUILD_BYPRODUCTS` on the `.a`.
  - An imported static target `SynesthesiaVibe::Core` that carries the framework list.
  - A guard that seeds `cmake/.CMakeBuildNumber`. This works around a first-run template bug; see
    the evidence report.
- Source:
  - `obs_module_load` registers the `synesthesia_vibe_visualizer` input source
    (`OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW`). Its display name comes from the locale.
  - `create`/`destroy`, 1920×1080 `get_width`/`get_height`, and an empty `video_render`.
  - The core link is proven by calling `sv_last_error(nullptr)`. It is null-safe, needs no GL or
    context, and is outside the header's GL-thread list. `sv_validate_preset_memory` was not
    used: the header requires an `sv_create` first, and it writes temp files.
- Built the plugin (configure, then build), ditto-installed the bundle into the user plugin folder,
  restarted OBS, confirmed the module loads, added the source in a `synesthesia-spike` scene
  collection, then restored the user's collection.

## Files changed
- `plugin/CMakeLists.txt` (new, header)
- `plugin/CMakePresets.json` (template copy + `macos-arm64` presets)
- `plugin/buildspec.json` (template copy, identity values)
- `plugin/cmake/**` (verbatim template copies, 25 files)
- `plugin/src/plugin-support.c.in`, `plugin/src/plugin-support.h` (verbatim template copies)
- `plugin/src/plugin-main.cpp` (new, header)
- `plugin/src/visualizer-source.cpp`, `plugin/src/visualizer-source.hpp` (new, header)
- `plugin/data/locale/en-US.ini` (new, `#` header; OBS's text-lookup treats `#` as a comment)
- `.gitignore`: added `plugin/.deps/`, `plugin/build_*/`, `plugin/cmake/.CMakeBuildNumber`,
  `plugin/CMakeUserPresets.json`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md`: SPK-003
  section appended
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-SPK-003.md`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-003/`:
  `SPK-003-report.md`, `SPK-003-build.log`, `SPK-003-obs-log.txt`, `SPK-003-add-source.png`,
  `SPK-003-properties.png`, `SPK-003-source-in-scene.png`
- Outside the repo: `~/Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin`,
  left installed for SPK-004+ (S-012). Also an OBS scene collection `synesthesia-spike`.
- Generated and git-ignored: `plugin/.deps/` (about 3 min to set up on first configure),
  `plugin/build_macos_arm64/`, `plugin/cmake/.CMakeBuildNumber`.
- Not touched: `external/synesthesia-vibe/` (clean, including ignored files), `PLAN.md`, `docs/`.

## What changed and why
`plugin/` is now the real start of the Phase 2 plugin, as the S-003 decision requires:
- One entry point (`cmake --preset macos-arm64` then `cmake --build --preset macos-arm64`) builds
  libobs 31.1.1 from the template buildspec, builds and installs the core through ExternalProject,
  and links an arm64, ad-hoc-signed `synesthesia-vibe-obs.plugin`.
- That plugin registers an empty "SynesthesiaVibe Visualizer" source.
- libobs 31.1.1 built cleanly with Xcode 27.2 and the macOS 26 SDK, so no 33.0.3 bump was needed.
- The Xcode outer / Ninja inner ExternalProject combination works, so S-005's `build-core.sh`
  fallback is not needed.

## Verification
- `cd plugin && cmake --preset macos-arm64`:
  - The first run failed after the libobs build, because of the template's empty build-number bug.
  - Fixed with the guard. Re-verified from clean (`rm -rf build_macos_arm64 cmake/.CMakeBuildNumber`):
    exit 0 in 41.8 s with `.deps` reused.
- `cmake --build --preset macos-arm64`: `** BUILD SUCCEEDED **` in 27 s, including the core
  configure, build and install. 105 warnings, all in `external/synesthesia-vibe`, none in `plugin/`.
- Bundle checks:
  - `lipo -info`: arm64.
  - `codesign -dvv`: `Signature=adhoc`, flags `adhoc,runtime`, no team.
  - `codesign --verify` of the installed copy: valid.
- OBS 33.0.3 log: `[synesthesia-vibe-obs] core linked (sv_last_error(NULL) = "")`,
  `plugin loaded successfully (version 0.1.0)`, the module appears in "Loaded Modules", and
  `User added source 'SynesthesiaVibe Visualizer' (synesthesia_vibe_visualizer)`.
- No existing test suite covers this area.

## Evidence
- `evidence/SPK-003/SPK-003-report.md`: the write-up with every key log line quoted (tracked).
- `evidence/SPK-003/SPK-003-build.log`: configure (failing first run, second run, clean re-run),
  build, ExternalProject steps, bundle facts and the clean-submodule check. Paths are scrubbed.
- `evidence/SPK-003/SPK-003-obs-log.txt`: OBS log excerpt covering the OpenGL renderer, the module
  load, the collection switch, the source add and the restore. Paths and the device name are
  scrubbed.
- `evidence/SPK-003/SPK-003-add-source.png`: the OBS "Add Source" dialog listing and selecting
  "SynesthesiaVibe Visualizer".
- `evidence/SPK-003/SPK-003-properties.png`: the source's properties dialog ("No properties
  available"), with the source in the Sources list.
- `evidence/SPK-003/SPK-003-source-in-scene.png`: the source in the `synesthesia-spike` scene,
  drawing nothing inside its 1920×1080 box.
- `GetInputKindList` evidence is missing: the local obs-websocket credentials failed
  authentication (close code 4009), 3 attempts including one after the OBS restart. The OBS log
  line with the source id stands in for it.

## Known limitations / follow-ups
- **obs-websocket credentials don't match OBS's server password.** The orchestrator or user should
  re-check them before SPK-004/005. As a fallback I drove the OBS GUI through macOS accessibility:
  System Events menu clicks, plus synthetic clicks from a small helper in the session scratchpad,
  not the repo. It only created and switched to `synesthesia-spike`, added the source there, and
  switched back to the user's `Untitled` collection. If that goes beyond what S-010 allows,
  later tasks should ask the user to click instead.
- `PLAN.md` was not edited: the delegation limits it to "only if the task row says so", but S-006's
  default also asks for the template SHA there. The SHA to record is
  `3e7d7ac3b5342cd7d9b88890b9c70b472d1520fc`; SPK-008 or the orchestrator should add it.
- The installed bundle came from the build before the `.CMakeBuildNumber` guard was added. The
  guard only affects configure, and the sources are identical.
- Template quirks to remember:
  - The template's own libobs sub-build is hard-coded universal (`arm64;x86_64`). This is harmless:
    the plugin itself is arm64.
  - The template's Release/RelWithDebInfo signing turns on hardened runtime. OBS loads the plugin
    anyway thanks to `disable-library-validation` (S-002 confirmed).
  - `cmake --install` would also run the template's `pkgbuild`/`productbuild` and drop a `.pkg`
    into the plugins folder, so the local install uses `ditto` instead.
- The ExternalProject block and the framework list are macOS-only, with no `OS_MACOS` guard yet.
  Windows is Phase 2/4 work.
- The frameworks are the core's full list from SPK-002 (Foundation, AVFoundation, CoreAudio,
  OpenGL, AppKit, CoreVideo, IOSurface, QuartzCore). SPK-005 can narrow it once real calls exist.
- The plugin binary exports the core's `glad_gl*` / `GLAD_GL_*` symbols globally. The template's
  hidden-visibility preset doesn't reach the prebuilt static library. This is fine on macOS
  (two-level namespace) and relevant to upstream ask #3.
- OBS logs `Failed to load 'en-GB' text for module: 'synesthesia-vibe-obs'` because the UI locale
  is en-GB. This is benign (it falls back to en-US). An `en-GB.ini` would silence it, if wanted.
- The `core linked (…)` log line is spike instrumentation. Drop it once SPK-005 calls the core for
  real.
- `.vscode/settings.json` appeared untracked at 01:46 during this task. I did not create it; it is
  probably the editor's CMake extension reacting to `plugin/CMakeLists.txt`. Left alone.
- Doc gaps for document-writer (SPK-010):
  - `docs/integrations.md`: the preset commands, the `VCPKG_ROOT` and `ninja` requirements, the
    first-configure cost (about 3 min to download obs-deps/Qt6/obs-studio and build libobs), and
    the ditto install.
  - `docs/structure.md`: the `plugin/` layout, verbatim versus authored files, and the template
    SHA.
  - `.claude/common-issues/`: the template first-run build-number bug.

## Handoff for downstream review
- Acceptance criteria addressed:
  - The OBS log shows the module loading with no errors from it.
  - The source type appears in OBS's "Add Source" list (screenshot).
  - `.deps/` and the build dirs are git-ignored (`git check-ignore` confirmed).
- Areas to scrutinize:
  - `plugin/CMakeLists.txt`: the ExternalProject args, the imported target, the build-number
    guard sitting before the template bootstrap, and `add_dependencies` on the module target.
  - The `CMakePresets.json` addition.
  - That the verbatim files are byte-identical to the pinned commit.
  - That no absolute paths or personal data are in the repo files or the evidence.
- How to reproduce:
  1. From `plugin/` with `VCPKG_ROOT` set and `ninja` on PATH, run `cmake --preset macos-arm64`
     then `cmake --build --preset macos-arm64`.
  2. Quit OBS.
  3. Run `ditto build_macos_arm64/RelWithDebInfo/synesthesia-vibe-obs.plugin "$HOME/Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin"`.
  4. Relaunch OBS.
