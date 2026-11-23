# Technology Stack

**Analysis Date:** 2026-11-23

The repo has two deliverables with separate stacks: the Browser Source page (`browser-source/`)
and the native OBS plugin (`plugin/`: macOS on Apple Silicon, plus an experimental Linux x86_64
build that has not been run in OBS). Both are built on the SynesthesiaVibe core, pinned at
`v0.7.0`.

## Languages

**Primary:**
- C++17 - `plugin/src/plugin-main.cpp`, `plugin/src/visualizer-source.cpp`,
  `plugin/src/audio-ring.cpp`, `plugin/src/opengl-loader-macos.cpp`,
  `plugin/src/opengl-loader-linux.cpp` (standard set by the template's
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
- GitHub Actions YAML - `.github/workflows/build-plugin.yaml`, `ci.yaml`, `release.yaml`; their
  `run:` steps are Bash (`shell: bash`), plus one inline `python3 -I` `dlopen` probe.
- GNU ld version script - `plugin/exported-symbols.map` (Linux export list).

## Runtime

**Environment:**
- Browser Source: OBS's embedded Chromium (obs-browser / CEF). Checked on OBS 33.0.3, CEF 150.
  Needs WebGL 2 (OBS *Browser Source Hardware Acceleration* on) and, for audio, a secure context
  (`localhost`) plus OBS started with `--enable-media-stream`.
- Plugin: loaded by OBS Studio as a macOS `.plugin` bundle. Built against libobs **31.1.1**,
  checked in OBS **33.0.3**. Requires OBS's **OpenGL** renderer (Apple's GL 4.1-on-Metal driver
  counts as OpenGL).
- Plugin on Linux (experimental): a shared object `synesthesia-vibe-obs.so` loaded by OBS from
  `/usr/lib/x86_64-linux-gnu/obs-plugins/`. Built against the PPA's libobs **32.2.0** (API 32.2.0,
  `libobs.so.30`), so it needs OBS 32.2.0 or newer. OBS on Linux always uses OpenGL, through EGL on
  both X11 and Wayland. Never run in OBS yet (P4-008).

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
  audio capture / `audio_output_connect` / `audio_resampler`, `os_task_queue`, hotkeys. On Linux,
  libobs 32.2.0 from `ppa:obsproject/obs-studio` instead, plus `obs_get_nix_platform()` for the
  windowing-platform log line.

**Testing:**
- None. See `docs/testing.md` for how changes are verified.

**Build/Dev:**
- `obsproject/obs-plugintemplate` layout, pinned at `3e7d7ac3b5342cd7d9b88890b9c70b472d1520fc`
  (vendored `plugin/cmake/**`, `plugin/src/plugin-support.*`).
- CMake ≥ 3.28 (`cmake_minimum_required(VERSION 3.28...3.30)`); checked with CMake 4.4.5.
- Xcode generator for the plugin on macOS (template presets); checked locally with Xcode 27.2,
  macOS 26 SDK, and in CI with Xcode 26.6.
- Ninja - required; the core's inner `ExternalProject_Add` build forces `CMAKE_GENERATOR Ninja`.
  On Linux it is also the plugin's own generator (`ubuntu-x86_64` presets).
- Linux toolchain: GCC 13 (Ubuntu 24.04's `build-essential`). The core and all plugin sources
  compile under it; the plugin's own sources with warnings as errors in CI (`ubuntu-ci-x86_64`).
- GitHub Actions (`.github/workflows/`):
  - runner images `macos-26` (arm64, Xcode selected explicitly: `Xcode_26.6.app`) and
    `ubuntu-24.04` (x86_64);
  - never a `-latest` label;
  - actions pinned by SHA (map in `docs/integrations.md`, *Pinned actions*);
  - vcpkg cloned at `5f96cd15fd745122cf27e0524606d6c1efc5fd07`;
  - `gh` (preinstalled) creates the draft release.
- `curl`, `unzip`, `tar`, `shasum` or `sha256sum`, `python3` - required by `fetch-dist.sh`.
- macOS packaging tools: `pkgbuild` / `productbuild` (run by the template's install step),
  `ditto`, `codesign`, `pkgutil`, `spctl`; `productsign`, `xcrun notarytool` and `xcrun stapler`
  for notarization (`docs/integrations.md`).
- Linux packaging: CPack DEB (template defaults, `/usr` prefix, `dpkg-shlibdeps` for `Depends`);
  `dpkg-deb`, `nm -D`, `readelf -d` and `ldd` for the CI checks.

## Key Dependencies

**Critical:**
- `external/synesthesia-vibe` submodule at `887628f` (tag `v0.7.0`) - the core the plugin links as
  `libSynesthesiaVibeCore.a`. On macOS it is arm64 with projectM-Eval and ns-eel2 merged in. On
  Linux it is x86_64, built with `-fPIC`, and the two archives stay separate
  (`libprojectM_ns-eel2.a`, `libprojectM_eval.a` in the core's install `lib/`).
- Core web release `v0.7.0` - downloaded and SHA-256-checked into `browser-source/dist/`.
- obs-studio 31.1.1 source + obs-deps / Qt6 `2025-07-11` prebuilts - downloaded by the template
  into `plugin/.deps/` at first configure (hashes in `plugin/buildspec.json`).

**Infrastructure:**
- Presets and textures: the core tag's source tarball (`resources/mvp-presets/`, 10 presets;
  `resources/beatdrop/textures/`, 23 textures). Local use only (see `docs/decisions.md`).
- Apple frameworks linked through the imported core target: Foundation, AVFoundation, CoreAudio,
  OpenGL, AppKit, CoreVideo, IOSurface, QuartzCore (`plugin/CMakeLists.txt`).
- Linux GL through GLVND: `OpenGL::OpenGL` (`libOpenGL.so.0`) and `OpenGL::EGL` (`libEGL.so.1`),
  plus `Threads::Threads` and `${CMAKE_DL_LIBS}`. No GLX library is linked: the core's GL pointers
  come from `eglGetProcAddress`, falling back to `dlsym` on `libOpenGL.so.0`, then `libGL.so.1`.

## Configuration

**Environment:**
- `VCPKG_ROOT` must point at a vcpkg checkout when configuring the plugin; configure stops with a
  `FATAL_ERROR` otherwise (`plugin/CMakeLists.txt`).
- `FETCH_DIST_BASE_URL` / `FETCH_DIST_ARCHIVE_URL` - optional test hooks for `fetch-dist.sh`.
- Browser Source settings come only from URL parameters (`preset`, `cycle`, `blend`, `fps`,
  `audio`; see `browser-source/README.md`).

**Build:**
- `plugin/CMakePresets.json` - on macOS only `macos-arm64` passes the build guard in
  `plugin/CMakeLists.txt`; on Linux x86_64 the template's `ubuntu-x86_64` and `ubuntu-ci-x86_64`
  (Ninja) pass. Dev and CI builds use `RelWithDebInfo`; releases use `Release`.
- `plugin/buildspec.json` - plugin identity (`synesthesia-vibe-obs`, version `0.1.0`, bundle id
  `io.github.lee5783.synesthesia-vibe-obs`) and dependency pins.
- `plugin/exported-symbols.txt` - Xcode `EXPORTED_SYMBOLS_FILE` (macOS).
- `plugin/exported-symbols.map` - GNU ld version script (Linux); the same 7 names.
- `.github/workflows/*.yaml` - CI and release (`docs/integrations.md`, *CI/CD & Deployment*).
- `.gitmodules` - one HTTPS submodule entry, no `branch =`.

## Platform Requirements

**Development:**
- Browser Source: any OS with the `fetch-dist.sh` tools and a static HTTP server. Checked on macOS;
  Linux (GNU tar / `sha256sum`) and Windows (Git Bash / WSL) unverified.
- Plugin: macOS on Apple Silicon, Xcode, CMake ≥ 3.28, Ninja, vcpkg (`VCPKG_ROOT`). arm64 only.
- Plugin on Linux: Ubuntu 24.04 x86_64, GCC 13, CMake ≥ 3.28, Ninja, vcpkg, OBS from the PPA, and
  the apt list in `docs/integrations.md`, *Linux build and package*.

**Production:**
- Browser Source: served locally from `browser-source/` on `http://localhost:8080/`; no hosting
  (D5 open).
- Plugin: an ad-hoc-signed, arm64-only `synesthesia-vibe-obs.plugin` in
  `~/Library/Application Support/obs-studio/plugins/`, installed from an unsigned `.pkg`
  (current user only, macOS 12.0 minimum) or a zip. Users install per `plugin/README.md`. Not
  notarized yet: Developer ID credentials are pending (`docs/integrations.md`). Distributed as
  draft prereleases on GitHub Releases, built by `release.yaml`.
- Plugin on Linux: a `.deb` (Ubuntu 24.04 x86_64, OBS 32.2.0+ from the PPA). It is a CI workflow
  artifact only, not a release asset, until it is tested in OBS. Flatpak OBS is unsupported.

---

*Stack analysis: 2026-11-23*
