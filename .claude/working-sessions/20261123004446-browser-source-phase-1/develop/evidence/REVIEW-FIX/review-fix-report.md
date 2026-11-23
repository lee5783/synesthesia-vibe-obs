# REVIEW-FIX: verification report

Date: 2026-11-23. Host: macOS arm64, Xcode generator via the `macos-arm64` preset. No OBS run.
The plugin bundle has not been installed since SPK-008.

## Build

```
cd plugin && cmake --preset macos-arm64 && cmake --build --preset macos-arm64
```

- Exit 0, `** BUILD SUCCEEDED **`.
- `CompileC` ran for `src/plugin-main.cpp` and `src/visualizer-source.cpp`.
- **0** `warning:` lines in the whole build log, so none from `plugin/src`.
- With `BUILD_ALWAYS TRUE`, the core step now runs on every build:
  - Xcode notes that the `synesthesia-vibe-core` build and install script phases "will be run
    during every build".
  - The inner Ninja printed `ninja: no work to do.`
  - The install step printed only `-- Up-to-date:` lines.
  - So a build with unchanged core sources costs a Ninja no-op plus an install check.

## Exports

```
nm -gU build_macos_arm64/rundir/RelWithDebInfo/synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
00000000000037e4 T _obs_module_free_locale
0000000000003788 T _obs_module_get_string
000000000000380c T _obs_module_load
000000000000379c T _obs_module_set_locale
0000000000003730 T _obs_module_set_pointer
00000000000038b0 T _obs_module_unload
0000000000003748 T _obs_module_ver
```

That is exactly 7 `obs_module_*` exports, the same set as SPK-007.

## Debug line gone (B2)

- `strings <binary> | grep -c '^frame %u'` returns `0`. The plugin's 10 s format string is gone.
- One `gl_errors` string remains: `synesthesia-vibe: GL pipeline destroyed gl_errors=%d`. It comes
  from the core's own logging in the static library, not from the plugin.
- `strings` still shows `renderer check passed (%s, device type %d)`.

## Hygiene grep (B1)

- File set: `git ls-files -o --exclude-standard` plus tracked and staged changes, without
  `.vscode/`. That is 85 files.
- Patterns: `/private/tmp`, `/Users/`, `claude-501`, `scratchpad/obs-ws`, `credentials.env`.
- Hits after the fix (none is a real path):

| File | Line | What it is |
|---|---|---|
| `develop/output-SPK-008.md` | 164 | The list of patterns SPK-008 grepped for |
| `review/input-code-review.md` | 59 | The list of patterns the reviewer was asked to grep for |
| `review/output-review-findings.md` | 34 | The reviewer's own pattern list |
| `review/output-review-findings.md` | 255 | A redacted placeholder (`/Users/<name>/…`) describing `.vscode/settings.json` |

- An extra pass for the user name and the local folder name matched only
  `review/output-review-findings.md`. Lines 34, 53, 252 and 255 name the folder in prose or in
  the placeholder. Line 259 is a home-relative path into the user's local folder layout, a home-relative
  fragment. None of these is an absolute path or holds the user name. They are outside this
  task's write list, so they are left for the orchestrator to decide on.
- `develop/output-SPK-006.md:176` now says "in the session scratchpad
  (`spk006/visualizer-source-instrumented.cpp`)".

## Code changes (the files are untracked, so they are listed by hand)

- **`plugin/src/visualizer-source.cpp`**
  - Removed `debug_interval_seconds`, `visualizer_log_debug`, and the
    `sv_set_debug_callback` / `sv_set_debug_interval` block in `visualizer_create` (B2).
  - `gl_state_guard()`: after the unit 0–7 save loop, added
    `glActiveTexture(static_cast<GLenum>(active_texture));` (N1).
  - `visualizer_create`: `obs_get_module_data_path` goes into a `const char *` first, and the
    `std::string` is built as `data_path ? data_path : ""` (N3).
- **`plugin/src/plugin-main.cpp`**: the pass log uses `device_name ? device_name : "unavailable"`,
  the same fallback the refusal branch uses (N4).
- **`plugin/CMakeLists.txt`**: `BUILD_ALWAYS TRUE` added to `ExternalProject_Add` (S2).

## Runtime re-check

- Items 2, 6 and 7 are verified by the build only. No OBS run was made, as instructed.
- Item 7 should not change rendering:
  - The core sets its own texture unit before every sampling bind (`glActiveTexture` in
    `preset_shader_inputs.cpp`, `blend_state.cpp`, `blur_chain.cpp`, `motion_interpolator.cpp`
    and `luminance_limiter.cpp`). It always leaves `GL_TEXTURE0` active.
  - The only effect is on texture binds the core makes before its first `glActiveTexture` in a
    call (for example, uploads in `sv_create` / `sv_load_preset`). These now land on the unit OBS
    had active instead of unit 7.
  - Units 0–7 are all restored by the guard's destructor, so OBS sees no net difference.
