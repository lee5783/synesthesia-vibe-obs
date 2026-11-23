# P2-005

## Subtasks executed
- **List entry.** "Whole mix" is the second entry of the "Audio source" list, after "None" and
  before the sources. It is stored as the sentinel string `whole_mix` in the same `audio_source`
  setting. That value cannot collide with a UUID.
- **Mix track setting.** `audio_mix_track` is an int from 1 to 6 (`MAX_AUDIO_MIXES`), step 1.
  - Its default is 1 through a new `info.get_defaults` (`visualizer_get_defaults`), which holds
    only this default for now.
  - The value is also clamped to 1–6 when read (`audio_mix_index_from_settings`).
- **Visibility.** `audio_source_modified` is the list's modified callback. It shows "Mix track"
  only while "Whole mix" is selected and returns `true`.
- **Connect.** `connect_audio_mix` calls `audio_output_connect(obs_get_audio(), track - 1, &{44100,
  AUDIO_FORMAT_FLOAT, SPEAKERS_STEREO, false}, mix_capture, visualizer)`. It logs
  `audio mix connected: track N (44100 Hz stereo)`, or a warning if the connect fails.
- **Callback.** `mix_capture` runs on the libobs audio thread, under libobs's `input_mutex`. It
  writes `data[0]`, which is interleaved stereo float at 44.1 kHz, straight into the P2-004 ring.
  It takes no lock of ours, calls no `sv_*` and allocates nothing.
- **Disconnect.** `disconnect_audio_mix` calls `audio_output_disconnect` with the same mix index,
  callback and param, then logs `audio mix disconnected: track N`. It runs on every selection
  change and on a track change in mix mode (in `select_audio_source`), and in `destroy`. All of
  them run under `audio_mutex`, after the source detach.
- **Tick.** `maintain_audio_source` skips UUID resolution while "Whole mix" is selected.
- **Strings.** New `en-US.ini` keys: `AudioSource.WholeMix="Whole mix"` and
  `MixTrack="Mix track"`.

## Files changed
- `plugin/src/visualizer-source.cpp` (the working-tree diff also holds the uncommitted P2-002,
  P2-003 and P2-004 changes)
- `plugin/data/locale/en-US.ini`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md` (P2-005
  section appended)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-P2-005.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-005/P2-005-whole-mix.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-005/*.png`,
  `P2-005-mix.mjs`, `P2-005-render-time.mjs`, `P2-005-windows.swift` (git-ignored local evidence)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/session.log` (log lines)

## What changed and why

**One producer at a time.** The ring is single-producer, so the source capture callback and the
mix callback must never both be registered. Every attach, detach, connect and disconnect runs
under the per-visualizer `audio_mutex`:

- `select_audio_source` removes both producers first (detach the source, then disconnect the mix),
  then connects the mix if "Whole mix" is selected.
- `audio_output_disconnect` takes `input_mutex`, the same mutex `do_audio_output` holds while it
  calls inputs. So the mix callback is not running once disconnect returns. That is the same
  guarantee `remove_audio_capture_callback` gives for a source.
- The lock order stays ours → libobs. The mix callback takes nothing of ours.

**Connect in `update`, not `tick`.** A chosen source can appear after the visualizer while a
collection loads, which is why that path resolves lazily in `tick`. The mix always exists once OBS
is running, so connecting directly in `update` is simpler. That `update` call can come from
`create` or from a settings change on the UI thread. The log shows the connect happening inside
`create` on a collection load.

**Change detection.** Re-applying identical settings does nothing. A track change only reconnects
while "Whole mix" is selected. In source mode it just stores the index.

**Silence.** The mix callback fires about every 21 ms even when nothing is routed to the track, so
the core receives real zeros. The P2-004 0.5 s silence push stays for None and source mode.

**Shutdown.** `obs_shutdown` runs `stop_audio`, which closes the audio output and frees its
inputs, then nulls `obs->audio.audio` before `obs_free_data`. A visualizer destroyed that late
calls `audio_output_disconnect(NULL, …)`, which is a no-op. Normally the frontend destroys sources
earlier, while audio is still live.

## Verification
- `cmake --build --preset macos-arm64` (from `plugin/`): `** BUILD SUCCEEDED **` for the final and
  debug variants, with no warning or error from `plugin/src`.
  - `nm -gU` lists exactly the 7 exports (no new `obs_module_*` entry, so
    `exported-symbols.txt` is unchanged).
  - New imports: `audio_output_connect`, `audio_output_disconnect`, `obs_get_audio`,
    `obs_data_set_default_int`, `obs_properties_add_int`, `obs_properties_get`,
    `obs_property_set_modified_callback` and `obs_property_set_visible`.
- **`sv_*` reachability:** `mix_capture` calls only `audio_ring::write`. `sv_push_pcm_float` is
  still reached only from `push_captured_audio` ← `render_core_frame` ← `video_render`.
- **OBS 33.0.3** (OpenGL), collection `synesthesia-spike`, with the P2-004 fixture on track 1:
  - **Whole mix, track 1:** reacts. Bass/mid/treb reach up to about 7 / 11 / 10 on the beat and 0
    in silence. About 44,100 frames are pushed every second.
  - **Whole mix, track 2 with the fixture on track 1**, and **track 1 with the fixture moved to
    track 2** (`SetInputAudioTracks` on the test input only): exactly 0, peak 0.000, while the mix
    keeps delivering about 44,100 frames a second. Swapping to the matching track reacts again.
  - **Switching** source ↔ mix ↔ None: removal is always logged before the next connect or attach.
    Identical settings re-applied log nothing.
  - **Removing a visualizer on the whole mix while audio flows,** 5 times per build: `audio mix
    disconnected` from `destroy`, no crash.
  - **Collection load with `whole_mix` saved:** connected in `create`.
  - **Quit while connected (debug build)** and **quit after cleanup (final build):**
    `Number of memory leaks: 0`. 0 GL error lines in both session logs.
  - **Render time median:** 9.74 / 8.88 ms on the whole mix, 9.25 ms on a chosen source and
    9.51 ms on None. 0 skipped frames in each 30 s window.
- The properties dialog was opened through `OpenInputPropertiesDialog` and captured with
  `screencapture -l`. "Mix track" shows for "Whole mix" and is hidden for "None".
- **Cleanup:**
  - The test inputs and scene were removed, and the visualizer's original settings restored
    (`overlay: false`).
  - `Untitled` was restored. OBS was quit with 0 leaks and reopened on `Untitled`.
  - The final build stays installed: its `__text` hash matches the repo build, `codesign --verify`
    passes, and it has no debug strings.

## Evidence
- `evidence/P2-005/P2-005-whole-mix.md`: the full write-up, with sanitised log excerpts:
  - the levels table per track combination;
  - the track-change excerpt;
  - the switch ordering;
  - the load-time connect and the removal lines;
  - the leak and GL-error table, render time and the dialog notes.
- `P2-005-props-whole-mix.png`: "Audio source: Whole mix" with "Mix track: 2" visible.
  `P2-005-props-none.png`: "None", with "Mix track" hidden. Both were viewed.
- Visualizer stills at 960 px, all viewed:
  - `P2-005-debug-none-beat.png` (black, no audio) and `P2-005-debug-mix-track1-beat.png` (drawn
    after connecting the mix);
  - `*-mix-track2-beat-on-1-beat.png`, `*-mix-track1-beat-on-2-beat.png` and
    `*-mix-track2-beat-on-2-beat.png`;
  - `*-temp-viz-before-remove.png` and `*-main-viz-after-temp-removals.png`.

  The stills show that the source renders in every state. Fractopia keeps animating in silence,
  so reaction is proven by the C-010 levels, not by the stills.
- Tooling:
  - `P2-005-mix.mjs`, with the phases `setup`, `react`, `track`, `switch`, `viz-remove`,
    `props-open`, `props-close`, `stats` and `cleanup`;
  - `P2-005-render-time.mjs` (the P2-004 script, plus a `mix` mode);
  - `P2-005-windows.swift` (lists OBS window ids for `screencapture -l`).

  The fixture generator and the debug patch are reused unchanged from `evidence/P2-004/`.

## Known limitations / follow-ups
- **The mix is the stream mix.** It excludes muted sources and sources set to "Monitor only", and
  it carries only what is routed to the chosen track. This is by design: document it, don't fix
  it.
- **C-006 also shows here.** Two visualizers on different tracks, or one on the mix and one on a
  source, feed the same process-global core buffer. In the temporary-visualizer test, an instance
  on track 2 "reacted" to the main instance's track 1 audio. This is upstream ask #4, unchanged.
- **Connecting to a mix makes libobs mix that track** while the visualizer exists. Before, a track
  nothing used was skipped (`clamp_audio_output` skips mixes without inputs). The cost is small,
  and the render time is unchanged.
- **Every settings change still reloads the preset**, including a track change. This is the
  spike's `visualizer_update`, and P2-006 replaces it.
- **The 15 skipped frames during the temporary-visualizer churn** come from `sv_create` plus a
  synchronous preset load on the graphics thread. P2-006 moves preset loading off-thread. The
  steady state shows 0.
- **A failed `audio_output_connect` is not retried** until the settings change. That only happens
  if OBS has no audio output, or if libobs cannot create its resampler. It is logged as a warning.
- **`get_defaults` holds only the track default.** P2-010 owns the full defaults and the property
  layout, for example putting "Mix track" right under "Audio source" in a group.
- **The "Mix track" control is a plain int spinner.** P2-010 may prefer a "Track 1"–"Track 6"
  list.
- No unit test added (none requested).
- **Doc gaps for document-writer:**
  - `docs/architecture.md` (audio path): the whole-mix producer. That covers
    `audio_output_connect` with libobs-side conversion to 44.1 kHz stereo, the callback under
    `input_mutex` writing the ring, one producer at a time under `audio_mutex`, connecting in
    `update` rather than `tick`, and disconnecting in `destroy`.
  - `docs/concerns.md`: the mix excludes monitor-only and muted sources and is per track; C-006
    with mixed modes.
  - `.claude/how-x-works/plugin-render-frame.md`: the ring now has two possible producers.
  - `plugin/README.md` (P2-014): the "Whole mix" option, "Mix track" and the routing caveats.

## Handoff for downstream review
- **Acceptance criteria addressed:**
  - Media Source on track 1 with "Whole mix / track 1": levels react.
  - Moving the fixture off track 1, or choosing track 2: the silent baseline, exactly 0.
  - Switching between the whole mix and a chosen source never double-feeds. The log always shows
    the removal before the next producer is added.
  - "Mix is the stream mix" is documented above, not fixed.
- **Areas to scrutinize:**
  - the early-return condition in `select_audio_source`, for the track change in mix mode versus
    source mode;
  - connecting in `update`, and the lock order `audio_mutex` → `input_mutex`;
  - `disconnect_audio_mix` using `obs_get_audio()` at disconnect time rather than a stored
    `audio_t *`, which is NULL-safe after `stop_audio`;
  - `reinterpret_cast` of `data[0]`: with `AUDIO_FORMAT_FLOAT`, which is not planar, libobs fills
    one interleaved plane.
- **How to run:**
  1. Generate the fixture with `python3 -I evidence/P2-004/P2-004-make-fixture.py <out.wav>`.
  2. With the visualizer in `synesthesia-spike`:
     `OBS_WS_CREDS=<creds file> node P2-005-mix.mjs setup <shots dir> <out.wav>`, then the phases
     `react`, `track`, `switch`, `viz-remove`, then `cleanup`.
     - The script imports `ws-session.mjs` from its own folder, which is
       `evidence/P2-002/P2-002-ws-session.mjs` renamed.
  3. For levels, apply `P2-004-add-audio-debug.py` to a copy of `visualizer-source.cpp`. It
     applies unchanged.
