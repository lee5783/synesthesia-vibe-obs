# External Integrations

**Analysis Date:** 2026-11-23

## APIs & External Services

**SynesthesiaVibe core releases (GitHub, `lee5783/synesthesia-vibe`):**
- Web dist for the Browser Source, fetched by `browser-source/fetch-dist.sh [version]`.
  - Release assets: `https://github.com/lee5783/synesthesia-vibe/releases/download/v<ver>/SHA256SUMS.txt`
    and `…/synesthesia-vibe-web-<ver>.zip`. The zip is verified against `SHA256SUMS.txt`; a
    missing line, a mismatch or a partial download exits 1 before anything is staged.
  - Source tarball: `https://github.com/lee5783/synesthesia-vibe/archive/refs/tags/v<ver>.tar.gz`.
    Only `LICENSE`, `NOTICE`, `resources/mvp-presets/*.milk` and the top-level files of
    `resources/beatdrop/textures/` are extracted. No published checksum (accepted, C-010).
  - `version` is optional, defaults to `PINNED_VERSION="0.7.0"`, and accepts `0.7.0` or `v0.7.0`.
  - Test hooks: `FETCH_DIST_BASE_URL` replaces the release base, `FETCH_DIST_ARCHIVE_URL` the
    tarball base (e.g. a local `file://` mirror). The checksum then comes from the overridden base
    too, so an override checks self-consistency only.
  - Required tools: `bash`, `curl`, `unzip`, `tar`, `python3`, and `shasum` or `sha256sum`.
- Core source for the plugin: the git submodule `external/synesthesia-vibe/`
  (`https://github.com/lee5783/synesthesia-vibe.git`), pinned at `v0.7.0` (`887628f`). Fetch it
  with `git submodule update --init --recursive` (no nested submodules; `--recursive` is
  harmless). Never run `git submodule update --remote`: there is no `branch =` line, so it would
  move the pin to the default branch.

**OBS Studio:**
- **obs-browser (Browser Source).** Loads `http://localhost:8080/` plus URL parameters. Setup,
  parameters and troubleshooting live in `browser-source/README.md`; do not duplicate them here.
  - OBS must start with `--enable-media-stream` for audio
    (`open -a OBS --args --enable-media-stream` on macOS, with OBS fully quit first).
  - *Enable Browser Source Hardware Acceleration* must be on for WebGL 2.
- **libobs (plugin).** The plugin is an `OBS_SOURCE_TYPE_INPUT` source with id
  `synesthesia_vibe_visualizer`, display name "SynesthesiaVibe Visualizer" (from
  `plugin/data/locale/en-US.ini`). It registers only on the OpenGL renderer.
- **obs-websocket.** Used only to verify changes (scene and input creation, `GetSourceScreenshot`,
  `GetInputKindList`, `GetStats`) in a dedicated `synesthesia-spike` scene collection. No repo
  file reads or stores websocket credentials. GUI automation of OBS is not allowed.

**Plugin build downloads (first configure):**
- obs-plugintemplate's buildspec mechanism downloads the obs-studio 31.1.1 source tag and the
  obs-deps / Qt6 `2025-07-11` prebuilts, sha256-checked against `plugin/buildspec.json`, and builds
  libobs into `plugin/.deps/`. This takes about 3 minutes once.
- vcpkg installs the core's dependencies into the inner build dir (warm binary cache: about 9 s;
  cold: about 93 s). The manifest also builds `glfw3` and `gtest`, which only the core's desktop
  tools use.

## Data Storage

**Databases:**
- None.

**File Storage:**
- Local filesystem only.
  - `browser-source/dist/` (git-ignored): web dist, `LICENSE`, `NOTICE`, `VERSION`,
    `resources/presets/`, `resources/textures/`, generated `resources/manifest.json`
    (`{"presets": [...], "textures": [...], "audio": []}`, sorted).
  - Plugin bundle resources: `Contents/Resources/presets/` and `Contents/Resources/textures/`
    inside the installed `.plugin`, copied by hand (no install step ships them).
  - Core shader cache: `obs_module_config_path("cache")`, created at source creation.

**Caching:**
- None beyond the core's shader cache and vcpkg's binary cache.

## Authentication & Identity

**Auth Provider:**
- None. Code signing is ad-hoc only (`CODESIGN_IDENTITY` / `CODESIGN_TEAM` left empty); OBS loads
  ad-hoc plugins because `OBS.app` carries `disable-library-validation`.

## Monitoring & Observability

**Error Tracking:**
- None.

**Logs:**
- Browser Source: browser console, every line prefixed `obs-page:`. OBS copies the console's
  **error** lines into its own log as `[obs-browser: '<source name>'] Error: …`; info and warning
  lines appear only in the console. Two core startup lines (`render_gl: audio textures …`,
  `synesthesia-vibe: GL pipeline ready …`) arrive on the error channel and are harmless.
- Full console from OBS: start OBS with `--remote-debugging-port=9222`; the page is listed at
  `http://localhost:9222/json/list` (title *SynesthesiaVibe*). While the flag is on, any local
  program can control OBS's browser, so turn it off afterwards.
- Plugin: `obs_log` lines prefixed `[synesthesia-vibe-obs]` in the OBS log. Expected on a good run:
  `renderer check passed (OpenGL, device type 1)`, `plugin loaded successfully (version 0.1.0)`,
  `sv_init_gl and sv_create succeeded (1920x1080)`, `preset loaded: …`, `plugin unloaded`.

## CI/CD & Deployment

**Hosting:**
- None. The Browser Source is served locally; GitHub Pages hosting (D5) is open and skipped for
  now, because the presets are approved for local serving only.

**CI Pipeline:**
- None. The template's `macos-ci`, `windows-ci-x64` and `ubuntu-ci-x86_64` presets exist but are not
  wired to any workflow, and only `macos-arm64` builds (see `docs/concerns.md`).

## Local build and install

Run from the repository root unless noted.

**Browser Source:**

```sh
browser-source/fetch-dist.sh
python3 -m http.server 8080 --bind 127.0.0.1 --directory browser-source
```

`--bind 127.0.0.1` keeps the presets off the local network. `localhost` is a secure context, so
plain HTTP is enough for audio capture; a LAN IP or hostname is not.

**Plugin (macOS, Apple Silicon):**

Prerequisites: Xcode, CMake ≥ 3.28, `ninja` on the `PATH`, `VCPKG_ROOT` set, submodule fetched.

```sh
cd plugin
cmake --preset macos-arm64
cmake --build --preset macos-arm64
```

Output: `plugin/build_macos_arm64/RelWithDebInfo/synesthesia-vibe-obs.plugin` (arm64,
ad-hoc-signed). Every build re-runs the core's ExternalProject build and install steps
(`BUILD_ALWAYS TRUE`); the inner Ninja build is a no-op when nothing changed.

Install with `ditto`, not `cmake --install` (the template's install runs `pkgbuild` /
`productbuild` and drops a `.pkg` into the plugins folder). Quit OBS first:

```sh
P="$HOME/Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin"
ditto plugin/build_macos_arm64/RelWithDebInfo/synesthesia-vibe-obs.plugin "$P"
mkdir -p "$P/Contents/Resources/presets" "$P/Contents/Resources/textures"
cp external/synesthesia-vibe/resources/mvp-presets/*.milk "$P/Contents/Resources/presets/"
cp external/synesthesia-vibe/resources/beatdrop/textures/* "$P/Contents/Resources/textures/"
codesign --force --sign - "$P" && codesign --verify "$P"
```

The re-sign is required after modifying the bundle. Without the preset copy the source shows
nothing: its default `preset_path` is `presets/Rovastar - Fractopia (Upspoken Mix).milk` inside the
bundle. Check the export list with `nm -gU "$P/Contents/MacOS/synesthesia-vibe-obs"` (exactly the 7
names in `plugin/exported-symbols.txt`).

Uninstall: quit OBS and delete `$P`.

## Environment Configuration

**Required env vars:**
- `VCPKG_ROOT` (plugin configure).

**Optional env vars:**
- `FETCH_DIST_BASE_URL`, `FETCH_DIST_ARCHIVE_URL` (`fetch-dist.sh` test hooks).

**Secrets location:**
- None in the repo. Local obs-websocket credentials, if used for verification, stay outside the
  repo and out of every committed file.

## Webhooks & Callbacks

**Incoming:**
- None.

**Outgoing:**
- None.

---

*Integration audit: 2026-11-23*
