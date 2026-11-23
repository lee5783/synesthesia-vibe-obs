# P2-004

## Subtasks executed
- **Ring:** new `audio_ring` in `plugin/src/audio-ring.hpp`/`.cpp`. It is a lock-free
  single-producer/single-consumer ring of interleaved stereo float frames.
  - Capacity is 65536 frames (about 1.5 s at 44.1 kHz), a power of two so indexing is a mask.
  - Write and read positions are monotonic `std::atomic<size_t>` with acquire/release ordering.
  - When the ring is full, `write`/`write_silence` keep what fits and drop the rest; they never
    block.
  - `read_newest` takes everything that is waiting and copies out only the newest ≤ 2304 frames.
- **Setting:** an "Audio source" list (`audio_source`, `OBS_COMBO_TYPE_LIST`,
  `OBS_COMBO_FORMAT_STRING`), registered as `info.get_properties`.
  - Entries: "None" (`""`), then every input with `OBS_SOURCE_AUDIO` that has not been removed,
    from `obs_enum_sources`.
  - The label is the source name; the stored value is the UUID (C-005).
  - A stored UUID that no longer resolves appears as "(missing)" at index 1.
- **Capture** follows the compressor-filter sidechain pattern:
  - The source is held through a weak reference only.
  - `video_tick` resolves it lazily, at most once a second, with `obs_get_source_by_uuid`.
  - Attach is `obs_source_add_audio_capture_callback`. Detach is
    `obs_source_remove_audio_capture_callback`, on a setting change, in `destroy`, and when the
    source is lost.
  - The same 1 s check notices a weak reference that has expired or a source marked `removed`.
    It logs "audio source gone" once, detaches, and keeps resolving the UUID.
- **Callback (audio/source thread):** `audio_resampler_resample` converts OBS's planar float (OBS
  rate and layout) to 44.1 kHz interleaved stereo float (C-007), then writes into the ring.
  - Muted: the same number of zero frames is written (C-008).
  - No `sv_*` call, no lock of ours, and no allocation of ours.
  - There is one resampler per attachment, created before the callback is added and destroyed
    after it is removed.
- **Render:** `push_captured_audio` drains the ring into `sv_push_pcm_float(ctx, …, frames, 2)`
  right before `sv_render_frame`, after `gs_texrender_begin`, in the render that wins
  `frame_pending`.
  - If 0.5 s pass with no captured audio, it pushes 2304 zero frames once. The core never drains
    stale PCM on its own (see *What changed and why*).
- **Removed:** `push_synthetic_tone`, `tone_time`, `tone_sample_rate`, `tone_channels` and
  `<cmath>`.
- New strings: `AudioSource`, `AudioSource.None`, `AudioSource.Missing`.
- Verified on OBS with a scratch debug-level build (C-010) and with the final build, using a
  looping Media Source WAV fixture (C-019).

## Files changed
- `plugin/src/audio-ring.hpp` (new)
- `plugin/src/audio-ring.cpp` (new)
- `plugin/src/visualizer-source.cpp` (the working-tree diff also holds the uncommitted P2-002 and
  P2-003 changes)
- `plugin/CMakeLists.txt`: `target_sources` now lists `src/audio-ring.cpp src/audio-ring.hpp`,
  reformatted to the file's multi-line call style. The diff also holds the uncommitted P2-001
  guard.
- `plugin/data/locale/en-US.ini`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md` (P2-004
  section appended)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-P2-004.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-004/P2-004-audio-source.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-004/P2-004-add-audio-debug.py`,
  `P2-004-make-fixture.py`, `P2-004-motion.py`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-004/P2-004-*.png`,
  `P2-004-audio.mjs`, `P2-004-render-time.mjs` (git-ignored local evidence)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/session.log` (log lines)

## What changed and why

**Threading.** There are three threads, each with a fixed role:

- **Producer:** the chosen source's audio thread, which runs our callback under libobs's
  `audio_cb_mutex`.
- **Consumer:** `video_render` under the graphics lock.
- **Control:** tick (and the deferred `update`) on the graphics thread; `destroy` and
  `get_properties` on any thread.

One per-visualizer `audio_mutex` serialises every attach, detach and resampler create/destroy. So
at most one capture callback is registered at a time: there is strictly one producer.

- The callback never takes `audio_mutex`, so the lock order is always ours → `audio_cb_mutex`, with
  no cycle.
- `remove_audio_capture_callback` takes `audio_cb_mutex`, so after it returns the callback is not
  running. That makes destroying the resampler, and in `destroy` freeing the ring, safe.
- The tick uses `try_lock`, so a concurrent `get_properties` or `destroy` never stalls the graphics
  thread.
- The resampler pointer is published to the callback by the add/remove calls, which take the same
  libobs mutex.

**Storage.** Sources are stored by UUID (C-005), so a rename keeps working. A source that is
deleted and created again gets a new UUID and shows "(missing)". Scene collection load order is
handled by the 1 s lazy resolution: the log shows "not found" and then "attached" about 1 s later.

**Silence after audio stops.** The plan expected the core to drain stale PCM within about 1 s, but
it does not. `DrainAudioBufIfExpired` needs `SetAudioDrainClock`, which nothing installs through
the public API. A first debug build showed the levels frozen at their last values after switching
to "None". So `push_captured_audio` pushes 2304 zero frames once after 0.5 s without captured
audio. That covers None, a lost source, and a paused or stopped source.

0.5 s is well above the Media Source's chunk interval (about 85 ms). Muting still goes through the
callback as written zeros.

## Verification
- `cmake --build --preset macos-arm64` (from `plugin/`): `** BUILD SUCCEEDED **` for the final,
  debug and P2-003-baseline variants, with no warning from `plugin/src`. `nm -gU` lists exactly
  the 7 exports, and `lipo -info` reports arm64. The new imports are the resampler, weak-source,
  `obs_get_source_by_uuid` and capture-callback functions.
- `sv_*` reachability: `audio_capture` calls only `audio_resampler_resample` and the ring.
  `sv_push_pcm_float` is called only from `push_captured_audio` ← `render_core_frame` ←
  `video_render`.
- OBS 33.0.3 (OpenGL), collection `synesthesia-spike`, fixture `P2-004 Beat` (48 kHz WAV, 6 s beat
  and 6 s silence, looping):
  - **Levels (debug build).** Beat sections read bass/mid/treb between about 0.1 and 77. Silence, muted, "None"
    and a lost source read 0.000 within about 0.5–1 s.
  - **Startup and collection switch.** The visualizer loads before its source: "not found", then
    "attached" 1.0 s later.
  - **Switching.** Each change logs detach before attach, including switching to "None".
  - **Removing the chosen source while attached** (once during its beat section): "audio source
    gone" once, silence, no crash. The list then shows "(missing)".
  - **Removing a second, attached visualizer while audio flows**, 5 times: no crash.
  - **Quitting with audio attached:** `Number of memory leaks: 0` on all five quits.
  - **GL errors:** 0 GL error lines in every session log.
  - **Render time:** median 9.5–10.2 ms on the final build, against 8.8–9.2 ms for the P2-003
    build measured in the same session. Attached against None on the same build is within noise.
    0 skipped frames in every 30 s window.
- Cleanup: the test inputs and scene were removed, and `Untitled` restored. OBS was quit with 0
  leaks and reopened on `Untitled`. The final build stays installed: its `__text` matches the repo
  build and `codesign --verify` passes.

## Evidence
- `evidence/P2-004/P2-004-audio-source.md`: the full write-up with sanitised log excerpts:
  - levels for beat, silence, muted, None, switch and removal;
  - the attach/detach/gone lines on both builds;
  - the list items before and after removal;
  - the leak and GL-error table;
  - render-time and motion tables.
- Screenshots (960 px, all viewed):
  - `P2-004-final-fresh-none-during-beat.png`: black, no audio, against
    `P2-004-final-fresh-attached-during-beat.png`: the preset fully drawn after choosing the
    source.
  - `P2-004-final-attached-{beat,silence}.png` and `-attached-2-*`.
  - `P2-004-final-muted-during-beat.png` / `-unmuted-during-beat.png`.
  - `P2-004-final-removed-source-during-beat.png`, `-beat3-attached-during-beat.png` /
    `-beat3-removed-2s-later.png`.
  - `P2-004-final-temp-viz-before-remove.png` / `-main-viz-after-temp-removals.png`.
  - `P2-004-final-none-{beat,silence}.png`, `P2-004-debug-reloaded-beat.png`.
- Motion between consecutive frames (`P2-004-motion.py`): about 36–37 on the beat against 22–25 in
  silence.
- Tooling:
  - `P2-004-make-fixture.py` (fixture);
  - `P2-004-add-audio-debug.py` (the scratch C-010 levels log, applied only to a copy);
  - `P2-004-audio.mjs` (phases `setup`, `fresh`, `react`, `mute`, `switch`, `remove-beat`,
    `viz-remove`, `reload`, `motion`, `cleanup`);
  - `P2-004-render-time.mjs`.

## Known limitations / follow-ups
- **Every settings change reloads the current preset.** This is the spike's `visualizer_update`:
  it always re-queues `preset_path`, so changing the audio source logs `preset loaded` again and
  restarts the preset. P2-006 replaces that path. If it stays, it needs a "load only when the path
  changed" check.
- **Process-global PCM (C-006), now visible.** A visualizer on "None" still reacts while another
  instance feeds the core, and a new instance pushes one silent block into the shared buffer.
  Document it for the user, along with upstream ask #4.
- **Upstream ask, new:** the core never drains stale PCM on its own, because the drain clock is
  not installed through the public API. The plugin works around this with the 0.5 s silence
  push. A public `sv_*` way to reset or expire the PCM buffer would be cleaner. Add it to the
  upstream-ask draft next to #4.
- **Same narrow window as libobs's own sidechain.** If the source's last reference is released on
  another thread exactly while its audio thread is inside our callback, the weak reference fails
  before libobs frees `audio_cb_list`. We then skip `remove` and destroy the resampler, or in
  `destroy` the ring, while that last callback may still run. `compressor-filter.c` has the same
  window. It was not hit in any run.
- **Media Source chunking.** The Media Source delivers about 12 chunks of roughly 3,700 frames a
  second. About 18 of 30 renders find nothing new, and each chunk keeps only its newest 2304
  frames. The core keeps only that many anyway. Mic and desktop sources deliver about 10 ms
  chunks. The user checklist (TC-012) covers those.
- **Stale audio after off-air.** The first render after off-air can push up to 2304 frames
  captured just before the ring filled, about 1.5 s old. It lasts one frame.
- **Log lines don't name the visualizer instance.** This matches the existing "preset loaded"
  line. With several visualizers the lines are ambiguous; consider adding
  `obs_source_get_name(visualizer->source)` in P2-010.
- No `get_defaults` yet: the empty string means None. P2-010 owns defaults and the property
  groups. "Whole mix" is P2-005.
- No unit test added (none requested; C-023). The ring is the first candidate if one is wanted.
- **Doc gaps for document-writer:**
  - `docs/architecture.md` (plugin frame and threading): the audio path. That covers the capture
    callback, the resampler to 44.1 kHz stereo, the SPSC ring and the drain of the newest 2304
    frames before `sv_render_frame`. It also covers the 0.5 s silence push, the weak reference
    resolved in tick every 1 s, and `audio_mutex` with its lock order.
  - `docs/structure.md`: the new `plugin/src/audio-ring.*`.
  - `docs/concerns.md`: C-006 (now observed), the core's missing PCM drain, the sidechain
    teardown window, the preset reload on any settings change, and audio/visual sync (C-024).
  - `.claude/how-x-works/plugin-render-frame.md`: the tone is replaced by `push_captured_audio`.
  - `plugin/README.md` (P2-014): the "Audio source" setting, "(missing)", muted = silence, and the
    one-audio-input-per-OBS limit.
- No stuck-capture entry was needed.

## Handoff for downstream review
- **Acceptance criteria addressed:**
  - debug-callback levels clearly above the muted baseline; muted gives 0 within about 1 s;
  - switching to None or another source logs detach then attach;
  - deleting the chosen source while running: no crash, a "gone" log, silence;
  - a collection where the visualizer loads first attaches within the throttle;
  - removing the visualizer and quitting: no crash, 0 leaks.

  Re-creating a source "with the same identity" does not apply under UUID storage. A new source
  gets a new UUID, so the stored one shows "(missing)" (C-005).
- **Areas to scrutinize:**
  - the memory ordering in `audio_ring::store` / `read_newest`;
  - the lock order (`audio_mutex` → libobs `audio_cb_mutex`; the callback takes neither of ours);
  - `try_lock` in `maintain_audio_source`;
  - `detach_audio_source` running `obs_source_release` under `audio_mutex`, which may destroy a
    removed source on the graphics thread, as the sidechain does;
  - the silence push's 0.5 s threshold;
  - `reinterpret_cast` of the resampler's `output[0]` (interleaved float, one plane).
- **How to run:**
  1. Generate the fixture: `python3 -I P2-004-make-fixture.py <out.wav>`.
  2. With the visualizer in `synesthesia-spike`:
     `OBS_WS_CREDS=<creds file> node P2-004-audio.mjs setup <shots dir> <out.wav>`, then the
     phases `react`, `mute`, `switch`, `remove-beat`, `viz-remove`, `fresh`, `motion`.
  3. Measure render time: `node P2-004-render-time.mjs <tag> 30 [beat|none]`.
  4. Finish with `node P2-004-audio.mjs cleanup`.

  For levels, build a scratch copy with
  `python3 -I P2-004-add-audio-debug.py <copy of visualizer-source.cpp>`. `P2-004-motion.py <dir>
  <P2-003 evidence dir>` needs `P2-003-alpha-probe.py`.
