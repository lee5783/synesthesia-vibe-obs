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
  `plugin/data/locale/en-US.ini`). It registers only on the OpenGL renderer. libobs APIs it
  relies on beyond the source callbacks: `obs_source_add_audio_capture_callback` +
  `audio_resampler_*` (chosen audio source), `audio_output_connect` on `obs_get_audio()` (whole
  mix), `os_task_queue_*` (preset worker), `os_opendir` (folder scan),
  `obs_hotkey_register_source` (hotkeys), `obs_get_base_effect(OBS_EFFECT_OPAQUE)` (draw).
- **obs-websocket.** Used only to verify changes (scene and input creation, `GetSourceScreenshot`,
  `GetInputKindList`, `GetStats`, `TriggerHotkeyByName`, `OpenInputPropertiesDialog`) in a
  dedicated `synesthesia-spike` scene collection (`docs/testing.md`). No repo file reads or stores
  websocket credentials. GUI automation of OBS is not allowed.

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
  - Plugin presets: a folder the user picks in the source's "Preset folder" setting. No presets
    or textures ship in the bundle, pkg or zip, and the plugin never reads the bundle for presets
    (C-003). Named textures are found next to the `.milk` files.
  - Plugin bundle resources: `Contents/Resources/locale/en-US.ini` only. The core's random-texture
    picker scans `<resource path>/textures`, which does not exist in the bundle
    (`docs/concerns.md`).
  - Core shader cache: `obs_module_config_path("cache")`, created at source creation.

**Caching:**
- None beyond the core's shader cache and vcpkg's binary cache.

## Authentication & Identity

**Auth Provider:**
- None. Code signing is ad-hoc (`CODESIGN_IDENT` / `CODESIGN_TEAM` unset), with the hardened
  runtime flag on non-Debug builds (`xcode.cmake`); OBS loads ad-hoc plugins because `OBS.app`
  carries `disable-library-validation`. The pkg is unsigned. Developer ID signing and
  notarization are documented below and wait on credentials the user supplies out of band
  (C-002).

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
  `sv_init_gl and sv_create succeeded (1920x1080)`, `auto-cycle: every 16 s, blend 2.7 s,
  sequential order`, `preset prepared: <path> (… ms)`, `preset committed: <path> (blend … s, …
  ms)`, `audio source attached: …` or `audio mix connected: track 1 (44100 Hz stereo)`,
  `preset worker stopped (… ms)` on removal, `plugin unloaded`. Failure lines:
  `preset folder not set / not found / has no .milk presets`, `preset failed to prepare: …`,
  `preset failed to commit: …`, `audio source not found (…); checking again every second`. Lines
  do not name the source instance. The user-facing list is in `plugin/README.md`, *Reading the
  OBS log*.

## CI/CD & Deployment

**Hosting:**
- None. The Browser Source is served locally; GitHub Pages hosting (D5) is open and skipped for
  now, because the presets are approved for local serving only.

**CI Pipeline:**
- None. The template's `macos-ci`, `windows-ci-x64` and `ubuntu-ci-x86_64` presets exist but are not
  wired to any workflow. The build guard in `plugin/CMakeLists.txt` refuses every preset but
  `macos-arm64`.

**Releases:**
- Artifacts are built locally (see *Packaging*): an unsigned `.pkg`, a zip of the ad-hoc-signed
  `.plugin`, both arm64 only. No release has been published, and the pkg install into OBS is not
  verified end to end yet (`docs/concerns.md`, *Deferred checks*).

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

The configure stops at once, before the template bootstrap or the libobs download, unless the host
is macOS, the generator is Xcode and `CMAKE_OSX_ARCHITECTURES` is exactly `arm64` from the cache
(preset or `-D`; the environment variable is applied too late and is refused). That refuses
`-G Ninja`, universal or `x86_64` builds and the template's `macos`, `macos-ci`, `windows-*` and
`ubuntu-*` presets with one message:

```
SynesthesiaVibe for OBS builds only on macOS, with the Xcode generator, for arm64 alone. Detected
host '…', generator '…', CMAKE_OSX_ARCHITECTURES '…'. Configure with 'cmake --preset macos-arm64'
and build with 'cmake --build --preset macos-arm64'.
```

A refused preset configure leaves a stub `plugin/build_macos/`; delete it.

**Dev install** (quit OBS first):

```sh
P="$HOME/Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin"
rm -rf "$P"
ditto plugin/build_macos_arm64/RelWithDebInfo/synesthesia-vibe-obs.plugin "$P"
codesign --verify "$P"
nm -gU "$P/Contents/MacOS/synesthesia-vibe-obs"
```

Use `ditto`, not `cmake --install` without `--prefix`: `cmake/macos/defaults.cmake` caches
`CMAKE_INSTALL_PREFIX` as the real `~/Library/Application Support/obs-studio/plugins`, and the
template's install step runs `pkgbuild` / `productbuild` and drops a `.pkg` there. `nm -gU` must
list exactly the 7 names in `plugin/exported-symbols.txt`. Do not add files to the installed
bundle; a modified bundle needs `codesign --force --sign - "$P"` again.

**Dev-only preset folder.** Nothing ships presets. For testing, make a folder **outside the
repo** with the core's tested presets and their textures side by side (the core looks for named
textures in the preset's own folder), then pick it in the source's "Preset folder" setting:

```sh
PRESETS="<any folder outside the repo>"
mkdir -p "$PRESETS"
cp external/synesthesia-vibe/resources/mvp-presets/*.milk "$PRESETS/"
cp external/synesthesia-vibe/resources/beatdrop/textures/* "$PRESETS/"
```

Never copy presets or textures into the bundle, the pkg or the zip, and never commit them (local
use only, `docs/decisions.md`).

Uninstall a dev install: quit OBS and delete `$P` (plus `$P.dSYM` if a pkg was installed).
User install and uninstall steps, including the pkg receipt, are in
[`plugin/README.md`](../plugin/README.md).

## Packaging

Run from `plugin/`. Always pass `--prefix` to `cmake --install` (see *Dev install*). Never name
the staging folder `release`: on a case-insensitive volume it is Xcode's
`build_macos_arm64/Release/`, and `rm -rf` on it deletes the Release build.

```sh
cmake --preset macos-arm64
grep hostArchitectures build_macos_arm64/distribution
cmake --build --preset macos-arm64 --config Release
rm -rf build_macos_arm64/package-staging
cmake --install build_macos_arm64 --config Release --prefix build_macos_arm64/package-staging
cd build_macos_arm64/package-staging
ditto -c -k --norsrc --noextattr --keepParent synesthesia-vibe-obs.plugin synesthesia-vibe-obs-0.1.0-macos-arm64.zip
```

- **Distribution check.** The `grep` must print `hostArchitectures="arm64"`. The override in
  `plugin/CMakeLists.txt` works only because its `configure_file` runs after the template writes
  the same file inside `set_target_properties_plugin`. A template bump that moves or renames that
  step silently brings back `arm64,x86_64`, and Installer would then accept the pkg on Intel Macs
  where OBS cannot load the bundle.
- **What `cmake --install` makes:** the staged `synesthesia-vibe-obs.plugin`, its
  `synesthesia-vibe-obs.plugin.dSYM` (Release only), and an unsigned `synesthesia-vibe-obs.pkg`
  (`pkgbuild` + `productbuild` from the template's generated `create-package.cmake`). The pkg
  installs for the current user only (`enable_currentUserHome`), into
  `~/Library/Application Support/obs-studio/plugins/`, with a minimum of macOS 12.0.
- **Payload:** the `.plugin` and the `.plugin.dSYM`, no presets or textures. The bundle holds
  `Contents/Info.plist`, `Contents/MacOS/synesthesia-vibe-obs`, `Contents/_CodeSignature/` and
  `Contents/Resources/locale/en-US.ini`.
- **Receipt id:** the template passes `--identifier '@MACOS_BUNDLEID@'` and
  `--version '@CMAKE_PROJECT_VERSION@'` through `execute_process`, which uses no shell, so the
  receipt id is `'io.github.lee5783.synesthesia-vibe-obs'` and the version `'0.1.0'`, single
  quotes included. `plugin/README.md` gives the matching `pkgutil --forget` command.
- **Zip:** `--norsrc --noextattr` keeps `._*` entries and extended attributes out; the zip holds
  only the bundle.
- **Names:** the Installer title and `CFBundleDisplayName` are the project name
  (`synesthesia-vibe-obs`). `CFBundleVersion` is the local build counter and goes up on every
  configure.

**Inspection** (the regression check; there is no automated one):

```sh
pkgutil --payload-files synesthesia-vibe-obs.pkg
pkgutil --check-signature synesthesia-vibe-obs.pkg
codesign -dvv synesthesia-vibe-obs.plugin
codesign --verify --deep --strict --verbose=2 synesthesia-vibe-obs.plugin
lipo -info synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
nm -gU synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
spctl --assess -vv --type install synesthesia-vibe-obs.pkg
```

Expected while unnotarized: `Status: no signature` for the pkg; `Signature=adhoc`,
`flags=0x10002(adhoc,runtime)` and a valid deep/strict verify for the bundle; arm64 only; the 7
exports; `spctl` rejects the pkg (`source=no usable signature`). `--payload-files` may list `._*`
entries on a machine that stamps `com.apple.provenance` on build outputs; they merge back into
extended attributes on install (`.claude/common-issues/plugin-build.md`).

A user-style install of the built pkg needs the user's approval:
`installer -pkg synesthesia-vibe-obs.pkg -target CurrentUserHomeDirectory`.

## Signing and notarization

Not run yet: it needs a Developer ID Application and a Developer ID Installer identity and a
notary profile, which the user supplies outside the repo. Credentials never go into the repo or
into a session file. The presets read `CODESIGN_IDENT` / `CODESIGN_TEAM` from the environment;
`xcode.cmake` then signs with that identity, hardened runtime and `--timestamp` for Release. Every
`<…>` is a placeholder.

```sh
xcrun notarytool store-credentials "<NOTARY_PROFILE>" --apple-id "<APPLE_ID>" --team-id "<TEAM_ID>"

export CODESIGN_IDENT="Developer ID Application: <NAME> (<TEAM_ID>)"
export CODESIGN_TEAM="<TEAM_ID>"
cmake --preset macos-arm64 --fresh
cmake --build --preset macos-arm64 --config Release
rm -rf build_macos_arm64/package-staging
cmake --install build_macos_arm64 --config Release --prefix build_macos_arm64/package-staging
cd build_macos_arm64/package-staging

productsign --timestamp --sign "Developer ID Installer: <NAME> (<TEAM_ID>)" \
  synesthesia-vibe-obs.pkg synesthesia-vibe-obs-signed.pkg
xcrun notarytool submit synesthesia-vibe-obs-signed.pkg --keychain-profile "<NOTARY_PROFILE>" --wait
xcrun stapler staple synesthesia-vibe-obs-signed.pkg
spctl --assess -vv --type install synesthesia-vibe-obs-signed.pkg

ditto -c -k --norsrc --noextattr --keepParent synesthesia-vibe-obs.plugin notarize-upload.zip
xcrun notarytool submit notarize-upload.zip --keychain-profile "<NOTARY_PROFILE>" --wait
xcrun stapler staple synesthesia-vibe-obs.plugin
ditto -c -k --norsrc --noextattr --keepParent synesthesia-vibe-obs.plugin synesthesia-vibe-obs-0.1.0-macos-arm64.zip
```

Unverified: whether `stapler` staples a `.plugin` bundle (it does for apps, pkgs and dmgs). If it
refuses, ship the zip unstapled; Gatekeeper then checks the ticket online.

## Environment Configuration

**Required env vars:**
- `VCPKG_ROOT` (plugin configure).

**Optional env vars:**
- `FETCH_DIST_BASE_URL`, `FETCH_DIST_ARCHIVE_URL` (`fetch-dist.sh` test hooks).
- `CODESIGN_IDENT`, `CODESIGN_TEAM` (plugin signing identity for a Developer ID build; unset means
  ad-hoc). Set them in the shell only, never in a committed preset.

**Secrets location:**
- None in the repo. Local obs-websocket credentials, if used for verification, and the notary
  profile / signing identities stay outside the repo and out of every committed file.

## Webhooks & Callbacks

**Incoming:**
- None.

**Outgoing:**
- None.

---

*Integration audit: 2026-11-23*
