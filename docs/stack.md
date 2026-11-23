# Technology Stack

**Analysis Date:** 2026-11-23

The repo has two deliverables with separate stacks: the Browser Source page (`browser-source/`)
and the native OBS plugin (`plugin/`, macOS on Apple Silicon). Both are built on the SynesthesiaVibe
core, pinned at `v0.7.0`.

## Languages

**Primary:**
- C++17 - `plugin/src/plugin-main.cpp`, `plugin/src/visualizer-source.cpp`,
  `plugin/src/audio-ring.cpp` (standard set by the template's
  `plugin/cmake/common/compiler_common.cmake`, also C17). Uses `std::atomic`, `std::mutex`,
  `std::deque`, `<random>` (`std::mt19937`); no third-party C++ library beyond libobs and the
  core.
- JavaScript (plain ES modules, no bundler, no npm) - `browser-source/obs-page.js`.

**Secondary:**
- HTML/CSS - `browser-source/index.html` (one full-window `<canvas>`, inline styles).
- Bash - `browser-source/fetch-dist.sh` (`set -euo pipefail`).
- Python 3 (inline, `python3 -I`) - the manifest step inside `fetch-dist.sh`, and the local static
  server.
- CMake - `plugin/CMakeLists.txt`, `plugin/CMakePresets.json`, the vendored `plugin/cmake/**`.
- C (template-generated) - `plugin/src/plugin-support.c.in` → `plugin-support.c` at configure.
- XML - `plugin/packaging/macos/distribution.in` (the pkg's Distribution file, configured at
  configure time).

## Runtime

**Environment:**
- Browser Source: OBS's embedded Chromium (obs-browser / CEF). Checked on OBS 33.0.3, CEF 150.
  Needs WebGL 2 (OBS *Browser Source Hardware Acceleration* on) and, for audio, a secure context
  (`localhost`) plus OBS started with `--enable-media-stream`.
- Plugin: loaded by OBS Studio as a macOS `.plugin` bundle. Built against libobs **31.1.1**,
  checked in OBS **33.0.3**. Requires OBS's **OpenGL** renderer (Apple's GL 4.1-on-Metal driver
  counts as OpenGL).

**Package Manager:**
- None for this repo's own code. The core's C++ dependencies come through **vcpkg** (manifest
  `external/synesthesia-vibe/vcpkg.json`, overlay triplets in `external/synesthesia-vibe/triplets/`).
- Lockfile: not applicable. Pins live in `browser-source/fetch-dist.sh` (`PINNED_VERSION`),
  the submodule gitlink and `plugin/buildspec.json` (sha256 hashes).

## Frameworks

**Core:**
- SynesthesiaVibe core `v0.7.0` (MIT, `lee5783/synesthesia-vibe`):
  - web dist `synesthesia-vibe-web-0.7.0.zip` (WebGL 2 / WASM, ES-module wrappers
    `SynesthesiaCanvas`, `AudioInput`, `ResourceLoader`, `loadSynesthesiaModule`) for the page;
  - C API `include/synesthesia-vibe/synesthesia-vibe.h` (`sv_*`) built from the submodule
    `external/synesthesia-vibe/` for the plugin.
- libobs 31.1.1 (GPL-2.0-or-later) - source plugin API, `gs_*` graphics, `obs_module_*` entry,
  audio capture / `audio_output_connect` / `audio_resampler`, `os_task_queue`, hotkeys.

**Testing:**
- None. See `docs/testing.md` for how changes are verified.

**Build/Dev:**
- `obsproject/obs-plugintemplate` layout, pinned at `3e7d7ac3b5342cd7d9b88890b9c70b472d1520fc`
  (vendored `plugin/cmake/**`, `plugin/src/plugin-support.*`).
- CMake ≥ 3.28 (`cmake_minimum_required(VERSION 3.28...3.30)`); checked with CMake 4.4.5.
- Xcode generator for the plugin (template presets); checked with Xcode 27.2, macOS 26 SDK.
- Ninja - required; the core's inner `ExternalProject_Add` build forces `CMAKE_GENERATOR Ninja`.
- `curl`, `unzip`, `tar`, `shasum` or `sha256sum`, `python3` - required by `fetch-dist.sh`.
- macOS packaging tools: `pkgbuild` / `productbuild` (run by the template's install step),
  `ditto`, `codesign`, `pkgutil`, `spctl`; `productsign`, `xcrun notarytool` and `xcrun stapler`
  for notarization (`docs/integrations.md`).

## Key Dependencies

**Critical:**
- `external/synesthesia-vibe` submodule at `887628f` (tag `v0.7.0`) - the core the plugin links as
  `libSynesthesiaVibeCore.a` (arm64, projectM-Eval and ns-eel2 merged in).
- Core web release `v0.7.0` - downloaded and SHA-256-checked into `browser-source/dist/`.
- obs-studio 31.1.1 source + obs-deps / Qt6 `2025-07-11` prebuilts - downloaded by the template
  into `plugin/.deps/` at first configure (hashes in `plugin/buildspec.json`).

**Infrastructure:**
- Presets and textures: the core tag's source tarball (`resources/mvp-presets/`, 10 presets;
  `resources/beatdrop/textures/`, 23 textures). Local use only (see `docs/decisions.md`).
- Apple frameworks linked through the imported core target: Foundation, AVFoundation, CoreAudio,
  OpenGL, AppKit, CoreVideo, IOSurface, QuartzCore (`plugin/CMakeLists.txt`).

## Configuration

**Environment:**
- `VCPKG_ROOT` must point at a vcpkg checkout when configuring the plugin; configure stops with a
  `FATAL_ERROR` otherwise (`plugin/CMakeLists.txt`).
- `FETCH_DIST_BASE_URL` / `FETCH_DIST_ARCHIVE_URL` - optional test hooks for `fetch-dist.sh`.
- Browser Source settings come only from URL parameters (`preset`, `cycle`, `blend`, `fps`,
  `audio`; see `browser-source/README.md`).

**Build:**
- `plugin/CMakePresets.json` - only `macos-arm64` (configure + build preset) passes the build
  guard in `plugin/CMakeLists.txt`. Dev builds use the default `RelWithDebInfo`; releases use
  `--config Release`.
- `plugin/buildspec.json` - plugin identity (`synesthesia-vibe-obs`, version `0.1.0`, bundle id
  `io.github.lee5783.synesthesia-vibe-obs`) and dependency pins.
- `plugin/exported-symbols.txt` - Xcode `EXPORTED_SYMBOLS_FILE`.
- `.gitmodules` - one HTTPS submodule entry, no `branch =`.

## Platform Requirements

**Development:**
- Browser Source: any OS with the `fetch-dist.sh` tools and a static HTTP server. Checked on macOS;
  Linux (GNU tar / `sha256sum`) and Windows (Git Bash / WSL) unverified.
- Plugin: macOS on Apple Silicon, Xcode, CMake ≥ 3.28, Ninja, vcpkg (`VCPKG_ROOT`). arm64 only.

**Production:**
- Browser Source: served locally from `browser-source/` on `http://localhost:8080/`; no hosting
  (D5 open).
- Plugin: an ad-hoc-signed, arm64-only `synesthesia-vibe-obs.plugin` in
  `~/Library/Application Support/obs-studio/plugins/`, installed from an unsigned `.pkg`
  (current user only, macOS 12.0 minimum) or a zip. Users install per `plugin/README.md`. Not
  notarized yet: Developer ID credentials are pending (`docs/integrations.md`).

---

*Stack analysis: 2026-11-23*
