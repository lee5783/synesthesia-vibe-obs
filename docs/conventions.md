# Coding Conventions

**Analysis Date:** 2026-11-23

There is no linter or formatter. The closest existing file is the style guide: match it. The rules
in root `CLAUDE.md` (no new code comments, standard file header, kebab-case names, no unit tests
unless asked) apply everywhere.

## Naming Patterns

**Files:**
- kebab-case for every authored file: `obs-page.js`, `fetch-dist.sh`, `visualizer-source.cpp`,
  `exported-symbols.txt`.

**Functions:**
- JavaScript: camelCase verbs (`readSettings`, `resolvePresets`, `createPlaylist`, `startAudio`).
- C++: snake_case, prefixed by the thing they act on (`visualizer_create`, `visualizer_video_render`,
  `render_core_frame`, `renderer_is_opengl`). OBS callbacks are `visualizer_<callback>`.
- Bash: snake_case (`normalise_version`, `verify_checksum`, `swap_dist`), `die` for fatal errors.

**Variables:**
- JavaScript: module constants `UPPER_SNAKE_CASE` at the top of the file (`DEFAULT_CYCLE_SECONDS`,
  `LOG_PREFIX`); locals camelCase.
- C++: `constexpr` constants in snake_case inside the anonymous namespace, grouped at the top of
  the file: setting keys `setting_<key>` (`setting_preset_folder`), group ids `group_<name>`
  (`group_audio`), hotkey names `hotkey_<action>`, limits and defaults (`max_output_size`,
  `default_cycle_seconds`). Units go in the name (`audio_silence_after_ns`, `max_step_seconds`).
  Struct fields snake_case.
- Settings keys stored in scene collections: snake_case strings (`preset_folder`, `cycle_seconds`,
  `audio_mix_track`, `render_scale`). Never rename one; saved collections would lose the value.
- Bash: `readonly UPPER_SNAKE` constants at the top; lowercase locals declared with `local`.

**Types:**
- C++: snake_case structs, classes and enums (`visualizer_source`, `gl_state_guard`,
  `audio_ring`, `enum class preset_command`), as libobs does. A class with resources deletes its
  copy constructor and copy assignment (`gl_state_guard`, `audio_ring`).

## Code Style

**Formatting:**
- JavaScript: 4-space indent, double quotes, semicolons, template literals for messages. Braces
  omitted on single-statement `if`/`for` bodies. A blank line after a block of `const`
  declarations.
- C++: tabs (template style), opening brace of a function on its own line, braces omitted on
  single-statement `if`. `nullptr`, `static_cast`, `std::array`, `std::lock_guard`.
- Bash: 4-space indent, `set -euo pipefail`, quote every expansion.
- CMake: 2-space indent, multi-line argument lists as in `plugin/CMakeLists.txt`.

**Linting:**
- None. Compiler warnings: the plugin's own sources build with no warnings; the core emits about
  105 deprecation warnings on the macOS 26 SDK, so the core is never built with `-Werror`.

## Import Organization

**C++ order** (`plugin/src/visualizer-source.cpp`):
1. The file's own header (`"visualizer-source.hpp"`), then other project headers in quotes
   (`"audio-ring.hpp"`).
2. OBS and core headers in angle brackets, alphabetical (`<media-io/audio-resampler.h>`,
   `<obs-module.h>`, `<plugin-support.h>`, `<synesthesia-vibe/synesthesia-vibe.h>`, `<util/…>`).
3. System headers (`<OpenGL/gl3.h>`, `<dlfcn.h>`).
4. Standard library, alphabetical (`<algorithm>`, `<array>`, `<atomic>`, `<mutex>`, `<string>`).

Blank line between groups.

**JavaScript:**
- No static imports. The dist is loaded with `import(DIST_ENTRY)` inside `stage(...)` so a missing
  dist is a readable error.

**Path Aliases:**
- None.

## Error Handling

**Patterns:**
- JavaScript: wrap each startup step in `stage("what failed", () => …)`; the error message names
  the step and keeps the original as `cause`. Report with `reportError` (`console.error` with the
  `obs-page:` prefix). Non-fatal problems (bad parameter, unmatched name, missing audio) `warn`
  and fall back to a default; they never stop rendering.
- C++: check every libobs/core return; log and degrade (`context` stays null, `video_render`
  returns early; a failed prepare keeps the current preset; a missing audio source means silence
  and a retry every second). Clamp every numeric setting on read (`std::clamp`) to the same range
  the property declares. Use the ternary null fallback for C strings from libobs:
  `path ? path : ""`, `device_name ? device_name : "unavailable"`.
- Bash: `die "message"` → `fetch-dist: message` on stderr, exit 1. Clean up with a `trap … EXIT`.

## Logging

**Framework:** `console` (page), `obs_log` from `plugin-support.h` (plugin).

**Patterns:**
- Page: every line starts with `obs-page:`. Log one summary line of what was understood
  (`obs-page: settings …`) and one line per preset change and audio decision.
- Plugin: `LOG_INFO` for once-per-event lines (module load, source create, preset prepared /
  committed, auto-cycle request, hotkey, audio attach / detach, resize), `LOG_WARNING` for
  failures and the renderer refusal. Nothing per frame at `LOG_INFO`. A condition that repeats
  (audio source not found) logs once and sets a flag (`audio_wait_logged`). Timed steps log their
  milliseconds (`preset committed: … (blend 2.7 s, 1.4 ms)`). Log messages are developer text and
  stay in code, in English; they are not localised.

## Strings and properties

**Every user-visible string goes through `obs_module_text`:** the source name, property labels,
list items, unit suffixes, hints and hotkey descriptions. The text lives in
`plugin/data/locale/en-US.ini`; no UI text is hard-coded in C++.

**Locale key conventions** (`plugin/data/locale/en-US.ini`):
- A setting's label: PascalCase of the setting (`PresetFolder`, `CycleSeconds`, `RenderScale`).
- Sub-items of a setting: `<Label>.<Item>` (`AudioSource.None`, `AudioSource.WholeMix`,
  `AudioSource.Missing`, `RenderScale.Half`, `PresetFolder.Hint`, `Preset.First`).
- Group titles: `Group.<Name>` (`Group.Presets`, `Group.Transitions`, `Group.Audio`,
  `Group.Output`).
- Unit suffixes: `Unit.<Unit>`, with the leading space quoted (`Unit.Seconds=" s"`,
  `Unit.Pixels=" px"`). OBS's ini parser keeps the quoted space. Set them with
  `obs_property_int_set_suffix` / `obs_property_float_set_suffix`.
- Mix tracks: `MixTrack.Track<n>` for n = 1–6, listed from the `mix_track_labels` array.
- Hotkeys: `Hotkey.<Action>` (`Hotkey.NextPreset`). The hotkey *name* (`SynesthesiaVibe.NextPreset`)
  is a stable id saved in OBS's hotkey bindings, not a locale key; never rename it.

**Property layout** (`visualizer_get_properties`):
- Build each group as its own `obs_properties_t`, add it with
  `obs_properties_add_group(…, OBS_GROUP_NORMAL, …)`. Group order: Presets, Transitions, Audio,
  Output. New settings go into the group they belong to.
- Every setting gets a default in `visualizer_get_defaults`, so `GetInputDefaultSettings` lists
  all of them.
- Prefer a list (`OBS_COMBO_TYPE_LIST`) over a free number when only a few values make sense
  (render scale, mix track). For a string list whose stored value may be empty, add an item with
  value `""` (the Preset list's "(first preset)"). OBS writes a list setting when the dialog opens
  if the stored value matches no item, which marks the dialog dirty
  (`.claude/common-issues/verification-tooling.md`).
- Modified callbacks look properties up with `obs_properties_get` on the topmost properties; it
  searches groups.

## Concurrency

- No `sv_*` call outside the render thread except `sv_prepare_preset` (worker),
  `sv_prepared_preset_free` and `sv_set_preset_request_callback` in `create`
  (`docs/architecture.md`, *Threading*).
- State shared with the tick: `std::atomic`. State shared with the worker, hotkeys or `update`:
  a field under `preset_mutex` or `audio_mutex`, copied out in a short `std::lock_guard` scope.
  Never hold a plugin mutex across an `sv_*` call or a free; free handles after the lock scope.
- Callbacks that run on OBS's audio threads write only the ring. They take no plugin mutex.
- The tick never blocks: use `std::unique_lock` with `std::try_to_lock` there.

## Comments

**When to Comment:**
- Do not add comments. The only comment blocks in authored files are the standard header:

  ```
  //
  //  <FileName>.<ext>
  //  SynesthesiaVibeOBS
  //
  //  Created by lee5783 on <yyyy-MM-dd>.
  //
  ```

  `#` lines in CMake, shell, `.txt` and `.ini` files (OBS's locale parser treats `#` as a
  comment); `<!-- -->` in HTML.
- Vendored template files (`plugin/cmake/**`, `plugin/src/plugin-support.*`) get no header and keep
  their own comments.
- Facts that would otherwise be a comment (why a guard exists, a maintenance trap) go into
  `docs/` or `.claude/common-issues/`.

**JSDoc/TSDoc:**
- Not used.

## Function Design

**Size:** small, single-purpose functions; `main()` in `obs-page.js` reads as the startup sequence.

**Parameters:** pass dependencies explicitly (`sv`, `view`, `module`) rather than using globals.
C++ callbacks cast `void *data` to `visualizer_source *` on the first line.

**Return Values:** JavaScript returns `null` for "not found" (`findMatch`) and throws for failures.

## Module Design

**Exports:**
- Page: none; `obs-page.js` runs `main()` on load.
- Plugin: one registration function per source (`visualizer_source_register`, declared in
  `visualizer-source.hpp`); everything else lives in an anonymous namespace.
- Binary exports: only the names in `plugin/exported-symbols.txt`. It must list exactly the
  `obs_module_*` entry points the plugin defines. Listing an undefined one fails the link; a
  wildcard re-exports libobs imports; forgetting a new optional one (`obs_module_post_load`,
  `obs_module_name`, `obs_module_description`, `obs_module_author`) hides it with no warning.

**GL calls into the core:**
- Every `sv_*` call that touches GL goes through `with_core_gl_state([&] { … })`
  (`docs/architecture.md`).

**Barrel Files:**
- Not used.

---

*Convention analysis: 2026-11-23*
