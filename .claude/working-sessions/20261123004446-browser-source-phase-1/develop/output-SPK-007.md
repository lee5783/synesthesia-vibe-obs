# SPK-007

## Subtasks executed
- Read the libobs 31.1.1 module loader: `load_module_exports` needs 3 entry points
  (`obs_module_load`, `obs_module_set_pointer`, `obs_module_ver`) and looks up 8 optional ones.
  `os_dlopen` on macOS uses `RTLD_LAZY | RTLD_FIRST | RTLD_LOCAL`; on Linux it uses `RTLD_LAZY`.
- Ran `nm -m`, `nm -gU`, `otool -hv`, `otool -L` and `dyld_info -exports/-fixups` on the SPK-006
  plugin binary (before) and on OBS 33.0.3's `libobs-opengl.dylib`. Only OBS's files were read;
  OBS itself was not touched.
- Added `plugin/exported-symbols.txt` and wired it into `plugin/CMakeLists.txt` as the Xcode
  `EXPORTED_SYMBOLS_FILE`.
  - First tried the wildcard `_obs_module_*`. It also re-exported 2 libobs imports.
  - Then tried listing all 11 loader names. The link fails on the 4 names the plugin doesn't
    define.
  - Kept the 7 names the plugin defines.
- Rebuilt into `plugin/build_macos_arm64` with the `macos-arm64` preset and re-ran every check on
  the rebuilt binary (after).
- Wrote a scratch `dlopen` probe (session scratchpad, not the repo). It opens the binary with
  OBS's flags, looks up every loader name plus a sample of core symbols, and calls
  `obs_module_ver()`. Ran it on both binaries.
- Wrote the "Loading GL functions" answer, the Linux note, the Risks row and a refined upstream
  ask #3 (not filed) into `PLAN.md`.

## Files changed
- `plugin/exported-symbols.txt` (new, `#` header; ld64 ignores `#` lines)
- `plugin/CMakeLists.txt` (one `set_target_properties` block: `XCODE_ATTRIBUTE_EXPORTED_SYMBOLS_FILE`)
- `PLAN.md`:
  - *Loading GL functions*: the answer and the Linux note.
  - Risks row "Duplicate glad symbols on Linux".
  - Upstream ask #3: marked not needed, with optional draft text.
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md` (SPK-007 section appended)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-SPK-007.md` (this file)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-007/SPK-007-report.md`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-007/SPK-007-symbols.txt`
  (git-ignored full dump, paths scrubbed)
- Generated, git-ignored: `plugin/build_macos_arm64/` (rebuilt bundle).
- Not touched: `external/synesthesia-vibe/` (clean, including ignored files), OBS, the installed
  plugin in the OBS plugins folder, `docs/`.

## What changed and why

**Answer: on macOS the two glad loaders are isolated.** The plugin now exports only its 7
module entry points.

- OBS's glad can't be bound to. `libobs-opengl.dylib` has 103 glad symbols, all
  `non-external (was a private external)`; OBS builds with hidden visibility. Nothing else in
  OBS.app exports a glad symbol.
- The plugin used to export 4,506 symbols, all from the prebuilt core `.a`, because the template's
  `-fvisibility=hidden` doesn't reach a prebuilt archive:
  - 2,522 `glad_gl*`, 631 `GLAD_GL_*` and the 2 `gladLoad*`;
  - the 26 `sv_*`;
  - prjm_eval, NSEEL and stb;
  - 484 weak C++ definitions.
- For glad that was harmless: `TWOLEVEL` binding, `RTLD_LOCAL`, and `glad_` prefixed names that
  differ from the framework `gl*` functions.
- The real leak was the weak definitions (`WEAK_DEFINES`, 492 `weak-def-coalesce` binds).
  dyld coalesces those across images whatever the namespace, so two plugins built from different
  core versions could have silently shared one copy of a core function.
- After the change:
  - `nm -gU` and `dyld_info -exports` show exactly the 7 `obs_module_*`;
  - no glad, `sv_*` or other core symbol is external;
  - `WEAK_DEFINES` is gone, leaving only the libc++ `operator new`/`delete` and typeinfo binds;
  - the binary is 2,285,840 → 1,938,208 bytes, since `-dead_strip` now drops the unused core API.
- GL resolution is unchanged:
  - `otool -L` is identical before and after.
  - The GL and `dlopen`/`dlsym` imports are identical, bound two-level to `OpenGL` and
    `libSystem`.
  - The core's glad is still filled through the plugin's `dlsym` loader on the OpenGL framework
    handle, which hiding the plugin's symbols doesn't affect.

**Linux.** `os_dlopen` is `RTLD_LAZY` (local), and OBS's glad is hidden there too. But ELF
default-visibility symbols can be preempted from the global scope. The plugin should hide the core
at its own link step, with a version script exporting the same 7 names or with
`-Wl,--exclude-libs,ALL`. That needs no core change, so **upstream ask #3 is not needed**. It is
kept as optional hygiene (hidden presets plus an `SV_API` macro in the core), drafted, not filed.

## Verification
- `cd plugin && cmake --preset macos-arm64` → exit 0 (8.4 s).
- `cmake --build --preset macos-arm64`:
  - `** BUILD SUCCEEDED **` twice: once with the wildcard list, once with the final list.
  - The link line carries `-exported_symbols_list .../plugin/exported-symbols.txt`.
  - Xcode relinked when only the list file changed.
- `nm -gU` / `dyld_info -exports` → the 7 entry points only.
- `nm -m | grep -c ' external _glad_gl\| external _GLAD_GL\| external _gladLoad'` → 0.
- `otool -hv` → `TWOLEVEL`, no `WEAK_DEFINES`.
- `codesign --verify --strict` → ok (adhoc, runtime).
- `lipo -info` → arm64.
- `dlopen` probe with `RTLD_LAZY|RTLD_FIRST|RTLD_LOCAL` → all 7 entry points found, every core
  symbol not found, `obs_module_ver() = 0x1f010001` (31.1).
- No existing test suite covers this area.
- **Pending: the OBS load and render check.** BS-008 holds OBS. The exact commands and pass
  criteria are in `evidence/SPK-007/SPK-007-report.md` §9 and below.

## Evidence
- `evidence/SPK-007/SPK-007-report.md` (tracked). It holds trimmed `nm`/`otool`/`dyld_info`
  output for OBS's `libobs-opengl` and for the plugin before and after, the GL import binding,
  the dlopen probe table, the two rejected export-list attempts, codesign, and the pending OBS
  check.
- `evidence/SPK-007/SPK-007-symbols.txt` (git-ignored, 9,355 lines). This is the full dump the
  plan's evidence name asks for, with paths scrubbed.
- This is a build and link change with no UI, so the evidence is tool output. Visual and OBS-log
  evidence waits for the pending OBS check.

## Known limitations / follow-ups
- **The pending OBS check is for the orchestrator to run after BS-008**, from the repo root:
  ```
  osascript -e 'tell application "OBS" to quit'
  P="$HOME/Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin"
  ditto plugin/build_macos_arm64/RelWithDebInfo/synesthesia-vibe-obs.plugin "$P"
  mkdir -p "$P/Contents/Resources/presets" "$P/Contents/Resources/textures"
  cp external/synesthesia-vibe/resources/mvp-presets/*.milk "$P/Contents/Resources/presets/"
  cp external/synesthesia-vibe/resources/beatdrop/textures/* "$P/Contents/Resources/textures/"
  codesign --force --sign - "$P" && codesign --verify "$P"
  nm -gU "$P/Contents/MacOS/synesthesia-vibe-obs"
  open -a OBS
  ```
  Pass criteria in the OBS log:
  - `renderer check passed (OpenGL, device type 1)`;
  - `plugin loaded successfully (version 0.1.0)`, and the module is listed under Loaded Modules;
  - with the visualizer in `synesthesia-spike`: `sv_init_gl and sv_create succeeded (1920x1080)`
    and the periodic `frame N, ... gl_errors 0` line, with the preset animating;
  - `plugin unloaded` on quit.
- **Maintenance trap:** `exported-symbols.txt` must name only defined symbols. If
  `obs_module_post_load`, `obs_module_name`, `obs_module_description` or `obs_module_author` is
  added later, add it to the list. Otherwise it is hidden without any warning and OBS never calls
  it. This is recorded in `PLAN.md`. Code comments are not allowed, so the note can't live in the
  file.
- The setting is macOS/Xcode only, like the rest of `plugin/CMakeLists.txt`, which has no
  `OS_MACOS` guards. Phase 4 Linux needs the version-script equivalent. Windows DLLs export
  nothing by default, so `__declspec(dllexport)` on the `MODULE_EXPORT` names already covers it.
- The installed plugin in the OBS plugins folder is still the SPK-006 build until the pending
  check reinstalls it.
- `PLAN.md` doesn't record SPK-003's template SHA. That is still open from SPK-003, for SPK-008.
- S-014 time-box: SPK-007 took about 7 minutes of agent time (session log 00:57:05Z
  delegation → 01:03:59Z END), one pass, no retry.
- Doc gaps for document-writer (SPK-010):
  - `docs/conventions.md` or `docs/integrations.md`: the export-list rule and the trap above.
  - `docs/concerns.md`: the Linux version-script follow-up.
  - `.claude/how-x-works/` could get a short "how OBS loads a module" page (`os_dlopen` flags,
    `load_module_exports` names).

## Handoff for downstream review
- Acceptance criteria addressed:
  - `nm -m` on the plugin and on `libobs-opengl`;
  - `otool -hv` shows `TWOLEVEL` and no flat namespace;
  - hidden visibility plus an export list, checked with `nm -gU`;
  - the PLAN answer, the Linux note and the upstream ask #3 decision;
  - the evidence file.
- Areas to scrutinize:
  - The `set_target_properties` placement in `plugin/CMakeLists.txt`.
  - That the 7 names match what OBS dlsyms.
  - The `PLAN.md` claims about weak-definition coalescing and Linux ELF preemption.
  - That no absolute machine paths are in repo files. I grepped for them: none.
- How to reproduce: from `plugin/` with `VCPKG_ROOT` set, run `cmake --preset macos-arm64`, then
  `cmake --build --preset macos-arm64`, then
  `nm -gU build_macos_arm64/RelWithDebInfo/synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs`.
