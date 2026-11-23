# Plugin build & core submodule

> Part of the known-issues corpus — see [`../COMMON-ISSUES.md`](../COMMON-ISSUES.md) for the
> index. **Analysis date:** 2026-11-23 (obs-plugintemplate `3e7d7ac3`, core `v0.7.0`, Xcode 27.2,
> macOS 26 SDK; Linux entries: Ubuntu 24.04, GCC 13, GitHub Actions `ubuntu-24.04`).

## First configure fails with `set_target_properties called with incorrect number of arguments`

- **Symptom:** on a fresh clone, `cmake --preset macos-arm64` builds libobs into `plugin/.deps/`,
  then stops with:
  ```
  CMake Error at cmake/macos/helpers.cmake:22 (set_target_properties):
    set_target_properties called with incorrect number of arguments.
  ```
  A second configure succeeds.
- **Root cause:** a template bug. On a non-CI machine with no `plugin/cmake/.CMakeBuildNumber`,
  `plugin/cmake/common/buildnumber.cmake` leaves `PLUGIN_BUILD_NUMBER` empty and writes an empty
  file. The unquoted empty value drops an argument from `set_target_properties` in
  `cmake/macos/helpers.cmake`. On the second run the empty file reads as `0+1`.
- **Fix / workaround:** already handled. `plugin/CMakeLists.txt` seeds `cmake/.CMakeBuildNumber`
  with `0` before the template runs, when the file is missing and no `PLUGIN_BUILD_NUMBER` was
  passed. The vendored template file stays verbatim. If you see this error, check that the guard
  is still at the top of `plugin/CMakeLists.txt`.
- **CI:** the seed is skipped when `CI` is truthy (same test as the template's `if("$ENV{CI}")`),
  so CI builds take the template's `GITHUB_RUN_ID` branch, which never writes an empty value. The
  macOS CI job asserts `CFBundleVersion` equals the run id. Do not cache `plugin/cmake/` in CI: a
  restored `.CMakeBuildNumber` would be incremented instead.
- **How to verify this is still current:** `rm -rf plugin/build_macos_arm64
  plugin/cmake/.CMakeBuildNumber`, then `cmake --preset macos-arm64` must pass on the first try. If
  a newer template fixes `buildnumber.cmake`, the guard can go.

## `git submodule status` shows `v0.5.0-19-g887628f` instead of `v0.7.0`

- **Symptom:** the submodule looks pinned to the wrong release.
- **Root cause:** `v0.7.0` (like `v0.4.0` and `v0.6.0`) is a **lightweight** tag in the core repo.
  `git submodule status` uses plain `git describe`, which only considers annotated tags. The SHA
  `887628f` is the correct `v0.7.0` commit.
- **Fix / workaround:** nothing to fix. Confirm with
  `git -C external/synesthesia-vibe describe --tags` (prints `v0.7.0`). Annotated release tags
  upstream would remove the confusion (not filed).
- **How to verify this is still current:** after a core bump, run both commands; if the new tag is
  annotated, `git submodule status` shows it directly.

## Configure stops with "builds only on macOS, with the Xcode generator, for arm64 alone, or on Linux, with the Ninja generator, for x86_64"

- **Symptom:** `cmake` exits 1 at once with:
  ```
  SynesthesiaVibe for OBS builds only on macOS, with the Xcode generator, for arm64 alone, or on
  Linux, with the Ninja generator, for x86_64. Detected host '…' (…), generator '…',
  CMAKE_OSX_ARCHITECTURES '…'. On macOS configure with 'cmake --preset macos-arm64' and build
  with 'cmake --build --preset macos-arm64'. On Linux configure with 'cmake --preset
  ubuntu-x86_64' and build with 'cmake --build --preset ubuntu-x86_64'.
  ```
- **Root cause:** the guard at the top of `plugin/CMakeLists.txt` allows exactly two combinations.
  - On macOS it refuses `-G Ninja` (which would silently drop the export list), universal or
    `x86_64` builds, no architecture at all, and the template's `macos`, `macos-ci` and
    `windows-*` presets. A `CMAKE_OSX_ARCHITECTURES` set only as an **environment variable** is
    refused too: CMake applies it at `project()`, after the guard.
  - On Linux it refuses every generator but exactly `Ninja` (so Ninja Multi-Config is refused
    too) and any host processor but `x86_64`.
- **Fix / workaround:** on macOS, `cd plugin && cmake --preset macos-arm64`; for a one-off
  configure pass `-G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64`. On Linux, `cmake --preset
  ubuntu-x86_64` (or `-G Ninja`). A refused preset configure leaves a stub `plugin/build_macos/`
  (CMake cache only, git-ignored); delete it.
- **How to verify this is still current:** `cmake --preset macos` must fail with the message, and
  `.CMakeBuildNumber` must be unchanged afterwards (the guard runs before the seed and the
  template bootstrap).

## `rm -rf build_macos_arm64/release` deletes the Release build

- **Symptom:** after cleaning a staging folder named `release` (or `Release`) inside the build dir,
  `build_macos_arm64/Release/synesthesia-vibe-obs.plugin` and its `.dSYM` are gone.
- **Root cause:** APFS volumes are case-insensitive by default, so `release/` and Xcode's
  `Release/` output folder are the same directory.
- **Fix / workaround:** stage into `build_macos_arm64/package-staging` (the name used in
  `docs/integrations.md`, *Packaging*). Rebuild with
  `cmake --build --preset macos-arm64 --config Release` if it already happened.
- **How to verify this is still current:** `diskutil info / | grep -i "case"` shows whether the
  volume is case-sensitive.

## `pkgutil --pkgs` lists the receipt as `'io.github.lee5783.synesthesia-vibe-obs'`, quotes included

- **Symptom:** the installed pkg's receipt id and version carry literal single quotes. A
  `pkgutil --forget io.github.lee5783.synesthesia-vibe-obs` (or the listed name pasted unquoted)
  finds no receipt.
- **Root cause:** a template bug. The vendored `plugin/cmake/macos/resources/create-package.cmake.in`
  passes `--identifier '@MACOS_BUNDLEID@'` and `--version '@CMAKE_PROJECT_VERSION@'` to
  `pkgbuild` through `execute_process`, which runs no shell, so the quotes become part of the
  values. Upgrades still match, because every build has the same quoted id.
- **Fix / workaround:** `pkgutil --forget "'io.github.lee5783.synesthesia-vibe-obs'"` (single quotes
  inside double quotes), as in `plugin/README.md`, *Uninstall*. A real fix is a project-owned
  `create-package.cmake` configured to the same build-dir path (`docs/concerns.md`); it would
  also let the payload drop the `.dSYM`.
- **How to verify this is still current:** `pkgutil --expand synesthesia-vibe-obs.pkg x` and read
  `x/*/PackageInfo` (`identifier=` and `version=`). Not yet checked against a real installed
  receipt (deferred with the pkg install check).

## `pkgutil --payload-files` lists `._*` entries

- **Symptom:** the pkg payload listing shows AppleDouble `._*` entries next to the real files,
  although the zip (made with `ditto --norsrc --noextattr`) has none.
- **Root cause:** some machines stamp a `com.apple.provenance` extended attribute on every file a
  build writes, and it cannot be removed (`xattr -cr`, `ditto --noextattr` and
  `COPYFILE_DISABLE=1` all leave it). `pkgbuild` stores xattrs as AppleDouble sidecars.
- **Fix / workaround:** none needed. Installer merges the sidecars back into xattrs: no `._` files
  land on disk and the signature verifies. `pkgutil --expand-full` reproduces the install-side
  extraction; run `codesign --verify --deep --strict` on the extracted bundle to confirm. A clean
  CI machine would not produce them.
- **How to verify this is still current:** `xattr -l` on a freshly built bundle; if no
  `com.apple.provenance` appears, the listing has no `._` entries either.

## Linux link fails with `undefined reference to 'NSEEL_HOSTSTUB_EnterMutex'`

- **Symptom:** on Linux the core and all plugin sources compile, then the link of
  `synesthesia-vibe-obs.so` fails inside `libprojectM_ns-eel2.a(ns-eel.c.o)` with undefined
  `NSEEL_HOSTSTUB_EnterMutex` / `NSEEL_HOSTSTUB_LeaveMutex`. macOS links fine.
- **Root cause:** GNU ld archive order with a cycle between static archives.
  - The stubs are defined in the core (`external/synesthesia-vibe/src/platform/nseel_hoststub.cpp`,
    inside `libSynesthesiaVibeCore.a`), but only ns-eel2 uses them.
  - ld scans a plain archive list once. The core comes first, nothing needs the stubs yet, so
    their object is skipped, and ld never rescans.
  - On Apple the core merges ns-eel2 and eval into its own archive, and ld64 resolves within it.
- **Fix / workaround:** already handled. `plugin/CMakeLists.txt` links
  `$<LINK_GROUP:RESCAN,SynesthesiaVibe::Core,SynesthesiaVibe::NsEel2,SynesthesiaVibe::ProjectMEval>`
  (`--start-group … --end-group`). Keep the three in one group, and don't "fix" a new undefined
  symbol by reordering; add the archive to the group. Rationale: `docs/architecture.md`, *Core
  link on Linux*.
- **How to verify this is still current:** the CI *Build / Linux x86_64* link step passes with
  `-Wl,--no-undefined`. After a core bump, check whether the core started merging the archives on
  Linux too; then the group is harmless but unnecessary.

## Linux CI fails "refusing to build against the distro OBS", or `obs-config.h` is missing

- **Symptom:** the Linux job fails in *Install build dependencies* with `obs-studio candidate '…'
  is not from ppa:obsproject/obs-studio; refusing to build against the distro OBS`, or with
  `apt-get update failed 5 times` / `apt-get install failed 3 times`. In *Record libobs version*
  it can fail with `installed … is not from ppa:obsproject/obs-studio` or `no installed package
  provides obs-config.h`. Before the guard existed, the step died with
  `awk: fatal: cannot open file '/usr/include/obs/obs-config.h'`.
- **Root cause:** Launchpad's PPA CDN returns `503 Service Unavailable` on and off, for the index
  (`InRelease`) and for package files.
  - apt only **warns** when an index fails ("Some index files failed to download. They have been
    ignored").
  - `apt-get install obs-studio` then quietly takes Ubuntu's own `obs-studio` 30.0.2 from
    noble/universe.
  - That package has no libobs headers or CMake config (Ubuntu ships them in `libobs-dev`), and it
    would set a different Linux OBS floor anyway.
- **Fix / workaround:**
  - Re-run the job; the guard turned a silent wrong build into a clear failure.
  - The step already retries: `Acquire::Retries "5"`, `apt-get update --error-on=any` up to 5
    times, install up to 3 times.
  - Do **not** add a fallback to Ubuntu's `libobs-dev`: it would change the Linux OBS floor
    without anyone noticing (C-009).
  - Locally, `apt-cache policy obs-studio` must show an `obsproject` candidate before you build.
- **How to verify this is still current:** the job's log shows `apt-cache policy obs-studio` and
  the `obs-studio package:` line; a healthy run has `…obsproject…` in both.
