# P2-002

## Subtasks executed
- Replaced `pending_seconds` / `frame_pending` with `std::atomic<uint64_t> pending_nanoseconds`
  and `std::atomic<bool> frame_pending` in `visualizer_source`.
- `visualizer_video_tick` now only adds the frame delta to `pending_nanoseconds` (`fetch_add`) and
  sets `frame_pending`. It no longer calls `sv_*`.
- `visualizer_video_render` takes `frame_pending` with `exchange(false)`. Only a render that wins the
  flag renders the core. `take_pending_seconds` then takes the delta with `exchange(0)` and caps it
  at 0.1 s (`max_step_seconds`, C-011). Anything above the cap is dropped, not carried over.
- The synthetic tone moved into `render_core_frame`, driven by the consumed (capped) delta. It runs
  under the graphics lock, right after `load_pending_preset`. P2-004 deletes it.
- New `visualizer_reset_pending_delta`, registered as both `info.show` and `info.activate`. It
  zeroes the pending delta and leaves the flag alone.
- `visualizer_update` is unchanged: it still writes only the mutex-guarded preset slot.
- Verified per C-009 with a scratch-only re-entrancy detector, the 5-minute screenshot stress
  loop and the off-air test. Ran the same tests on the spike as a baseline.

## Files changed
- `plugin/src/visualizer-source.cpp`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md` (P2-002
  section appended)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-P2-002.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-002/P2-002-race-fix.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-002/P2-002-add-detector.py`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-002/P2-002-*.png`,
  `P2-002-*.mjs` (git-ignored local evidence)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/session.log` (log lines)

## What changed and why
In the spike, `video_tick` ran on the graphics thread without the graphics lock and called
`sv_push_pcm_float`. `video_render`, which obs-websocket's `GetSourceScreenshot` runs on its own
thread under the graphics lock, called `sv_render_frame` at the same time. That broke the
single-threaded `sv_context` rule. The baseline detector caught this: 3 overlapping core calls in
60 s on the spike.

Every `sv_*` call now sits in `video_render`, or in create/destroy under `obs_enter_graphics`. The
tick side only touches two atomics. The flag is exchanged before the delta, so a second render in
the same frame (projector, screenshot) reuses the texture. It does not step the core again, and
no delta is counted twice.

The spike passed a whole off-air gap to the core in one step, which turned the frame near-black.
Two changes fix that: the delta resets on `show`/`activate`, and one step is capped at 0.1 s.
`GetSourceScreenshot` of a hidden source goes through `show`, so it also gets a continuous frame.
No GL-state code changed: the guard and the draw path are untouched.

## Verification
- `cmake --build --preset macos-arm64` (from `plugin/`): `** BUILD SUCCEEDED **` for the final
  source and both scratch variants. No warning from `plugin/src`. `nm -gU` lists exactly 7 exports;
  `lipo -info` reports arm64.
- `sv_*` reachability check (grep plus the call sites; table in the evidence file): no `sv_*` call
  in `video_tick`, `update`, `show`/`activate`.
- On OBS 33.0.3 (OpenGL):
  - **Stress run, final build with the detector:** 5802/5802 screenshots OK in 302 s (19.2/s),
    560 item toggles and 28 visualizer hide/show cycles. **0 overlaps in 22450 core calls.**
  - **GL errors:** 0 GL error lines in every log.
  - **Leaks:** `Number of memory leaks: 0` on each of the three quits.
  - **Render time:** `averageFrameRenderTime` 5.4–8.9 ms, within the spike's 6.9–10.3 ms.
    `renderSkippedFrames` stayed flat.
  - **Final build without the detector:** off-air test plus a 60 s smoke stress (1159/1159 OK),
    then `Untitled` restored, quit with 0 leaks, OBS reopened.

## Evidence
- `evidence/P2-002/P2-002-race-fix.md`: the full write-up. It has:
  - the detector log lines for the spike (3 overlaps) and the final build (0 overlaps);
  - the stress summaries and `GetStats`;
  - the GL-error counts, the leak lines and the reachability table.
- Off-air screenshots (all viewed). The final build's frames continue the on-air frame with no jump:
  - spike, after 12 s hidden: `evidence/P2-002/P2-002-before-2-hidden-12s.png`, near-black;
  - spike, just shown: `evidence/P2-002/P2-002-before-3-just-shown.png`, near-black;
  - final build: `evidence/P2-002/P2-002-final-1-on-air.png` → `P2-002-final-2-hidden-12s.png` →
    `P2-002-final-3-just-shown.png` → `P2-002-final-4-shown-1s.png`;
  - final build with the detector, after the stress: `P2-002-after-stress-2-hidden-15s.png`.
- Whole scene, showing the image, colour and text sources drawing correctly:
  `P2-002-final-5-scene.png`, `P2-002-after-stress-5-scene.png`, `P2-002-before-5-scene.png`.
- Helpers: `P2-002-add-detector.py` (the scratch detector, applied only to scratch copies),
  `P2-002-stress.mjs`, `P2-002-offair.mjs`, `P2-002-ws-session.mjs`.

## Known limitations / follow-ups
- Taking the flag and taking the delta are two separate exchanges. If a tick lands between them,
  this render takes that tick's delta too, and the next render steps the core by about 0 s. That is
  harmless (a repeat frame, no lost or doubled time). A packed single atomic would avoid it if a
  reviewer prefers.
- The tone's `static` sample buffer is now touched only under the graphics lock. It is still
  process-global, which is fine until P2-004 removes it.
- `sv_push_pcm_float` is not inside `with_core_gl_state`. It doesn't touch GL (the header doesn't
  list it among the GL-thread calls), and the spike didn't guard it either.
- No unit test added (none requested). The race fix rests on the detector and the stress run, per
  C-009. TSan stays unavailable against release OBS.
- Doc gaps for document-writer:
  - `docs/concerns.md`: the "`sv_context` race" entry and "Whole off-air gap passed as one step"
    are fixed. Keep the C-009 verification note.
  - `docs/architecture.md`, *Plugin frame* and *Threading*: tick writes atomics only; render
    exchanges the flag and the delta, caps at 0.1 s and pushes the tone; `show`/`activate` reset
    the delta.
  - `.claude/how-x-works/plugin-render-frame.md` sections 1–3 need the same update. The line
    citations have moved.
- No stuck-capture entry was needed.
- The plugin is left installed (final build plus the dev-only presets and textures). C-026 removes
  it at the end of the run.

## Handoff for downstream review
- Acceptance criteria addressed:
  - no `sv_*` reachable from `video_tick`, `update` or another non-graphics callback;
  - the 5-minute on-air stress with item toggles: no crash, 0 GL error lines, 0 detector overlaps;
  - a screenshot taken after 12–15 s off air is not black;
  - render time within the spike's range.
- Areas to scrutinize:
  - the order of the two exchanges in `visualizer_video_render` / `take_pending_seconds`;
  - `show` and `activate` both resetting only the delta;
  - the float-to-nanosecond conversion in `video_tick`, which skips non-positive deltas;
  - the tone push placed before `gs_texrender_begin`. If the begin fails, the audio is still
    consumed for that frame.
- How to run: install as in `docs/integrations.md`, then run
  `OBS_WS_CREDS=<creds file> node P2-002-stress.mjs 300 <out-prefix>` and
  `node P2-002-offair.mjs <out-prefix> 12` against a collection with a `Scene` that holds the
  visualizer. For overlap counts, build a scratch copy with
  `python3 P2-002-add-detector.py <copy of visualizer-source.cpp>`.
