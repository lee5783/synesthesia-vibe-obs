# Developer input

## BS-001 — fetch-dist.sh: download, verify, unpack

### Task

Plan task BS-001 from `../plan/output-plan-tasks.md`: `browser-source/fetch-dist.sh [version]`
downloads the web dist zip and `SHA256SUMS.txt` for the pinned core release, verifies the
checksum and atomically installs into `browser-source/dist/` (git-ignored). Non-zero exit and the
old `dist/` untouched on any failure. BS-002 (presets, textures, notices, manifest) is out of scope.

### Skills (using)

- None. The orchestrator listed no extra skills, and no installed skill matches a shell download
  script (`shader-programming`, `cpp-*` do not apply).

### Rules (read)

- Root `CLAUDE.md`: no code comments, standard file header (`#` lines, author `lee5783`, project
  `SynesthesiaVibeOBS`, date `2026-11-23`), kebab-case names, no unit tests, public repo (no
  absolute machine paths, secrets or personal emails), CodeGraph first (index empty, no source yet).
- `.claude/harness/developer.md` (role file, read in full).
- `.claude/COMMON-ISSUES.md` (no relevant entries).
- `../inputs/requirements.md`
- `../plan/output-plan-gate-1-decisions.md` (C-004 to C-020 accept recommended defaults; BS-006 dropped)
- `../plan/output-plan-summary.md` (Web dist API facts, release URLs, zip layout)
- `../plan/output-plan-tasks.md` (shared rules + BS-001 section)
- `../plan/output-plan-concerns.md` (C-004 recommended default: optional argument, single `0.7.0`
  constant in the script, accept `0.7.0` and `v0.7.0`)

### Interpretation notes

- Test hook for the checksum-failure evidence: an optional `FETCH_DIST_BASE_URL` environment
  variable (default the GitHub releases URL). Pointing it at a local `file://` mirror holding a
  tampered zip exercises the mismatch path without editing the committed script.
- "Atomic" install: stage inside `browser-source/` (same filesystem), rename the old `dist/` aside,
  rename the stage into place, then delete the old copy; restore the old copy if the swap fails.

## BS-002 — presets, textures, notices and manifest

### Task

Plan task BS-002 from `../plan/output-plan-tasks.md`: extend `browser-source/fetch-dist.sh` so it
also downloads the pinned tag's source tarball, extracts `resources/mvp-presets/*.milk` into
`dist/resources/presets/`, `resources/beatdrop/textures/*` (no dotfiles) into
`dist/resources/textures/`, `LICENSE` and `NOTICE` into `dist/`, and writes
`dist/resources/manifest.json` in the `ResourceLoader.load()` format. Local serving only. No page,
no URL settings.

### Skills (using)

- None. The orchestrator listed no extra skills; no installed skill matches a shell download
  script.

### Rules (read)

- Root `CLAUDE.md` (no code comments, keep the existing header, no unit tests, public repo: no
  absolute machine paths).
- `.claude/harness/developer.md` (role file).
- `.claude/COMMON-ISSUES.md` (template only, no relevant entries).
- `../plan/output-plan-gate-1-decisions.md` (Pages skipped, presets local only)
- `../plan/output-plan-summary.md` (Web dist API facts, `ResourceLoader` manifest format)
- `../plan/output-plan-tasks.md` (shared rules + BS-002 section)
- `../plan/output-plan-concerns.md` (C-001, C-002, C-010 resolutions)
- `output-BS-001.md` (stage/swap, `FETCH_DIST_BASE_URL` hook, trap cleanup)
- `browser-source/dist/resource-loader.js` (confirmed: categories `presets`/`textures`/`audio`,
  paths relative to the manifest, basename kept, each segment URL-encoded)

### Interpretation notes

- Placement follows the plan's acceptance criteria: presets and textures under `dist/resources/`,
  `LICENSE` and `NOTICE` at `dist/` (the delegation's one-line summary lumps them under
  `resources/`; the plan row is more specific).
- Everything is assembled in the one `.dist-stage.*` directory and swapped in once, so the BS-001
  guarantee (old `dist/` untouched on any failure) covers the tarball steps too.
- Test hook: `FETCH_DIST_ARCHIVE_URL` (default
  `https://github.com/lee5783/synesthesia-vibe/archive/refs/tags`), the counterpart of
  `FETCH_DIST_BASE_URL`, so the bad-tarball path can be exercised against a local `file://`
  mirror without editing the script.
- Archive members are only copied: regular files only (symlinks skipped), no dotfiles, extracted
  into the temp work dir by explicit member path, never executed. The manifest is written by
  `python3 -I` reading the staged directory names.

## LIC-001 — D1: LICENSE file and PLAN.md

### Task

Plan task LIC-001 from `../plan/output-plan-tasks.md`: add a root `LICENSE` holding the canonical
GPL-2.0 text fetched from `https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt` (byte-identical,
no header), and lock D1 in `PLAN.md` (GPL-2.0-or-later; the MIT core keeps its own licence; the
"or later" grant recorded per C-021). `docs/decisions.md` is LIC-002 (document-writer) and the root
`README.md` is out of scope.

### Skills (using)

- None. The orchestrator listed no extra skills; no installed skill matches a licence file and a
  plan-table edit.

### Rules (read)

- Root `CLAUDE.md` (no file header on a licence file, public repo, keep `PLAN.md`'s style).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (D1: GPL-2.0-or-later, LICENSE in this run, PLAN.md
  edit approved).
- `../plan/output-plan-tasks.md` (LIC-001 row and section).
- `../plan/output-plan-concerns.md` (C-015 superseded; C-021 recommended default: "or later" stated
  in the `PLAN.md` D1 row and `docs/decisions.md`, no SPDX lines, no README edit).
- `PLAN.md` (Decisions table, Risks table, status line).

### Interpretation notes

- The status line already reads "Last updated 2026-11-23", so the date needs no change.
- The Risks row "GPL vs the MIT core and vendored code" says "Settle D1 before Phase 2"; per the
  delegation it is updated minimally to reflect that D1 is settled, keeping the NOTICE check.

## BS-003 — index.html + obs-page.js (render path, no settings yet)

### Task

Plan task BS-003 from `../plan/output-plan-tasks.md`: `browser-source/index.html` (full-window
canvas, black body, no on-screen UI) and `browser-source/obs-page.js` (load the dist module from
`./dist`, load `./dist/resources/manifest.json` through `ResourceLoader`, create a
`SynesthesiaCanvas`, load the first preset; every failure to the console). No URL settings
(BS-004), no audio (BS-005), but the module is split into small step functions so both slot in.

### Skills (using)

- None. No GL/WebGL state is touched directly (the dist's `SynesthesiaCanvas` owns the context),
  so `shader-programming` does not apply; no other installed skill matches a static ES-module page.

### Rules (read)

- Root `CLAUDE.md`: plain ES modules, no bundler/npm; no code comments; standard file header
  (HTML comment block in `index.html`, `//` block in `.js`, author `lee5783`, 2026-11-23,
  `SynesthesiaVibeOBS`); kebab-case; no unit tests; public repo.
- `.claude/harness/developer.md` (role file).
- `.claude/COMMON-ISSUES.md` (template only, no relevant entries).
- `../plan/output-plan-gate-1-decisions.md`
- `../plan/output-plan-summary.md` (Web dist API facts: barrel exports, `ResourceLoader`,
  `SynesthesiaCanvas.create` options, MEMFS preset paths, sample integration order)
- `../plan/output-plan-tasks.md` (BS-003 row and section; BS-004/BS-005 sections for the slots)
- `../plan/output-plan-test-cases.md` (TC-007, TC-008, TC-009)
- `../plan/output-plan-concerns.md` (C-009: console-only errors)
- `output-BS-002.md` (manifest layout `presets/<name>.milk`, `textures/<name>`)
- Dist sources consulted for exact behaviour: `synesthesia-vibe.js`, `synesthesia-module.js`,
  `synesthesia-canvas.js` (`create`, `loadPreset`, `#report`), `webgl-context.js`.

### Interpretation notes

- The dist is imported with a dynamic `import()` rather than a static `import` so a missing
  `dist/` is caught and logged with the page's own prefix and the file name, instead of only the
  browser's module-graph failure.
- No `configuration` is passed yet: `maxFps` comes in BS-004 with the `fps` parameter.
- Sibling style: the dist's own JS (4-space indent, double quotes, `//` header).

## BS-004 — URL settings and preset cycling

### Task

Plan task BS-004 from `../plan/output-plan-tasks.md`: extend `browser-source/obs-page.js` with the
URL settings `preset` (one name or a comma-separated list, C-005 matching), `cycle` (default 30 s,
`0` = off, engine timer through `setTransitionConfig` + `onPresetRequest`), `blend` (default 2 s,
clamped 0–10), `fps` (default 60, `0` = uncapped, `configuration.maxFps`) and a parsed `audio`
(default `default`, plus `none`) stored for BS-005. Sequential order, random blend pattern, hard
cuts off (C-016), try the next preset when one fails, one full pass at most. No audio capture.

### Skills (using)

- None. The orchestrator listed no extra skills; the page drives the dist's `SynesthesiaCanvas`
  API and touches no GL state directly, so `shader-programming` does not apply.

### Rules (read)

- Root `CLAUDE.md` (no code comments, keep the existing header, sibling style, no unit tests).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (C-004…C-020 defaults accepted)
- `../plan/output-plan-summary.md` (`setTransitionConfig`, `onPresetRequest`, `setBlendPattern`,
  `maxFps`, `SynesthesiaContext.BLEND_PATTERN_RANDOM`)
- `../plan/output-plan-tasks.md` (BS-004 row and section)
- `../plan/output-plan-test-cases.md` (TC-010…TC-018)
- `../plan/output-plan-concerns.md` (C-005, C-007, C-016)
- `output-BS-003.md` (step-function layout, `stage()`, BS-004 slots)
- Dist sources checked: `synesthesia-canvas.js` (configuration merge, `#frameIntervalFor`,
  transition/request setters reapplied on context restore), `synesthesia-context.js`
  (`DEFAULT_TRANSITION_CONFIG`, request delivery after `render`).

### Interpretation notes

- `blend`: a finite number outside 0–10 is clamped with a warning (`blend=-5` → 0); a non-number
  falls back to 2. `cycle` and `fps`: a non-number or a negative number falls back to the default.
- Name matching normalises case, treats `+` as a space and collapses whitespace, so a raw `+`
  in the URL (decoded to a space by `URLSearchParams`) still matches (C-005 "lenient").
- Several substring matches: the first in sorted order wins, with a warning (C-005 wording; the
  tasks table says "unique substring", C-005 is the accepted default).
- With one resolved preset the engine timer is off (`presetDuration: 0`) whatever `cycle` says.

## SPK-001 — core submodule at v0.7.0

### Task

Plan task SPK-001 from `../plan/output-plan-tasks.md`: add `https://github.com/lee5783/synesthesia-vibe.git`
as a git submodule at `external/synesthesia-vibe/`, check out tag `v0.7.0` inside it and stage the
gitlink (no commit). Confirm no nested submodules and a clean submodule tree. Record the answer in
`PLAN.md` under the matching "Spike first" bullet.

### Skills (using)

- None. The orchestrator listed no extra skills; no installed skill matches a git submodule add
  (`shader-programming`, `cpp-*` do not apply).

### Rules (read)

- Root `CLAUDE.md` (core is never edited here, public repo: no absolute machine paths).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-spike-decisions.md` (S-* and C-021 recommended defaults accepted)
- `../plan/output-plan-gate-1-decisions.md`
- `../plan/output-plan-summary.md` (*Phase 2 spike facts*: no nested submodules at `v0.7.0`)
- `../plan/output-plan-tasks.md` (shared SPK rules + SPK-001 row and section)
- `../plan/output-plan-concerns.md` (S-001…S-014, C-021)

### Interpretation notes

- SPK-001 has no "Spike first" bullet of its own. The answer goes as an "Answer (2026-11-23):"
  sub-bullet under **Building the core**, limited to the submodule pin; the subdirectory question
  stays for SPK-002.
- Only `.gitmodules` and the gitlink are staged (by `git submodule add`); every other working-tree
  change stays unstaged.

## SPK-002 — Q1: core as a CMake subdirectory

### Task

Plan task SPK-002 from `../plan/output-plan-tasks.md`: empirically check whether the core builds
via `add_subdirectory()` from a throwaway parent with desktop tools off (`SYNESTHESIA_VIBE_GL_DEV=ON`,
`SYNESTHESIA_VIBE_BACKEND=gl`, vcpkg toolchain), record what breaks, then prove the fallback: a
separate configure + build + `cmake --install` into a prefix outside the submodule. Answer in
`PLAN.md` under **Building the core**.

### Skills (using)

- None. The orchestrator listed no extra skills; a CMake build probe matches no installed skill
  (`shader-programming`, `cpp-*` do not apply).

### Rules (read)

- Root `CLAUDE.md` (core never edited, public repo: no absolute machine paths in `PLAN.md` or
  session files; use `$HOME/…` or relative paths).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-spike-decisions.md` (S-005: core built separately with
  ExternalProject, plugin links the installed static library)
- `../plan/output-plan-summary.md` (*Phase 2 spike facts*)
- `../plan/output-plan-tasks.md` (SPK-002 row and section)
- `../plan/output-plan-concerns.md` (S-005, S-013, S-014)
- `output-SPK-001.md`

### Interpretation notes

- All build output goes under the git-ignored `build/` (`build/core-subdir-probe/`,
  `build/core-macos-arm64/`, `build/core-install/`); the submodule must stay clean.
- vcpkg manifest mode for the fallback uses the core's own `vcpkg.json` (it is the top-level
  source dir there); `VCPKG_INSTALLED_DIR` is pointed outside the submodule so no
  `vcpkg_installed/` lands in the core tree.
- Time-box (S-014): at most one retry per blocked approach.

## BS-005 — audio input by device label

### Task

Plan task BS-005 from `../plan/output-plan-tasks.md`: audio in `browser-source/obs-page.js` from
the device named by `audio=` (parsed by BS-004). Label lookup via `enumerateDevices` after a
throwaway permission call, `{ deviceId: { exact } }` into `AudioInput.fromMicrophone`; unknown
label warns and falls back to the default input; `audio=none` opens no capture; denied / no device
keeps the visuals running with a console error; unplugged logs, no retry; a suspended
`AudioContext` resumes on the first click/keypress through an invisible one-shot listener.

### Skills (using)

- None. The orchestrator listed no extra skills; Web Audio / getUserMedia in plain JS matches no
  installed skill (`shader-programming` is GL-only, `cpp-*` do not apply).

### Rules (read)

- Root `CLAUDE.md` (no code comments, keep the file header, sibling style, no unit tests).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md`
- `../plan/output-plan-summary.md` (*AudioInput* facts; the wrapper drops the DOMException name)
- `../plan/output-plan-tasks.md` (BS-005 row and section)
- `../plan/output-plan-test-cases.md` (TC-019 … TC-025, TC-029)
- `../plan/output-plan-concerns.md` (C-006, C-008)
- `output-BS-004.md` and `evidence/BS-004/bs-004-cdp-check.mjs`
- `browser-source/dist/audio-input.js` (the wrapper itself, to see where `resume()` is awaited)

### Interpretation notes

- `startAudio` slots in after `await loadNext(0)` in `main()` and never rethrows, so audio
  failures cannot stop rendering.
- The wrapper keeps its `AudioContext` private, so detecting and resuming a suspended context has
  to work from outside the wrapper (see output for the mechanism chosen).

## SPK-003 — minimal plugin/ on the obs-plugintemplate layout

### Task

Plan task SPK-003 from `../plan/output-plan-tasks.md`: a minimal `plugin/` CMake project on the
pinned obs-plugintemplate layout (libobs from the template's buildspec, obs-studio 31.1.1) that
builds the core through `ExternalProject_Add`, links it as an imported static target, and registers
an empty "SynesthesiaVibe Visualizer" source (`synesthesia_vibe_visualizer`). Build arm64, ad-hoc
signed, install into the user plugin folder, load in OBS 33.0.3. No renderer check (SPK-004), no
rendering (SPK-005).

### Skills (using)

- None. `cpp-coding-standards` is manual-only and was not invoked; `shader-programming` does not
  apply (no GL state or shaders touched yet). CMake / libobs module registration matches no
  installed skill.

### Rules (read)

- Root `CLAUDE.md` (C++17 / C for libobs callbacks, no code comments, header with author
  `lee5783`, kebab-case names, no unit tests, public repo hygiene).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-spike-decisions.md` (template buildspec as shipped, 31.1.1, 33.0.3
  fallback only on an Xcode 27 failure; `plugin/` is the real start; S-002/S-004/S-006/S-007
  defaults).
- `../plan/output-plan-summary.md` (*Phase 2 spike facts*).
- `../plan/output-plan-tasks.md` (SPK-003 row and section).
- `../plan/output-plan-concerns.md` (S-001…S-007, S-010, S-012, S-013).
- `output-SPK-002.md` (core build recipe, ExternalProject notes, framework list).
- `PLAN.md` (*Repository layout*, *Building the core*).
- `.claude/COMMON-ISSUES.md` (no relevant entries).

### Interpretation notes

- OBS is driven only through the local obs-websocket credentials, in a `synesthesia-spike` scene
  collection; the user's original collection is restored at the end.

## BS-007 — browser-source/README.md (OBS setup, local serving only)

### Task

Plan task BS-007 from `../plan/output-plan-tasks.md`: write `browser-source/README.md`, the OBS
setup guide for the Browser Source page. Local serving only (no Pages or hosted URL; Pages was
dropped at Gate 1). Cover prerequisites, `fetch-dist.sh`, serving, OBS Browser Source properties,
the `--enable-media-stream` flag per OS, the URL parameter table, `cycle` semantics, loopback
routing, troubleshooting, test hooks, and credits/licences. No root `README.md` edit.

### Skills (using)

- `project-documentation` (README shape guidance; its `templates/OVERVIEW.md` targets the root
  README, so only its structure ideas apply: requirements, getting started, run, licence).

### Rules (read)

- Root `CLAUDE.md` (public repo: no absolute machine paths, secrets or personal data; kebab-case
  names; root README only on request).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md`, including the mid-develop `cycle=` decision (engine
  meaning: presets change every `cycle + blend` seconds).
- `../plan/output-plan-tasks.md` (BS-007 row and section).
- `../plan/output-plan-concerns.md` (C-005 to C-010, C-014 no header on README, C-017 to C-020).
- `output-BS-001.md` to `output-BS-005.md` (README items under each *Known limitations*).
- `browser-source/fetch-dist.sh`, `browser-source/index.html`, `browser-source/obs-page.js`.
- `.claude/COMMON-ISSUES.md` (template only, no relevant entries).

### Interpretation notes

- OBS UI strings are checked against the installed OBS 33.0.3 bundle's `en-US.ini` locale files
  (no OBS launch). Menu placement and CEF behaviour that cannot be checked that way, and the
  BlackHole / VB-Cable UI names, are marked unverified in the README.
- The server command gains `--bind 127.0.0.1` so the presets are not served to the LAN, in line
  with the local-serving-only terms.

## SPK-004 — Spike Q5: renderer check

### Task

Plan task SPK-004 from `../plan/output-plan-tasks.md`: call `gs_get_device_type()` at module
load; on a non-OpenGL renderer, log clearly and register nothing. Record the answer in `PLAN.md`
under the renderer-check spike bullet, and add the pinned `obs-plugintemplate` SHA
`3e7d7ac3b5342cd7d9b88890b9c70b472d1520fc` where `PLAN.md` describes the plugin layout (S-006).

### Skills (using)

- None. The delegation lists no extra skills; the change reads one libobs graphics query and does
  not touch GL state, render targets or shaders, so `shader-programming` does not apply.

### Rules (read)

- Root `CLAUDE.md` (no code comments, sibling style, header on new files, no unit tests, public
  repo).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-spike-decisions.md`, including the mid-develop section (websocket
  auth off; no GUI or accessibility automation; stop and report if the websocket is unavailable).
- `../plan/output-plan-tasks.md` (SPK-004 row and section).
- `../plan/output-plan-concerns.md` (S-006 template SHA in `PLAN.md`; S-008 never switch the
  user's renderer, the non-GL branch is review-verified plus an optional user check).
- `output-SPK-003.md` (plugin layout, preset, build-number guard, ditto install).
- libobs 31.1.1 sources under `plugin/.deps/` (`gs_get_device_type`, `gs_valid`,
  `obs_enter_graphics`, frontend `ResetVideo` before `obs_load_all_modules2`).

### Interpretation notes

- The check runs in `obs_module_load`, because the OBS frontend creates the graphics subsystem
  (`ResetVideo`) before it loads modules. Any value other than `GS_DEVICE_OPENGL`, including -1
  (no graphics context), refuses registration.
- The non-OpenGL branch is exercised only in a scratch build outside the repo with a temporary
  override, never committed; the user's renderer setting is not changed (S-008).

## SPK-005 — Spike Q2: render via sv_render_frame inside video_render

### Task

Plan task SPK-005 from `../plan/output-plan-tasks.md`: in `video_render`, call `sv_render_frame`
into the texrender FBO between `gs_texrender_begin`/`gs_texrender_end` (read the bound draw
framebuffer and pass it as `target_fbo`), then draw the texture with OBS's default effect. Test it
next to image, text and colour sources in the same scene and answer the `PLAN.md` "Rendering into
OBS" question. SPK-005 only: the GL-state audit is SPK-006 and glad isolation is SPK-007. A
minimal save/restore needed for a correct frame may be added and flagged for SPK-006.

### Skills (using)

- `shader-programming` (render targets, GL state, texture orientation).

### Rules (read)

- Root `CLAUDE.md` (C++17, no code comments, sibling style, header on new files, no unit tests,
  one `sv_context` per GL thread with the context current, public repo).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-spike-decisions.md`, including the mid-develop section (websocket
  only, auth off, no GUI automation; stop and report if the websocket is unavailable).
- `../plan/output-plan-summary.md` (Phase 2 spike facts: get-proc via `dlsym` on the OpenGL
  framework, the core does no GL save/restore, the thread rule).
- `../plan/output-plan-tasks.md` (SPK-005 row and section).
- `../plan/output-plan-concerns.md` (S-009: copy the 10 mvp presets and textures into the locally
  installed bundle only, never committed; load Fractopia; cache under
  `obs_module_config_path("cache")`).
- `output-SPK-003.md`, `output-SPK-004.md`.
- `external/synesthesia-vibe/include/synesthesia-vibe/synesthesia-vibe.h`.
- libobs 31.1.1 sources under `plugin/.deps/` (`texture-render.c`, `libobs-opengl/gl-subsystem.c`:
  `set_current_fbo`, `device_draw` program caching).
- `.claude/COMMON-ISSUES.md` (template only, no relevant entries).

### Interpretation notes

- Presets and textures are copied by hand into the installed bundle's `Contents/Resources/`
  (`presets/`, `textures/`) during the local install step, not by CMake, so nothing preset-related
  enters the repo or the build tree. The spike loads Fractopia from `obs_module_file()`.
- First run is without any save/restore, to record what breaks, per the delegation.

## SPK-006 — Spike Q3: GL-state audit and decision

### Task

Plan task SPK-006 from `../plan/output-plan-tasks.md`: log GL state before and after every core
call (`sv_init_gl`/`sv_create`, preset load, `sv_render_frame`, `sv_destroy`), compare it with
what `libobs-opengl` 31.1.1 caches, and decide between a save/restore guard and a separate shared
GL context (recommended default: save/restore). Replace or extend SPK-005's `gl_program_guard`
with a complete, minimal guard, remove the temporary state-diff logging from the final code
(S-012), and answer the `PLAN.md` "GL state" question. SPK-006 only.

### Skills (using)

- `shader-programming` (GL state, render targets, texture units and samplers).

### Rules (read)

- Root `CLAUDE.md` (C++17, no code comments, sibling style, no unit tests, public repo).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-spike-decisions.md`, including the mid-develop section (websocket
  only, auth off, no GUI automation; stop and report if the websocket is unavailable; never change
  the user's OBS profile, canvas or fps settings).
- `../plan/output-plan-tasks.md` (SPK-006 row and section).
- `../plan/output-plan-concerns.md` (S-012: remove the state-diff logging before SPK-009, keep the
  guard; S-014: time-box).
- `../plan/output-plan-summary.md` (*GL calls in the core* census).
- `output-SPK-005.md` (`gl_program_guard`, the program-cache break, flip/sRGB/blend handling,
  reload cycle, spike scene collection contents).
- libobs 31.1.1 under `plugin/.deps/obs-studio-31.1.1/`: `libobs-opengl/gl-subsystem.c`/`.h`,
  `gl-helpers.c`/`.h`, `gl-vertexbuffer.c`, `gl-texture2d.c`, `gl-stagesurf.c`;
  `libobs/graphics/graphics.c` (blend-state cache), `texture-render.c`.
- Core GL call sites under `external/synesthesia-vibe/src/render/gl/` (read only).

### Interpretation notes

- Two diffs are logged: the raw diff right around each core call (what the core changes) and the
  diff around the guard (what OBS sees afterwards). Acceptance is an empty guarded diff for every
  item OBS caches or assumes.
- The guard wraps only the core call itself, so OBS's own texrender begin/end bookkeeping stays
  OBS's.

## BS-008 — OBS end-to-end check (best effort) + user checklist

### Task

Plan task BS-008 from `../plan/output-plan-tasks.md`: a best-effort OBS check on this Mac of the
Browser Source page (built-in mic, no loopback device installed), driven only through the local
obs-websocket in the `synesthesia-spike` scene collection, plus a manual checklist the user runs
for macOS with BlackHole and Windows with VB-Cable. Confirm or correct the README items BS-007
left open. Phase 1 counts as done only after the user reports back. BS-008 only.

### Skills (using)

- None. The delegation names no extra skill and the task touches no GL state, shaders or C/C++
  (it drives OBS and edits a README), so no project skill matches the area.

### Rules (read)

- Root `CLAUDE.md` (public repo: no absolute paths, secrets or personal device names in repo
  files; device labels scrubbed in evidence).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (C-003: agent check + user checklist).
- `../plan/output-plan-gate-1-spike-decisions.md`, including the mid-develop section (websocket
  only, auth off, no GUI/accessibility automation; anything that needs a click is reported back,
  never clicked; credentials referred to only as "local obs-websocket credentials").
- `../plan/output-plan-tasks.md` (BS-008 row and section).
- `../plan/output-plan-test-cases.md` (TC-025, TC-030 to TC-035).
- `output-BS-005.md`, `output-BS-007.md` (README items to confirm or correct).
- `browser-source/README.md`, `browser-source/obs-page.js`.

### Interpretation notes

- Screenshots come from obs-websocket `GetSourceScreenshot` instead of `screencapture`, per the
  delegation (no GUI automation).
- A macOS microphone prompt is never clicked; it is reported for the orchestrator to ask the user.
- The user's OBS is left on the `Untitled` collection and relaunched without extra flags.

## SPK-007 — Spike Q4: glad loader isolation

### Task

Plan task SPK-007 from `../plan/output-plan-tasks.md`: check with `nm`/`otool` whether the core's
`glad_gl*` / `gladLoad*` symbols in the plugin can bind to (or be bound by) `libobs-opengl`'s
loader, then hide everything except the module entry points (`-fvisibility=hidden` plus an
exported-symbols list or `obs_module_*` visibility) and confirm with `nm -gU`. Answer the
`PLAN.md` "Loading GL functions" question, add the Linux note and decide whether upstream ask #3
is needed. SPK-007 only.

### Skills (using)

- None. The delegation names no extra skill, and the task touches only link options and symbol
  tables, not GL state, render targets or shaders, so `shader-programming` does not match.

### Rules (read)

- Root `CLAUDE.md` (no code comments, sibling style, header on new files, no unit tests, public
  repo: no absolute machine paths).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-tasks.md` (SPK-007 row and section).
- `../plan/output-plan-summary.md` (glad facts: glad2 2.0.8 compiled as C++ with `extern "C"`
  symbols, host get-proc through `gladLoadGLUserPtr`; the two-glad-copies risk).
- `../plan/output-plan-concerns.md` (S-011 upstream asks drafted not filed, S-014 time-box).
- `output-SPK-002.md` (core library exports 2,522 global `glad_gl*`), `output-SPK-003.md`
  (plugin exports them globally; template hidden preset does not reach the prebuilt `.a`;
  hardened runtime), `output-SPK-005.md` / `output-SPK-006.md` (dlsym loader on the OpenGL
  framework; install/reload procedure).
- libobs 31.1.1 under `plugin/.deps/obs-studio-31.1.1/`: `libobs/obs-module.c`
  (`load_module_exports`), `libobs/obs-module.h` (`MODULE_EXPORT` entry points),
  `libobs/util/platform-nix.c` (`os_dlopen` flags), `cmake/common/compiler_common.cmake`.

### Interpretation notes

- OBS is not touched: BS-008 is using it. The build goes only into `plugin/build_macos_arm64`
  and the OBS load test is recorded as pending, with the exact commands, for the orchestrator.
- "Module entry points" means the `obs_module_*` names `load_module_exports` looks up (3
  required, 8 optional). The list uses the `_obs_module_*` wildcard so optional entry points added
  later need no list edit; hidden helpers that match the pattern stay hidden.
- macOS only, like the rest of `plugin/CMakeLists.txt` today. Linux gets a note, not code.

## SPK-008 — consolidate the spike in PLAN.md

### Task

Plan task SPK-008 from `../plan/output-plan-tasks.md`: consolidate the spike results in
`PLAN.md` (each "Spike first" question answered, D4 state, Risks, Upstream asks drafted but not
filed) and record time spent against the 2-day box. Orchestrator-assigned extras: first run the
pending OBS load check of the SPK-007 build over the websocket only; last do the S-012 cleanup
(remove only the installed plugin bundle, relaunch OBS plainly, keep the `synesthesia-spike`
collection and `plugin/build_*`). No commit.

### Skills (using)

- None. The delegation names no extra skill. The task edits `PLAN.md` and session artifacts and
  runs an install/load check; it touches no GL state, render targets or shaders, so
  `shader-programming` does not match. `cpp-*` are manual-only.

### Rules (read)

- Root `CLAUDE.md` (keep `PLAN.md` short and plain; public repo: no absolute paths, no secrets).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-spike-decisions.md` (websocket only, no GUI automation; never name
  the credentials in repo files or artifacts).
- `../plan/output-plan-tasks.md` (SPK-008 row and section; shared SPK rules).
- `../plan/output-plan-concerns.md` (S-011 upstream asks drafted not filed, S-012 cleanup, S-014
  time-box).
- `output-SPK-001.md` … `output-SPK-007.md` (all spike handoffs).
- `../session.log` (timestamps for time spent).
- `PLAN.md` (current per-question answers).

### Interpretation notes

- Upstream drafts go to `evidence/SPK-008/spk-008-upstream-asks.md` (the plan said
  `SPK-008-upstream-asks.md`; the orchestrator moved it into the per-task evidence folder).
- "Consolidate" means tighten the existing answers into a coherent "Spike results" state without
  dropping facts; the detailed answers stay in `PLAN.md` under each question.

## REVIEW-FIX — Gate 2 review fix pass

### Task

Fix the code-reviewer's must-fix list in `../review/output-review-findings.md` (B1, B2, S4, and
the `PLAN.md` part of S1), plus the cheap, safe items S2, N1, N3 and N4, then re-verify with a
build, the export check and the hygiene grep. S3 and the other nits stay documented concerns. No
OBS run, no install, no websocket. No commit.

### Skills (using)

- `shader-programming` — loaded as the delegation asked. N1 touches GL state (the active texture
  unit in `gl_state_guard`), which is the skill's area.

### Rules (read)

- Root `CLAUDE.md` (no code comments, don't strip existing ones, sibling style, no unit tests,
  public repo: no absolute paths or secrets).
- `.claude/harness/developer.md` (role file).
- `../review/output-review-findings.md` (full).
- `output-SPK-005.md` … `output-SPK-008.md`, `../plan/output-plan-gate-1-spike-decisions.md`.
- `PLAN.md`, `plugin/src/visualizer-source.cpp`, `plugin/src/plugin-main.cpp`,
  `plugin/CMakeLists.txt`.

### Interpretation notes

- B2: removal, not `LOG_DEBUG`. Nothing else in the code reads the debug callback. The past
  `gl_errors 0` results recorded in `PLAN.md` stay, because they record what the spike measured.
- S1: `PLAN.md` task 4 (`sv_commit_preset` in `video_tick`) has the same hazard as task 3. It
  changed in the same way, and so did the matching Risks row.
- N1: `evidence/SPK-006/SPK-006-obs-log-*.txt` shows `ACTIVE_TEXTURE 33991 -> 33984` (unit 7 to
  unit 0). Unit 7 was the guard's own save loop. Only the `PLAN.md` table row is corrected. The
  upstream-ask text already says "always `GL_TEXTURE0`".

## REVIEW-RECHECK — runtime re-check of the review fixes (Gate 1.5 evidence for items 2, 6, 7)

### Task

Reinstall the current `plugin/build_macos_arm64` bundle into OBS (no rebuild, code frozen),
confirm it loads and renders cleanly with the review fixes (B2 debug line gone, N3/N4 null
guards, N1 active-texture-unit restore), then uninstall it again (S-012). Websocket only, no GUI
automation, no OBS config edits, no commit.

### Skills (using)

- None beyond the role file — the delegation asked for none. No source is edited, so
  `shader-programming` is not needed.

### Rules (read)

- Root `CLAUDE.md` (public repo: scrub paths, user names and device names from anything quoted).
- `.claude/harness/developer.md` (role file).
- `output-REVIEW-FIX.md`, `output-SPK-008.md` (load-check procedure and cleanup),
  `output-SPK-007.md` ("Pending OBS load check" commands, presets/textures copy, ad-hoc signing),
  `../plan/output-plan-gate-1-spike-decisions.md` (websocket only, no GUI automation).

### Interpretation notes

- Reuses the SPK-008 websocket runner from the session scratchpad; local obs-websocket
  credentials are referenced only from the scratchpad, never from repo files.
- Build freshness: the bundle binary (08:36) is newer than both plugin sources (08:35), so it is
  the REVIEW-FIX build.
