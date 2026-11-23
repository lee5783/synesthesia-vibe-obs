# Plugin build & core submodule

> Part of the known-issues corpus — see [`../COMMON-ISSUES.md`](../COMMON-ISSUES.md) for the
> index. **Analysis date:** 2026-11-23 (obs-plugintemplate `3e7d7ac3`, core `v0.7.0`).

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
- **Known trade-off:** the seed also bypasses the template's `GITHUB_RUN_ID` build number on CI.
  Revisit when CI exists (Phase 4, `docs/concerns.md`).
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
