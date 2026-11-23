# P2-006

## Subtasks executed
- **Settings:**
  - `preset_folder`: `obs_properties_add_path(…, OBS_PATH_DIRECTORY, …)`.
  - `preset`: a string list (`OBS_COMBO_TYPE_LIST`). The value is the file name and the label is
    the file name without `.milk`.
  - `preset_hint`: an `OBS_TEXT_INFO` property with `OBS_TEXT_INFO_WARNING` styling, reading
    "Choose a folder of .milk presets."
  - `preset_folder_modified` is the folder's modified callback and returns `true`. It refills the
    list from the folder, sorted case-insensitively, skipping dot-files and directories. It shows
    the list when the folder has presets, and the hint when it has none.
  - libobs runs the callback for the properties dialog and for
    `GetInputPropertiesListPropertyItems`.
- **Worker:**
  - One `os_task_queue_t` per source, created after a successful `sv_create`.
  - `update` (`request_preset`) stores the folder and file name under `preset_mutex`, bumps
    `preset_generation` and queues `prepare_requested_preset`. It does this only when the folder or
    file string changed.
  - The task reads the latest request and skips it if that generation was already handled. It
    then scans the folder; a missing or unlisted file falls back to the first preset in sorted
    order.
  - If the folder is unset, missing or holds no presets, the task logs one C-004 warning and
    stops.
  - If the resolved path is already the current target, the task does nothing.
  - Otherwise it runs `sv_prepare_preset` with no lock held and logs
    `preset prepared: <path> (N ms)`.
  - **Handing over:** under the lock, a result whose generation is still current goes into the
    `ready_preset` slot, freeing any handle there that was never committed. A superseded result,
    or one that finishes while the source closes, is freed on the worker and logged
    `preset prepared but superseded, dropped`.
- **Render:**
  - `commit_ready_preset` replaces `load_pending_preset` at the top of `render_core_frame`. It
    takes the slot under the lock.
  - It runs `sv_commit_preset(ctx, h, 0.0f)` inside `with_core_gl_state`, then
    `sv_prepared_preset_free`.
  - On success it logs `preset committed: <path> (N ms)`.
  - On failure (including `SV_ERR_GL`) it logs a warning with `sv_last_error`, frees the handle and
    clears the target, so a re-pick retries.
- **Destroy:** `stop_preset_worker` runs after the audio teardown and before `obs_enter_graphics`
  and `sv_destroy`:
  - it sets `preset_closing`, so queued tasks return at once;
  - `os_task_queue_destroy` runs the remaining queue and joins the thread, which waits for an
    in-flight prepare;
  - it then frees any pending handle and logs `preset worker stopped (N ms)`.
- **Removed:** `setting_preset_path`, `default_preset_file`, `module_file_path`,
  `preset_path_from_settings`, `pending_preset`, `load_pending_preset` and the synchronous
  `sv_load_preset` (C-021: no migration, old values are ignored).
- **Strings:** new `en-US.ini` keys `PresetFolder`, `Preset` and `PresetFolder.Hint`.

## Files changed
- `plugin/src/visualizer-source.cpp`. The working-tree diff also holds the uncommitted
  P2-002…P2-005 changes.
- `plugin/data/locale/en-US.ini`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md` (P2-006
  section appended)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-P2-006.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-006/P2-006-presets.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-006/*.png`,
  `P2-006-presets.mjs`, `P2-006-switch.mjs`, `P2-006-stale.mjs`, `P2-006-ws-session.mjs` and
  `P2-006-make-heavy.py`. These are git-ignored local evidence; only the `.md` is tracked.
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/session.log` (log lines)

## What changed and why

**Threads.** The worker reads only the context's paths (the core allows this while the context
renders) and writes only the guarded slot. `video_render` is the only place that commits.
`sv_commit_preset` binds GL programs, so it sits inside the GL state guard, exactly where
`sv_load_preset` sat.

`preset_mutex` is never held across a prepare, a commit or a free, so the render thread waits at
most for a few pointer and string swaps. The audio mutex and the preset mutex are never nested.

**No I/O on the render path.** OBS defers `update` for video sources to the graphics thread
(`video_tick`). So `update` only copies two strings and queues a task, and the folder scan happens
on the worker.

**Generations.** Each change of folder or file bumps the generation. A task reads the latest
request when it starts, so a burst of requests coalesces: requests that are already superseded
are never prepared. A prepare that finishes after a newer request arrived is dropped and freed.
`preset_target`, the last path handed to render, makes re-applying the same effective preset a
no-op. That covers identical settings, choosing the implicit first file explicitly, and switching
back to the current preset while a different one is still preparing.

**Destroy safety.** The core forbids destroying a context during a prepare. `destroy` therefore
joins the worker before `sv_destroy`. libobs calls `info.destroy` on its destruction task thread,
and the join happens before `obs_enter_graphics`, so waiting for a 400 ms prepare never holds the
graphics lock.

**Folder semantics.**
- Choosing a folder shows its first preset at once.
- Picking from the list commits that file.
- An empty, missing or unset folder logs one warning per change and shows the hint. The preset
  already committed keeps rendering, because the core has no "unload".
- A new source with no folder shows the core's no-preset image: an opaque grey wave and mesh.
- When the stored file is not in a newly chosen folder, the dialog's combo selects the first entry
  and writes it back. That is OBS's own behaviour for an unmatched list value; the worker makes
  the same choice for websocket changes.

**Dev copy (C-003).** The installed bundle was replaced by a clean `ditto` of the build. Its
`Contents/Resources/presets` and `textures` dev copies are gone, because nothing reads the bundle's
presets any more, and the release ships none. Testing used a scratch folder of the core's
mvp-presets with the textures flattened beside them. The core finds textures in the preset's own
folder.

## Verification
- `cmake --build --preset macos-arm64` (from `plugin/`): `** BUILD SUCCEEDED **`, with no warning
  from `plugin/src`.
  - `nm -gU` lists exactly the 7 exports, so `exported-symbols.txt` is unchanged.
  - New libobs imports: `os_task_queue_create`/`queue_task`/`destroy`, `os_opendir`/`readdir`,
    `astrcmpi`, `obs_properties_add_path`/`add_text`, `obs_property_list_clear` and
    `obs_property_text_set_info_type`.
  - The plugin object references `sv_prepare_preset`, `sv_commit_preset` and
    `sv_prepared_preset_free`, and no longer references `sv_load_preset`.
- **`sv_*` reachability:**
  - `sv_commit_preset` is reached only from `commit_ready_preset` ← `render_core_frame` ←
    `visualizer_video_render`, inside `with_core_gl_state`.
  - `sv_prepare_preset` is reached only from the worker task.
  - `sv_destroy` runs after `stop_preset_worker`.
- **OBS 33.0.3** (OpenGL), collection `synesthesia-spike`, 30 fps. Full write-up in
  `evidence/P2-006/P2-006-presets.md`:
  - **Folder → list:** 11 sorted entries for `presets-a` and 2 for `presets-b`. Each pick logs
    `prepared` (worker) then `committed` (render). Commits take 0.9–3 ms, and 11.8–17.8 ms for
    glowsticks.
  - **Switching 30 times at 1 s:** the old synchronous build skipped **16** frames, P2-006 skipped
    **0**. With about 400 ms "heavy" prepares, P2-006 still skipped **0**.
  - **Broken preset:** `preset failed to prepare: … (not a MilkDrop preset: …)`. The previous
    preset keeps rendering (screenshots).
  - **Empty, missing and unset folder:** one warning each, worded "has no .milk presets", "not
    found" and "not set". The list is empty and the previous preset keeps rendering.
  - **Audio-source and track changes:** no `preset` line at all. The preset is no longer restarted.
  - **Rapid heavy switching:** 3 prepares were dropped as superseded, and only the last request
    was committed.
  - **Remove during prepare, 20 + 20 iterations:** no crash and no leftover inputs. The heavy run
    logged `superseded, dropped` 20 times and `preset worker stopped (274–441 ms)`.
  - **Leaks and GL errors:** both quits printed `Number of memory leaks: 0`, with 0 GL error lines.
  - **Persistence:** a saved folder and preset are prepared and committed on collection load.
- **Cleanup:**
  - The spike visualizer's settings were restored exactly (`overlay: false`), and the temporary
    inputs and scene removed.
  - `Untitled` was restored. OBS was quit with 0 leaks and reopened on `Untitled`.
  - The P2-006 build stays installed, identical to the repo build (`cmp`).

## Evidence
- `evidence/P2-006/P2-006-presets.md`: log excerpts (paths shown as `<scratch>`), the
  switching-cost table, the stale-drop and removal tables, leaks, GL errors and cleanup.
- Screenshots (960 px), all viewed:
  - `P2-006-unset-fallback.png`: the core's no-preset image.
  - `P2-006-folder-a-implicit-first.png`, `-pick-swarm.png`, `-pick-fractopia.png` and
    `-folder-b-fractopia.png`.
  - `P2-006-broken-before.png` / `-broken-after-keeps-previous.png`.
  - `P2-006-{empty,missing,unset}-folder-keeps-previous.png`.
  - `P2-006-rapid-final.png` and `P2-006-reload-saved-swarm.png`.
  - `P2-006-props-folder-set.png`: the dialog with folder, Browse and the preset list.
  - `P2-006-props-folder-unset.png`: the dialog with the hint and the list hidden.
- Scripts:
  - `P2-006-presets.mjs`, with phases `unset`, `folder`, `broken`, `empty`, `audio`, `rapid`,
    `remove`, `remove-heavy` and `cleanup`;
  - `P2-006-switch.mjs` (`old|new <folder> <seconds> <interval>`);
  - `P2-006-stale.mjs`;
  - `P2-006-make-heavy.py`.

## Known limitations / follow-ups
- **No user-facing feedback for a failed preset.** Picking a broken preset logs a warning and keeps
  the old one, but the list still shows the broken name as selected. P2-010 could add a status
  line.
- **The commit's GL program build is still on the graphics thread.** It took up to 17.8 ms for
  glowsticks on an M1, which is inside the 33 ms budget at 30 fps but tighter at 60 fps. P2-012
  measures 1080p60. This is inherent in the core's split: GL work must run on the GL thread.
- **`destroy` blocks the libobs destruction thread** until an in-flight prepare finishes (up to
  about 0.4 s for a pathological preset). It does not block rendering.
- **Random textures.** The core's random-texture picker scans only `<resource_path>/textures`
  inside the bundle, and that folder is now empty in a release install. Presets that ask for a
  random texture (`rand…` sampler names, for example "Tiger No.5 (Random texture flow)") get none.
  Named textures are found next to the preset. Fixing this needs either a texture-folder setting
  with `texture_search_path`, which must be set at `sv_create` time, or an upstream ask for the
  picker to also scan the preset or search folder. Either is outside P2-006 per the plan; record it
  under C-003.
- **Folder changes are not watched.** Adding files to the folder shows up the next time the
  properties open (the callback rescans) or on the next change. Auto-cycle in P2-007 will need the
  file list too. It can reuse `list_preset_files` on the worker.
- **`get_defaults` still holds only the mix track.** The preset settings default to empty strings
  implicitly. P2-010 owns the full defaults and the group layout.
- **Log lines do not name the instance** (as in P2-004).
- **Manual picks commit as a hard cut** (blend 0, as the spike did). P2-007 adds `blend_seconds`.
- No unit test added (none requested).
- **Doc gaps for document-writer:**
  - `docs/architecture.md`: the preset path. That covers the folder setting, the worker, the
    generation slot, commit in render under the guard, destroy joining the worker, and `update`
    being deferred to the graphics thread for video sources.
  - `docs/structure.md`: no new files.
  - `docs/integrations.md`: the install snippet still copies `presets/` and `textures/` into the
    bundle. That copy is obsolete; users choose a folder instead.
  - `docs/concerns.md`: random textures without a bundled `textures/`; commit cost on the graphics
    thread; no folder watching.
  - `.claude/how-x-works/plugin-render-frame.md`: `load_pending_preset` became
    `commit_ready_preset`.
  - `.claude/common-issues/` (verification tooling): `RemoveInput` on a source in a scene that is
    not rendering leaves it alive, because libobs prunes the item only in `scene_video_render`.
    Test removals in a rendered scene.
  - `plugin/README.md` (P2-014): choosing a preset folder, putting textures next to the presets,
    and what an empty folder looks like.

## Handoff for downstream review
- **Acceptance criteria addressed:**
  - choosing another preset applies live, with `prepared` from the worker and `committed` from
    render;
  - changing the folder refreshes the list;
  - an empty or unset folder shows the hint, logs one warning, renders the C-004 fallback and
    never crashes;
  - a parse failure logs the core's error and keeps the current preset;
  - `GetStats` shows no skipped frames while switching (16 before);
  - deleting the source mid-prepare 20 times causes no crash and 0 leaks on quit.
- **Areas to scrutinize:**
  - the generation and handled-generation logic in `prepare_requested_preset`;
  - the `preset_target` skip and clearing it on a commit failure;
  - the ordering in `visualizer_destroy`: audio teardown, then worker stop, then
    `sv_destroy` under graphics;
  - `os_task_queue_destroy` relying on libobs moving its stop task behind queued tasks, so the
    queue drains and the join waits;
  - the C-004 wording and the hint's visibility.
- **How to run** (from a scratch copy of the evidence scripts):
  1. Build scratch folders `presets-a`, `-b`, `-empty` and `-heavy` as described in
     `P2-006-presets.md`.
  2. With the visualizer in `synesthesia-spike`, run
     `OBS_WS_CREDS=<creds file> node P2-006-presets.mjs <phase> <scratch root> <shots dir>`.
     `P2-006-presets.mjs` imports `./ws-session.mjs`, so rename `P2-006-ws-session.mjs` next to
     it.
  3. Measure with `node P2-006-switch.mjs new <scratch>/presets-a 30 1000`.
