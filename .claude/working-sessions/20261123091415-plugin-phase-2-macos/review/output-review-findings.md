# Review findings (P2-015, Gate 2)

## Decision

**Approve, with two README fixes before the doc pass.** The code has no blocker. The two
should-fix items are user-facing README errors about uninstalling. The deferred OBS checks (P2-011,
P2-012, the P2-010b check) are not code defects. They must appear in the PR as not done. One
measured fact from the run makes P2-012 the riskiest of them (see *Deferred checks*).

## Tooling

- No linter or formatter exists here (`CLAUDE.md`). The sibling file is the style guide.
- `cmake --build --preset macos-arm64` (from `plugin/`, on the current tree): `** BUILD SUCCEEDED **`,
  with no warning or error from `plugin/src`.
- `nm -gU` on the built bundle matches `plugin/exported-symbols.txt` exactly (7 names).
- `build_macos_arm64/distribution` has `hostArchitectures="arm64"`, so the override takes effect.
- `git diff master -- plugin/cmake` is empty, so the vendored template is untouched.
- The diff adds no code comments. New files carry the standard header (`audio-ring.hpp/.cpp`,
  `distribution.in` as an XML comment).
- libobs 31.1.1 facts below were checked in `plugin/.deps/obs-studio-31.1.1`: `obs_source_destroy`,
  `obs_hotkey_*`, `util/task.c` and `properties-view.cpp`.

## Code quality / common issues

### Threading and lifetime: verified

- **`sv_*` thread rule.** The only `sv_*` calls outside the render thread are allowed:
  - `sv_prepare_preset` on the worker (`visualizer-source.cpp:469-470`);
  - `sv_prepared_preset_free` in several places;
  - `sv_set_preset_request_callback` in `create`, before the source can render (`:1141`).

  Every GL call sits inside `with_core_gl_state`: `sv_init_gl`/`sv_create` (`:1128-1132`),
  `sv_commit_preset` (`:874`), `sv_set_transition_config` (`:906`), `sv_resize` (`:951-953`),
  `sv_render_frame` (`:1224-1226`) and `sv_destroy` (`:1169`). `sv_push_pcm_float` runs outside
  the guard. That is correct, because it does not touch GL.
- **Tick.** `video_tick` touches only the two atomics (`:1189-1192`) plus `maintain_audio_source`,
  which takes `audio_mutex` with `try_lock`.
  - The two-exchange note from P2-002 is harmless: at worst a repeat frame, never lost or
    doubled time.
  - For video sources, libobs runs `update` deferred inside `obs_source_video_tick`, so on the
    graphics thread and not the UI thread. Nothing in `update` depends on the thread, so this is
    fine. The doc pass should say so (P2-006 already noted it).
- **Audio callbacks.** `audio_capture` and `mix_capture` (`:660-679`) write only the ring. They
  never call `sv_*` or take one of our mutexes.
  - Attach creates the resampler before `obs_source_add_audio_capture_callback`. Detach removes
    the callback before destroying the resampler (`:715`, `:720`). Both libobs calls take
    `audio_cb_mutex`, so the resampler pointer is published and retired correctly.
  - Lock order is always `audio_mutex` → `audio_cb_mutex` / `input_mutex` / `sources_mutex`. No
    path takes them in reverse.
- **One producer at a time.** `select_audio_source` detaches or disconnects before attaching
  (`:808-815`). The libobs mutexes give the hand-off between producer threads a happens-before
  edge.
- **Worker vs `sv_destroy`.** `os_task_queue_destroy` queues its stop task behind any pending
  tasks and joins. Those tasks see `preset_closing` and return. An in-flight prepare is dropped
  as superseded and freed. `ready_preset` is freed before `sv_destroy` (`:644-658`,
  `:1165-1169`).
- **Handle ownership.** No double free: the replaced or superseded handle is freed outside the
  lock (`:479-495`), and commit takes the slot under the lock and frees after the commit
  (`:855-876`).
- **Hotkeys.** libobs calls hotkey callbacks with the hotkeys mutex held
  (`obs_hotkey_trigger_routed_callback`), and `obs_hotkey_unregister` takes the same mutex. So
  unregistering first in `destroy` (`:1157`) is a real barrier. libobs's own unregister in
  `obs_context_data_free` runs only after `info.destroy`, so the explicit unregister is needed.
- **Latch bookkeeping.** I checked it against the core header (`synesthesia-vibe.h:184-190`: a
  prepare alone keeps the latch, and a failed load, prepare or commit clears it). The plugin
  clears `cycle_requested` at three sites:
  - a non-superseded prepare success (`:492`);
  - a prepare failure (`:475`);
  - the render taking the slot (`:866-867`).

  Each matches a core latch release: a later commit, a failed prepare, or the commit itself. A
  superseded cycle prepare leaves the flag set, and the superseding pick's commit answers the
  core. The pause deferral (`:530-537`) and resume (`:567-579`) agree with this. I found no lost
  wakeup and no deadlock: no lock is held across `sv_render_frame` or `sv_prepare_preset`.

### Ring buffer: verified

- `audio-ring.cpp:43-62` (producer) and `:65-82` (consumer) use the correct SPSC ordering:
  - each side loads its own index relaxed and the other side's with acquire;
  - each side copies the data before its release store.
- Wraparound uses split chunks with the power-of-two mask. Full means `capacity - used`, so the
  producer drops the new frames. The `read_newest` arithmetic (`first = write_at - count`) is
  right.
- Every `video_render` call holds the graphics mutex, including websocket screenshots, so the
  consumer side is serialized. The resampler's `output[0]` is one interleaved plane for
  `AUDIO_FORMAT_FLOAT` stereo.

### GL and draw: verified

- Opaque draw: `OBS_EFFECT_OPAQUE` / `Draw` (`:1253-1261`).
- sRGB is handled as `obs_source_draw` does it.
- Framebuffer sRGB is off during the core render and restored (`:1218-1228`).
- The texrender uses the scaled size (`:1215`), and the sprite is drawn at the source size
  (`:1261`).
- Screenshot `P2-003-after-swarm-nest.png` shows the visualizer opaque over the magenta colour
  source.

### Properties and the P2-010b fix: verified

- `preset_folder_modified` (`:993-1007`) adds "(first preset)" = `""` only when files exist. The
  visibility test now uses `files.empty()`, which is right. A count-based test would have broken
  with the extra item.
- OBS writes on open only when `count && idx == -1` (`properties-view.cpp:684-686`):
  - an empty folder (count 0) writes nothing;
  - a folder with an empty `preset` now matches item 0.
- The worker turns `""` into the first file through `listed_or_first` (`:265-268`, `:455`). No
  other path treats `""` as a file.
- New behaviour when the folder changes in the dialog: OBS writes `""` instead of the first file
  name. The rendered result is the same. The README documents it (`plugin/README.md`, Presets
  bullets).

### Findings

| # | Severity | Location | Finding |
|---|---|---|---|
| S1 | should-fix | `plugin/README.md:62`, `:358-366` | **Uninstall leaves the dSYM behind.** The pkg installs `synesthesia-vibe-obs.plugin.dSYM` next to the bundle in the plugins folder (P2-013 payload listing). The README says the installer puts only the `.plugin` there, and step 2 of Uninstall removes only the `.plugin`. Either add the `.plugin.dSYM` to the text and the `rm -rf` line, or drop the dSYM from the payload with a project-owned `create-package.cmake`, the same override pattern as `distribution.in`. |
| S2 | should-fix | `plugin/README.md:368-373` | **`pkgutil --forget` as written fails.** Because of the template bug (P2-013), the receipt id carries literal single quotes: `'io.github.lee5783.synesthesia-vibe-obs'`. A user who pastes the line that `pkgutil --pkgs` prints gets the quotes stripped by the shell, so `--forget` finds no receipt. Either give the exact command, `pkgutil --forget "'io.github.lee5783.synesthesia-vibe-obs'"`, or fix the id by overriding `create-package.cmake`. The override would also fix the quoted version. |
| N1 | nit | `visualizer-source.cpp:708-723` | **Teardown window when the weak reference is already gone.** If the chosen source's last reference drops on another thread while its audio thread is inside `audio_capture`, `obs_weak_source_get_source` returns NULL. Detach then skips `remove` and destroys the resampler, or `destroy` frees the ring. `obs_source_destroy` frees `audio_cb_list` under `audio_cb_mutex` right after the refcount reaches 0 (`obs-source.c:614-618`), so the window is a few instructions. `compressor-filter.c` has the same window. Acceptable, and P2-004 discloses it. Record it in `docs/concerns.md`. |
| N2 | nit | `visualizer-source.cpp:940-941` | `render_scale` is clamped to 50–100 but not snapped to the list. A value such as 60, set through the websocket, renders at 60 %. Opening the dialog then writes 100 (`count && idx == -1`), which dirties the dialog. `audio_mix_track` has the same issue (noted in P2-010). Snap to 50/75/100, or leave it documented. |
| N3 | nit | `visualizer-source.cpp:265-268`, `:455` | Other ways the dialog still dirties itself on open: a stored `preset` whose file was later deleted or renamed. Because the dialog applies changes live, opening it switches the source to the first preset and leaves the dialog dirty. The common-issues wedge entry (`verification-tooling.md`) should name these remaining triggers once P2-010b is confirmed in OBS. |
| N4 | nit | `visualizer-source.cpp:340-342`, `:887-888` | A failed commit clears `preset_target`, so the next pick commits as a hard cut (blend 0), not only the first preset of a new source. `README.md:167` says "Only the first preset a new source shows appears without a blend". Rare (`SV_ERR_GL` only). Adjust the wording or ignore. |
| N5 | nit | `visualizer-source.cpp:438-443` | A cycle request that arrives while the folder is empty stays latched in the core and in `cycle_requested`. If presets are added to the same folder later, auto-cycle does not start until a pick or a settings change. This is consistent with "the folder is not watched". |
| N6 | nit | `visualizer-source.cpp:1139` | A failed `os_task_queue_create` (NULL) is not logged, and the presets would then never load without a log line. `os_task_queue_queue_task(NULL, …)` is null-safe, so it does not crash. One warning line would do. |
| N7 | nit | `audio-ring.cpp:46` + `visualizer-source.cpp:829` | After off-air, the ring holds the ~1.5 s captured before it filled, so the first render pushes stale "newest" frames for one frame. Disclosed in P2-004. Harmless. |
| N8 | nit | `visualizer-source.cpp:79`, `:1205` | The 0.1 s step cap slows the animation on canvases below 10 fps. That is unlikely, but `docs/` should state the cap. |
| N9 | nit | `plugin/CMakeLists.txt:137` | The distribution override depends on running after the template's `configure_file` in `set_target_properties_plugin`. It does, and I checked the result. A future template bump could silently undo it. Put the `grep hostArchitectures build_macos_arm64/distribution` check in `docs/integrations.md`. |

No sibling-style drift found. `audio_ring` follows `gl_state_guard`'s class layout, and the free
functions and naming match the file. Every user-visible string goes through `obs_module_text`
(34 keys, P2-010 audit).

## Architecture

- The threading contract from the plan holds:
  - the render thread owns the `sv_context`;
  - the worker only prepares;
  - tick, UI, audio and hotkey threads write atomics, the mutex-guarded slots or the SPSC ring.
- `visualizer-source.cpp` is now 1288 lines with three subsystems: presets/auto-cycle, audio and
  GL/render. That is acceptable for Phase 2. Phase 3 (D3D11 fallback) is a natural point to split
  the preset scheduler and the audio attachment into their own files, as `audio-ring.*` already
  is.
- The process-global PCM buffer (C-006) and the core's missing PCM expiry are core limits. They
  belong in the upstream ask #4 draft (P2-016).
- Open plan item: the session log (04:05:57) says the random-texture limit (presets asking for a
  random texture get none, because no `textures/` folder ships) was "to raise with the user before
  docs". It is not in `output-plan-gate-1-decisions.md`. The README documents it. The orchestrator
  should confirm the user saw it.

## Security

- **Critical / High:** none.
- **Medium:** none.
- **Low:**
  - The pkg is unsigned and the plugin ad-hoc signed (accepted, C-002). The README explains the
    Gatekeeper override. `installer -target CurrentUserHomeDirectory` needs no admin rights.
  - User text (the folder path) only selects files through `os_opendir`. It reaches nothing but
    `sv_prepare_preset` and the log.
- **Public-repo hygiene.** I grepped every file that would be committed: the diff, the untracked
  sources, `plugin/README.md`, `distribution.in`, and the session `.md`, `.log` and `.py` files
  (PNG, MJS and TXT are git-ignored). I searched for absolute user paths, `/private/tmp`,
  `claude-501`, `scratchpad` paths, the local projects-folder name, e-mail addresses, IPs other
  than 127.0.0.1, signing identity names or team IDs, hostnames and credentials. **Clean.**
  - The only matches are the word "scratchpad" used generically (`output-P2-001.md:31`,
    `P2-010-settings.md:5`), placeholders (`<TEAM_ID>`, `<NOTARY_PROFILE>`, `<creds file>`) and
    `127.0.0.1`.
  - The session records name the user's personal OBS canvas (1080×1920 at 30 fps) and that only
    Apple Development / Distribution identities exist, without names. That is harmless, but the
    user may prefer to trim it.
  - The README names an Apple M1 as the test machine. That is fine.

## Major issues (must-fix)

None in code. **Must-fix before the doc pass** (both README, user-facing):

1. **S1:** make Uninstall remove the installed `.plugin.dSYM`, and mention it in Option A. Or
   drop it from the payload.
2. **S2:** give a working `pkgutil --forget` command for the quoted receipt id. Or fix the id
   through a project-owned `create-package.cmake`.

## Minor issues (nice-to-have)

N1–N9 above. N2, N3 and N9 are mostly documentation items for P2-016. N4 is a one-line README
wording change.

## Tests

N/A. Tests were not expected, and none were added. The scratch-only instrumentation (the
re-entrancy detector, the audio debug callback) is absent from the source, which I confirmed by
reading the full file.

## Plan alignment notes

- P2-001 … P2-010, P2-013 and P2-014 meet their acceptance criteria as recorded, with evidence for
  each. P2-010b is built and correct by inspection, but has **no UI capture**. Normally that would
  block, but the user deferred it explicitly. It stays open, and the PR must list it as not done.
- Deviations, all accepted and recorded:
  - pause defers requests instead of setting `preset_duration` ≤ 0 (P2-008, C-013 intent kept);
  - notarization is blocked on credentials (C-002);
  - the pkg install into OBS moved into P2-011 (now deferred).
- No unrelated refactors. `plugin/cmake/**`, `CMakePresets.json`, `buildspec.json`,
  `exported-symbols.txt` and `plugin-main.cpp` are unchanged.

## Deferred checks: what is unverified and which matter most

These are ordered by how much the code makes them matter.

1. **P2-012, 1080p60: matters most.** P2-006 measured the commit's GL program build on the
   graphics thread at up to **17.8 ms** (glowsticks, M1). That is over the 16.7 ms frame budget at
   60 fps, so a heavy preset change will likely skip a frame at 60 fps. A blend also renders two
   presets at once (P2-007 saw an unexplained +6 skipped frames once). Done-when item 1 ("renders
   at 1080p60 without stalling OBS's renderer") is the criterion most likely to fail or need a
   caveat. Commit cost is inherent to the core's split, so the follow-up would be an upstream ask
   or a documented limit, not a plugin fix.
2. **P2-011, GL-state check on the integrated build: high.** `sv_commit_preset`, `sv_resize` and
   `sv_set_transition_config` are GL-touching calls outside the spike's state audit. Scissor,
   colour mask and clear colour stay unguarded, and P2-009 found that `sv_resize` writes them too.
   Per-task runs showed 0 GL errors, and P2-009 found other sources bit-identical. Never run:
   projectors, multiview, filters, and a second instance added and removed next to image, colour
   and text sources on the final build.
3. **P2-011, race stress plus combined teardown on the final build: medium-high.** The threading
   surface grew a lot after P2-002 (worker, hotkeys, two audio producers). Each piece was
   stress-tested on its own:
   - removal under prepare ×20 (P2-006);
   - removal under hotkey fire (P2-008);
   - removal with audio attached (P2-004/P2-005).

   Not run: the plan's combined case (remove while audio is attached and a prepare is in flight,
   then quit), and the 5-minute screenshot stress on the final build.
4. **P2-011, leak check on quit: medium.** The last clean `Number of memory leaks: 0` was the P2-009
   build. P2-010 and P2-010b changed only properties and defaults, with no lifecycle allocations,
   so the code risk is low. But P2-010's session ended in a SIGINT and a Qt abort (no plugin
   frames), so a normal quit on the final build has not been seen.
5. **P2-013 pkg install into OBS: medium for the README, low for the code.** Install Option A, the
   Gatekeeper steps, quarantine on the zip and the receipt handling are unverified end to end
   (P2-014 lists them). S1 and S2 come from this gap.
6. **P2-010b OBS check: low code risk.** The logic follows from OBS's own `count && idx == -1`
   rule, which I checked in the 31.1.1 source. The screenshot and the `GetInputSettings` check are
   still mandatory UI evidence before this counts as done.
