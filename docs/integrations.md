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

**OBS project PPA (Linux plugin build, `ppa:obsproject/obs-studio`):**
- The only source of libobs on Linux. Its `obs-studio` package bundles the libobs headers and CMake
  config, which the template's `find_package(libobs)` uses. The Linux plugin is therefore built
  against whatever the PPA ships on the build day, not the buildspec's 31.1.1: **32.2.0**
  (`32.2.0-0obsproject1~noble`, libobs API 32.2.0, `libobs.so.30`) when Phase 4 was built. The PPA
  keeps only its current version, so a later rebuild can raise the Linux minimum OBS.
- Ubuntu's own `obs-studio` (30.0.2 on noble) is refused: it is older, and it ships the headers in a
  separate `libobs-dev`. See *Linux build and package* for the 503 behaviour.

**GitHub Actions and Releases:** CI, the release pipeline and the pinned actions are under *CI/CD
& Deployment*.

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
- macOS: obs-plugintemplate's buildspec mechanism downloads the obs-studio 31.1.1 source tag and
  the obs-deps / Qt6 `2025-07-11` prebuilts, sha256-checked against `plugin/buildspec.json`, and
  builds libobs into `plugin/.deps/`. This takes about 3 minutes once.
- Linux: no buildspec download; libobs comes from the PPA package (above).
- vcpkg installs the core's dependencies into the inner build dir (warm binary cache: about 9 s;
  cold: about 93 s on an M1). The manifest also builds `glfw3` and `gtest`, which only the core's
  desktop tools use (upstream ask #1). On Linux the plan has 27 packages: `pthread-stubs` is an
  autotools build, and the X11/xcb ports (`libx11`, `libxcb`, `xproto`, …) are empty there and use
  the system X11 dev packages instead.
- vcpkg itself: CI clones `microsoft/vcpkg` at `5f96cd15fd745122cf27e0524606d6c1efc5fd07`, which is
  the registry baseline in the core's `vcpkg-configuration.json`, so the tool and the baseline
  match. A local checkout at another commit works too, because the baseline is pinned by the core.

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
  - Linux `.deb` install paths: `/usr/lib/x86_64-linux-gnu/obs-plugins/synesthesia-vibe-obs.so` and
    `/usr/share/obs/obs-plugins/synesthesia-vibe-obs/locale/en-US.ini`, for all users.
  - Core shader cache: `obs_module_config_path("cache")`, created at source creation.

**Caching:**
- Runtime: none beyond the core's shader cache.
- Build: vcpkg's binary cache locally; in CI the GitHub Actions caches below (*CI/CD &
  Deployment*, *Caches*).

## Authentication & Identity

**Auth Provider:**
- None. Code signing is ad-hoc (`CODESIGN_IDENT` / `CODESIGN_TEAM` unset), with the hardened
  runtime flag on non-Debug builds (`xcode.cmake`); OBS loads ad-hoc plugins because `OBS.app`
  carries `disable-library-validation`. The pkg is unsigned. Developer ID signing and
  notarization are documented below and wait on credentials the user supplies out of band
  (C-002).
- CI authenticates only with the workflow's own `github.token` (the release job's `gh` call). No
  workflow reads `secrets.*`.

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
  `preset worker stopped (… ms)` on removal, `plugin unloaded`. On Linux only, a
  `windowing platform: X11 (EGL)` / `Wayland (EGL)` / `unknown` line (from
  `obs_get_nix_platform()`) sits between the renderer check and `plugin loaded successfully`;
  macOS logs nothing there. Failure lines:
  `preset folder not set / not found / has no .milk presets`, `preset failed to prepare: …`,
  `preset failed to commit: …`, `audio source not found (…); checking again every second`. Lines
  do not name the source instance. The user-facing list is in `plugin/README.md`, *Reading the
  OBS log*.

## CI/CD & Deployment

**Hosting:**
- None. The Browser Source is served locally; GitHub Pages hosting (D5) is open and skipped for
  now, because the presets are approved for local serving only.

**CI Pipeline:** GitHub Actions, with project-owned workflows in `.github/workflows/`. They follow
the job layout of obs-plugintemplate's `build-project.yaml` at `3e7d7ac3` but are rooted at
`plugin/`, build macOS arm64 only, use `v*` tags, and have no Windows, format-check or codesign
steps.

| Workflow | Triggers | What it does |
|---|---|---|
| `build-plugin.yaml` (*Build Plugin*) | `workflow_call` only | Reusable. Input `config` (`RelWithDebInfo` by default, or `Release`; anything else fails). Jobs `macOS arm64` and `Linux x86_64`, 90-minute timeout each |
| `ci.yaml` (*CI*) | `pull_request` (any base), `push` to `master`, `workflow_dispatch` | Calls `build-plugin.yaml` with `RelWithDebInfo`. `concurrency` per PR (or ref) with `cancel-in-progress`. `workflow_dispatch` works only once the file is on the default branch |
| `release.yaml` (*Release*) | `push` of `v*` tags | Validates the tag, calls `build-plugin.yaml` with `Release`, creates a draft release (*Releases* below) |

- **macOS job:**
  - Runs on `macos-26` (arm64) and selects `/Applications/Xcode_26.6.app` with `xcode-select`. The
    runner-images policy drops older Xcode patch versions, so bump that path when the image does.
  - Steps: checkout with submodules; *Set up environment*; restore caches; vcpkg; then
    `cmake --preset macos-arm64` and `cmake --build --preset macos-arm64 --config "$CONFIG"`.
  - Then it packages exactly as *Packaging*, renaming the pkg to `<name>-<version>-macos-arm64.pkg`
    next to `<name>-<version>-macos-arm64.zip`, and runs *Inspect package*.
  - It uploads the artifact `<name>-<version>-macos-arm64` (pkg + zip).
- **Linux job:**
  - Runs on `ubuntu-24.04` and installs its packages (*Linux build and package*).
  - *Record libobs version* writes the PPA package and libobs API version to the log and to the
    run summary under *Linux libobs*.
  - Builds with `cmake --preset ubuntu-ci-x86_64 -DCMAKE_INSTALL_PREFIX=/usr
    -DCMAKE_BUILD_TYPE=$CONFIG` and `cmake --build --preset ubuntu-ci-x86_64`.
  - Then *Check plugin binary*, `cmake --build build_x86_64 --target package` and
    *Inspect package*.
  - It uploads the artifact `<name>-<version>-x86_64-linux-gnu` (the `.deb` only; the `-dbgsym.ddeb`
    stays on the runner).
- **Checks:** every check prints an `::error::` and fails the job. The full list is in
  `docs/testing.md`, *CI checks*.
- **Set up environment:**
  - Validates the buildspec `name` against `^[a-z0-9-]+$` and `version` against
    `^[0-9]+\.[0-9]+\.[0-9]+$` before anything is written.
  - Exports `VCPKG_ROOT=$RUNNER_TEMP/vcpkg`, `VCPKG_DEFAULT_BINARY_CACHE=$RUNNER_TEMP/vcpkg-archives`
    and `ASSET_BASE=<name>-<version>`.
  - Outputs the core gitlink SHA, which the cache keys use.
- **Build number:** `plugin/CMakeLists.txt` skips its `.CMakeBuildNumber` seed when `CI` is
  truthy, so the template's `buildnumber.cmake` uses `GITHUB_RUN_ID`. The macOS job asserts
  `CFBundleVersion` equals the run id.
- **Security posture:**
  - Every workflow has top-level `permissions: contents: read`; only the `release` job gets
    `contents: write`.
  - No `secrets.*`.
  - Every checkout sets `persist-credentials: false`.
  - The PR trigger is `pull_request`, never `pull_request_target`, so fork PRs get a read-only
    token and no secrets.
  - No `${{ }}` expression appears inside a `run:` script: `github.ref_name`, `inputs.config` and
    `vars.RELEASE_FINAL_ALLOWED` reach the shell through `env:`.
- **Timings** (first runs, 2026-11-23):
  - macOS: 5m39s cold (libobs build plus vcpkg), 1m59s warm.
  - Linux: 11m38s with cold vcpkg. A warm run took about 14.5 min, mostly PPA 503 retries; Linux
    time depends on Launchpad more than on the build.

**Pinned actions:** every `uses:` names a full 40-character commit SHA with no trailing version
comment (the no-comments rule, C-019). This table is the only SHA-to-version map; update it in the
same change as a bump. There is no Dependabot.

| Pinned item | Version | SHA |
|---|---|---|
| `actions/checkout` | v7.0.1 | `3d3c42e5aac5ba805825da76410c181273ba90b1` |
| `actions/cache/restore`, `actions/cache/save` | v6.1.0 | `55cc8345863c7cc4c66a329aec7e433d2d1c52a9` |
| `actions/upload-artifact` | v7.0.2 | `cf430e030ddbb5b0abf93d22962f4752f3646cd9` |
| `actions/download-artifact` | v8.0.2 | `9000827ccba6bdab643e8b6fd33ac0654aef8333` |
| `microsoft/vcpkg` (`VCPKG_COMMIT` in `build-plugin.yaml`, fetched with `git`) | 2026-09-20, the core's registry baseline | `5f96cd15fd745122cf27e0524606d6c1efc5fd07` |

To bump an action:
- Resolve the new tag to its commit with `gh api repos/<owner>/<repo>/commits/<tag> --jq .sha`;
  that also peels annotated tags.
- Replace every occurrence in `.github/workflows/`, then update the row above.
- Local reusable calls use `./.github/workflows/…` and need no pin.

**Caches** (`actions/cache/restore` and `actions/cache/save`, both jobs):
- **`plugin/.deps`** (macOS only, holds the libobs build):
  - Key: `obs-deps-macos-arm64-xcode<version>-<hash of plugin/buildspec.json>`.
  - Saved right after configure on a miss, because libobs is built during configure.
- **vcpkg binary archives** (`$RUNNER_TEMP/vcpkg-archives`):
  - Save key: `vcpkg-<os>-<triplet>-<vcpkg commit>-<core gitlink>-<run_id>-<run_attempt>`.
  - Restore prefixes: `…-<core gitlink>-`, then `…-<vcpkg commit>-`. The newest match wins.
  - *Count restored vcpkg archives* and *Check vcpkg binary cache* save only when the archive
    count went up. They also run after a failed build (`!cancelled()`), so fix-forward runs stay
    warm, and a warm run with nothing new adds no entry.
  - A fixed key would block every later save after one partial entry; the per-run suffix
    avoids that.
- **Scope:**
  - GitHub scopes caches by ref. A PR reads its own entries and the default branch's, and a
    `master` push seeds entries for later PRs.
  - A tag run reads only its own ref and the default branch. So a release builds vcpkg cold until
    `master` has an entry, and a PR's cache can never feed a release (checked on the test release
    run: `Cache not found` for keys PR #3 held).
  - Deleting a test tag leaves its tag-scoped entries (about 455 MiB for one run). They expire
    after 7 days unused; `gh cache list` and `gh cache delete <id>` remove them sooner.

**Releases** (`release.yaml`):
- **Jobs:** `validate` (*Validate tag*) → `build` (`build-plugin.yaml` with `Release`) →
  `release` (*Create draft release*). Nothing builds until the tag is valid.
- **Tag rule (C-016 b, C-017):**
  - `plugin/buildspec.json` `version` must be plain `X.Y.Z`. The prerelease suffix lives only in
    the tag, so the version bump is a manual buildspec commit before tagging.
  - The tag must be `v<X.Y.Z>`, `v<X.Y.Z>-beta.<N>` or `v<X.Y.Z>-rc.<N>`, with `X.Y.Z` equal to the
    buildspec version. Any other shape, for example `-ci.1`, is refused.
  - A suffix makes a **prerelease**. A plain `vX.Y.Z` is refused unless the repository variable
    **`RELEASE_FINAL_ALLOWED`** is `true` (Settings → Secrets and variables → Actions →
    *Variables*; a variable, not a secret).
  - The variable is unset today, so only prereleases are possible while Phase 2 is open. The first
    real tag is `v0.1.0-beta.1`.
  - Each refusal names the problem: the wrong shape, both versions on a mismatch, or the variable
    for a final tag.
- **Core check:**
  - `CORE_REPOSITORY` / `CORE_VERSION` (`lee5783/synesthesia-vibe`, `v0.7.0`) are workflow `env`
    values. *Validate core version* compares that tag's commit (`git ls-remote`) with the
    submodule gitlink.
  - A core bump must update `CORE_VERSION` in the same change, or the release fails before
    building.
- **Release job:**
  - Downloads the macOS artifact.
  - Writes `SHA256SUMS.txt` in `sha256sum` format and verifies it with `sha256sum -c`.
  - Writes the notes, then runs `gh release create <tag> --draft --verify-tag [--prerelease]` with
    the title `SynesthesiaVibe Visualizer <tag without v>`.
  - Assets: `synesthesia-vibe-obs-<X.Y.Z>-macos-arm64.pkg`, `…-macos-arm64.zip` and
    `SHA256SUMS.txt`.
- **Always a draft.** A person reviews it and clicks *Publish*; that click is the final gate.
  Signing is ad-hoc only.
- **No Linux asset (C-006):**
  - The release run still builds and checks Linux, and its `.deb` stays a workflow artifact. It is
    not downloaded or attached until the user's Ubuntu X11/Wayland test (P4-008) passes.
  - To attach it then:
    - add a `download-artifact` step for `<name>-<version>-x86_64-linux-gnu`;
    - add the `.deb` to the `sha256sum` and `gh release create` lists;
    - rewrite the notes' *Linux* paragraph;
    - update `plugin/README.md` (*Download*, *Linux*).
- **Notes**, generated in the job:
  - the downloads, with `shasum -a 256 --ignore-missing -c SHA256SUMS.txt`;
  - the ad-hoc / unnotarized warning, linking `plugin/README.md` at the tag;
  - the Linux status;
  - GPL-2.0-or-later plus the MIT core;
  - links to this tag's tree and to the core tag's tree;
  - a line saying GitHub's "Source code" archives leave out the submodule (C-026).
- **Re-running** for a tag that already has a release fails at `gh release create`. Delete the
  draft first.
- **Removing a test release:** `gh release delete <tag> --yes`, then
  `git push --delete origin <tag>`. The pipeline was checked once this way with the throwaway tag
  `v0.1.0-beta.0`: all four jobs green, the checksums `OK`, no `.deb`; then the draft and tag were
  deleted.
- User-facing download and checksum steps: `plugin/README.md`, *Download*. No release has been
  published yet, and the pkg install into OBS is not verified end to end
  (`docs/concerns.md`, *Open checks*).

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

The configure stops at once, before the template bootstrap or the libobs download, unless one of
exactly two combinations holds:
- **macOS:** host Darwin, generator `Xcode`, and `CMAKE_OSX_ARCHITECTURES` exactly `arm64` from
  the cache (preset or `-D`; the environment variable is applied too late and is refused).
- **Linux:** host Linux, generator exactly `Ninja` (Ninja Multi-Config is refused), and host
  processor `x86_64`. The processor comes from `cmake_host_system_information(QUERY OS_PLATFORM)`,
  because `CMAKE_HOST_SYSTEM_PROCESSOR` is still empty before `project()`.

So on macOS it refuses `-G Ninja`, universal or `x86_64` builds and the template's `macos`,
`macos-ci` and `windows-*` presets. On Linux it refuses every other generator and every other
host processor. Every refusal prints one message:

```
SynesthesiaVibe for OBS builds only on macOS, with the Xcode generator, for arm64 alone, or on
Linux, with the Ninja generator, for x86_64. Detected host '…' (…), generator '…',
CMAKE_OSX_ARCHITECTURES '…'. On macOS configure with 'cmake --preset macos-arm64' and build with
'cmake --build --preset macos-arm64'. On Linux configure with 'cmake --preset ubuntu-x86_64' and
build with 'cmake --build --preset ubuntu-x86_64'.
```

A refused preset configure leaves a stub `plugin/build_macos/`; delete it. On Linux the guard
checks the host, not the target, so a cross-compiling toolchain file on an x86_64 host would pass
it (`docs/concerns.md`).

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

## Linux build and package

Experimental: built and checked in CI, never run in OBS (`docs/concerns.md`). Ubuntu 24.04
x86_64 only; Flatpak OBS cannot see a `/usr` install and is unsupported.

Prerequisites:
- OBS from `ppa:obsproject/obs-studio`, CMake ≥ 3.28, a vcpkg checkout in `VCPKG_ROOT`, and the
  submodule fetched.
- The apt list of the CI job, plus `cmake` and `git`. `plugin/README.md`, *Build it yourself*,
  has the same list as one command. What each group is for:
  - `obs-studio` from the PPA: libobs, its headers and its CMake config.
  - `libgl-dev`, `libegl-dev`, `libopengl-dev`, `libgles2-mesa-dev`: GLVND GL and EGL.
  - `libsimde-dev`: the template.
  - `build-essential`, `ninja-build`, `pkg-config`.
  - `autoconf`, `autoconf-archive`, `automake`, `libtool`, `libltdl-dev`, `gettext`, `make`:
    vcpkg's `pthread-stubs` autoreconf.
  - `bison`, `flex`, `gperf`, `python3`, `libxml2-utils`, `xsltproc`: the tools the xcb/X11 ports
    name.
  - `xorg-dev`, `libxinerama-dev`, `libxcursor-dev`, `libglu1-mesa-dev`, `libxkbcommon-dev`: the
    `glfw3` port, built against system X11 without its `wayland` feature.
  - `curl`, `zip`, `unzip`, `tar`: vcpkg itself.

Run from `plugin/`:

```sh
cmake --preset ubuntu-x86_64 -DCMAKE_INSTALL_PREFIX=/usr
cmake --build --preset ubuntu-x86_64
cmake --build build_x86_64 --target package
```

- **Output:**
  - `plugin/build_x86_64/synesthesia-vibe-obs.so`;
  - the CPack package `plugin/release/synesthesia-vibe-obs-<version>-x86_64-linux-gnu.deb` plus
    a `-dbgsym.ddeb` (both git-ignored).
  - `-DCMAKE_INSTALL_PREFIX=/usr` puts the package files under
    `/usr/lib/x86_64-linux-gnu/obs-plugins/` and `/usr/share/obs/obs-plugins/<name>/`.
- **Presets:**
  - CI runs `ubuntu-ci-x86_64`, which inherits `ubuntu-x86_64` and adds warnings-as-errors and
    ccache.
  - The plain `ubuntu-x86_64` preset has not been run end to end.
- **What the Linux branch of `plugin/CMakeLists.txt` does:**
  - The core ExternalProject gets triplet `x64-linux`, `-DCMAKE_POSITION_INDEPENDENT_CODE=ON` (the
    core `FORCE`-clears `CMAKE_*_FLAGS`, but not this variable) and `-DCMAKE_INSTALL_LIBDIR=lib`.
  - The core's installed `libprojectM_ns-eel2.a` and `libprojectM_eval.a` become the imported
    targets `SynesthesiaVibe::NsEel2` and `SynesthesiaVibe::ProjectMEval`. They are linked in one
    `$<LINK_GROUP:RESCAN,…>` with `SynesthesiaVibe::Core`; the reason is in `docs/architecture.md`,
    *Core link on Linux*.
  - It adds `Threads::Threads`, `${CMAKE_DL_LIBS}`, `OpenGL::OpenGL`, `OpenGL::EGL`, and the
    definitions `GL_GLEXT_PROTOTYPES` and `EGL_NO_X11`.
  - Link options: `-Wl,--version-script=exported-symbols.map`, `-Wl,--exclude-libs,ALL` and
    `-Wl,--no-undefined`. The map is a `LINK_DEPENDS`, so editing it relinks.
  - `CPACK_PACKAGE_CONTACT` is `lee5783 <lee5783@users.noreply.github.com>`, set before
    `include(defaults)`, because the buildspec `email` is empty (C-008).
- **Package:**
  - `Maintainer` is that contact.
  - `Depends` (from `dpkg-shlibdeps`) is `libc6 (>= 2.38), libegl1, libgcc-s1 (>= 3.0), libopengl0,
    libstdc++6 (>= 13.1), obs-studio`. `obs-studio` is **unversioned**, so the package does not
    enforce the 32.2.0 floor.
  - `NEEDED` is only `libobs.so.30`, `libOpenGL.so.0`, `libEGL.so.1` and the C/C++ runtime.

**The PPA and its 503s.**
- Launchpad's PPA CDN returns `503 Service Unavailable` on and off, for both index and package
  files.
- apt only *warns* about a failed index ("Some index files failed to download"). Without a guard,
  the install quietly takes Ubuntu's own `obs-studio` 30.0.2, which has no `obs-config.h`. The
  build then fails at `find_package(libobs)` or, worse, links against a different OBS.
- The Linux job's *Install build dependencies* step guards against that:
  - `Acquire::Retries "5"` for every apt fetch;
  - `add-apt-repository --no-update`, then `apt-get update --error-on=any`, up to 5 attempts
    (waits of 20, 40, 60 and 80 s);
  - the `obs-studio` candidate must contain `obsproject`, or the job fails with "refusing to build
    against the distro OBS";
  - `apt-get install` up to 3 attempts (waits of 30 and 60 s).
- *Record libobs version* checks again after the install: the installed version must contain
  `obsproject`, and the header is found through `dpkg-query -S '*/obs-config.h'`, not a fixed
  path.
- A long outage still fails the job, with a message naming the PPA; re-run it.
- Never fall back to Ubuntu's `libobs-dev`: that would quietly change the Linux OBS floor
  (`.claude/common-issues/plugin-build.md`).

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
  configure; in CI it is the run id (`GITHUB_RUN_ID`).
- **Release asset names:** CI renames the pkg to `synesthesia-vibe-obs-<version>-macos-arm64.pkg`
  and names the zip `synesthesia-vibe-obs-<version>-macos-arm64.zip`, with the buildspec version
  and no prerelease suffix.

**Inspection** (CI runs the same checks in *Inspect package* on every build, `docs/testing.md`;
run them by hand for a local package):

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

**In CI (C-018).** Built, not yet run with real credentials.
- `build-plugin.yaml` takes a `sign` input, false by default. `ci.yaml` never sets it, so PR and
  `master` builds stay ad-hoc and read no secret. `release.yaml` sets it when the Actions variable
  `MACOS_NOTARIZE` is `true`, and calls the build with `secrets: inherit`. Without that, the
  called workflow sees no secrets at all, even environment secrets of a job that names the
  environment (first signed tag run: the secrets arrived empty).
- With `sign` on, the macOS job runs in the GitHub Environment `macos-signing` and reads its
  secrets from there:
  - `MACOS_SIGNING_CERT`: a base64 `.p12` holding the Developer ID Application and the Developer
    ID Installer certificates with their private keys.
  - `MACOS_SIGNING_CERT_PASSWORD`: the `.p12` export password.
  - `MACOS_NOTARIZATION_USERNAME`: the Apple ID; `MACOS_NOTARIZATION_PASSWORD`: an app-specific
    password for it.
- *Import signing certificate*, before *Configure*:
  - imports the `.p12` into a temporary keychain with a random password;
  - requires exactly one Developer ID Application and one Installer identity sharing one team ID;
  - exports `CODESIGN_IDENT`, `CODESIGN_TEAM` and `INSTALLER_IDENT`. The presets read the first
    two, so Xcode signs with the hardened runtime and `--timestamp`.
- *Sign and notarize*, between *Package* (`cmake --install`) and *Archive*:
  1. `productsign` the pkg;
  2. `notarytool submit --wait` (on failure it prints `notarytool log` and stops);
  3. `stapler staple` the pkg;
  4. try to staple the `.plugin`. If that fails it only warns, and Gatekeeper checks the zip's
     bundle online.
- *Inspect package*:
  - checks for a Developer ID Application authority, the team ID, a timestamp and the runtime
    flag;
  - requires `spctl --assess --type install` to report `source=Notarized Developer ID`;
  - runs `stapler validate` on the pkg.
- *Remove signing keychain* always runs at the end.
- The environment should allow only `v*` tags (*Deployment branches and tags*), so no branch or
  PR run can read the secrets.
- The release notes switch to the "signed and notarized" wording when `MACOS_NOTARIZE` is `true`.

## Environment Configuration

**Required env vars:**
- `VCPKG_ROOT` (plugin configure).

**Optional env vars:**
- `FETCH_DIST_BASE_URL`, `FETCH_DIST_ARCHIVE_URL` (`fetch-dist.sh` test hooks).
- `CODESIGN_IDENT`, `CODESIGN_TEAM` (plugin signing identity for a Developer ID build; unset means
  ad-hoc). Set them in the shell only, never in a committed preset.
- `CI` (truthy on GitHub runners): `plugin/CMakeLists.txt` then skips the build-number seed and
  the template numbers the build with `GITHUB_RUN_ID`.

**Repository settings (GitHub):**
- Actions variable `RELEASE_FINAL_ALLOWED`: `true` lets a plain `vX.Y.Z` tag make a (draft,
  non-prerelease) release. Unset today.
- Actions variable `MACOS_NOTARIZE`: `true` makes tag builds sign with Developer ID and notarize.
  Unset today.
- Environment `macos-signing`: holds the four signing secrets (*Signing and notarization*).
  Restrict it to `v*` tags.

**Secrets location:**
- None in the repo. Only the macOS job of a signed tag build reads secrets, from the
  `macos-signing` environment. Local obs-websocket credentials, if used
  for verification, and the notary profile / signing identities stay outside the repo and out of
  every committed file.

## Webhooks & Callbacks

**Incoming:**
- None.

**Outgoing:**
- None.

---

*Integration audit: 2026-11-23*
