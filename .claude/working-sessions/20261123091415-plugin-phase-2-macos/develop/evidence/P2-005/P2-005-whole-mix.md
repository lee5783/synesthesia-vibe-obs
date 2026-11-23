# P2-005 evidence: audio from the whole mix

OBS 33.0.3 (OpenGL, Apple M1), collection `synesthesia-spike`, a test scene `P2-005 Mix` holding
the visualizer and the fixture `P2-005 Beat`. The fixture is the P2-004 Media Source WAV (48 kHz,
6 s beat then 6 s silence, looping). Its tracks were set to track 1 only with
`SetInputAudioTracks`, on the test input only. OBS created it on tracks 1–6.

Two builds were used:

- **Debug build.** A scratch copy patched with `P2-004-add-audio-debug.py` (C-010). It logs the
  core's bass/mid/treb once a second, plus the frames pushed in that second and their peak.
- **Final build.** The repo source, unchanged. It is installed now.

Paths are shortened to `<plugin>`.

## Levels per mix track (debug build)

Each row covers one connection, from its `audio mix connected` line to the next decision line.
"Reacting s" is the number of 1 s reports with non-zero levels. The fixture plays 6 s of beat in
every 12 s.

| Fixture on | Visualizer reads | Reports | Reacting s | Max bass / mid / treb | Peak | Frames pushed / s |
|---|---|---|---|---|---|---|
| — (None) | — | 23 | 0 | 0 / 0 / 0 | 0.000 | 0 |
| track 1 | whole mix, track 1 | 23 | 12 | 7.08 / 10.66 / 9.75 | 0.913 | 44,749 |
| track 1 | whole mix, **track 2** | 12 | **0** | 0 / 0 / 0 | **0.000** | 45,236 |
| track 1 | whole mix, track 1 (back) | 23 | 6 | 4.72 / 10.07 / 10.19 | 0.914 | 44,871 |
| **track 2** | whole mix, **track 1** | 12 (10:32:55–10:33:07) | **0** | 0 / 0 / 0 | **0.000** | 43,300–46,100 |
| track 2 | whole mix, track 2 | 12 | 6 | 107.62 / 90.62 / 88.57 | 0.914 | 44,765 |
| track 1 | whole mix, track 1 | 13 | 6 | 3.39 / 5.05 / 3.64 | 0.913 | 44,578 |

What the table shows:

- **The mix delivers continuously at 44.1 kHz.** Every connection pushes about 44,100 frames a
  second, even with nothing routed to the track. libobs converts to
  `{44100, AUDIO_FORMAT_FLOAT, SPEAKERS_STEREO}`. On a silent track the core gets real zeros, so
  the P2-004 0.5 s silence push never fires in mix mode.
- **A track that doesn't carry the fixture reads exactly 0.** That holds both ways: choosing
  track 2, and moving the fixture to track 2 while reading track 1.
- **The 107 bass on the first beat after a long silence** is the core's auto-gain settling. The
  next seconds read 9.9, 3.3, 7.3, 2.1 and 1.2.

Excerpt for the track-change rows:

```
10:32:22.274: [audio debug] bass 7.076 mid 10.659 treb 9.752 | pushed 44217 frames, peak 0.818
10:32:31.173: audio mix disconnected: track 1
10:32:31.174: audio mix connected: track 2 (44100 Hz stereo)
10:32:36.540: [audio debug] bass 0.000 mid 0.000 treb 0.000 | pushed 46100 frames, peak 0.000   (fixture beat window)
10:32:43.040: audio mix disconnected: track 2
10:32:43.040: audio mix connected: track 1 (44100 Hz stereo)
10:32:46.807: [audio debug] bass 4.724 mid 10.073 treb 10.190 | pushed 45159 frames, peak 0.914
          SetInputAudioTracks on P2-005 Beat -> {1:false, 2:true}
10:33:00.041: [audio debug] bass 0.000 mid 0.000 treb 0.000 | pushed 44218 frames, peak 0.000   (fixture beat window)
10:33:07.141: audio mix disconnected: track 1
10:33:07.142: audio mix connected: track 2 (44100 Hz stereo)
10:33:10.208: [audio debug] bass 107.623 mid 90.625 treb 88.569 | pushed 46099 frames, peak 0.818
```

## Switching between a chosen source and the whole mix

Same lines on both builds. Final build:

```
10:38:26.130: audio mix connected: track 1 (44100 Hz stereo)
10:38:27.731: audio mix disconnected: track 1
10:38:27.731: audio source attached: P2-005 Beat (48000 Hz to 44100 Hz stereo)
10:38:30.731: audio source detached: P2-005 Beat
10:38:30.731: audio mix connected: track 1 (44100 Hz stereo)
10:38:33.730: audio mix disconnected: track 1
10:38:33.731: audio source: none
10:38:36.731: audio mix connected: track 1 (44100 Hz stereo)
10:38:39.730: audio mix disconnected: track 1
10:38:39.730: audio source attached: P2-005 Beat (48000 Hz to 44100 Hz stereo)   (mix track also set to 3)
10:38:42.731: audio source detached: P2-005 Beat
10:38:42.732: audio mix connected: track 1 (44100 Hz stereo)
          identical settings re-applied: no audio line (no reconnect)
```

The old producer is always removed before the new one is added. There is never a second
`connected` or `attached` line without a removal between them. Changing the mix track while a
chosen source is selected only stores the value: the 10:38:39 change sets track 3 and logs no mix
line.

## Load and teardown

- **Collection load with "Whole mix" saved.** The debug session quit while connected. The final
  build then opened `synesthesia-spike` with the visualizer on `whole_mix`. It connected inside
  `create`:

  ```
  10:36:41.036: sv_init_gl and sv_create succeeded (1920x1080)
  10:36:41.036: audio mix connected: track 1 (44100 Hz stereo)
  ```

- **Removing a visualizer while the mix flows.** A temporary visualizer on the whole mix (tracks
  1, 1, 2, 1, 2) was removed 5 times in each build, each time during the fixture's beat section.
  Each removal logs `audio mix disconnected: track N` from `destroy`, with no crash. The main
  visualizer kept rendering: see `P2-005-*-main-viz-after-temp-removals.png`.
  - During this phase two instances were connected at once, so the debug levels mix both
    instances. That is C-006, the process-global PCM buffer: a temporary visualizer on track 2
    "reacts" because the main one feeds track 1. Those rows are left out of the table above.

| Session | Ends with | Leaks | GL error lines |
|---|---|---|---|
| debug build | quit while connected to the whole mix, audio flowing | 0 | 0 |
| final build (scenarios) | quit on `Untitled` after cleanup | 0 | 0 |

"GL error lines" uses the P2-004 pattern: `gl… failed`, `device_load_texture (GL) failed` or
`GL_INVALID`.

## Render time (final build, 30 s windows, `P2-005-render-time.mjs`)

| Audio | Median ms | Min–max ms | Skipped |
|---|---|---|---|
| whole mix | 9.74 | 5.56–12.89 | 0 |
| chosen source | 9.25 | 6.03–12.22 | 0 |
| none | 9.51 | 6.84–12.41 | 0 |
| whole mix (again) | 8.88 | 5.63–11.37 | 0 |

These are within noise of each other and of P2-004 (9.5–10.2 ms). Over the whole scenario run,
`GetStats` counted 15 skipped frames, all during the temporary-visualizer churn. Each new
instance runs `sv_create` and a synchronous preset load on the graphics thread, which is the
spike path P2-006 replaces. The steady-state windows show 0.

## Properties dialog

`OpenInputPropertiesDialog` was sent through the websocket for a hidden temporary visualizer. The
dialog window alone was captured with `screencapture -l <window id>`; `P2-005-windows.swift`
finds the window id. Removing the input closed the dialog. No GUI automation was used.

- `P2-005-props-whole-mix.png`: "Audio source: Whole mix", with "Mix track: 2" shown.
- `P2-005-props-none.png`: "Audio source: None", with "Mix track" hidden.

The `audio_source` list items read through the websocket:
`[["None",""],["Whole mix","whole_mix"],["BS-008 Browser",<uuid>],["P2-005 Beat",<uuid>]]`. The
default settings for the kind read `{"audio_mix_track":1}`.

## Screenshots

- `P2-005-debug-none-beat.png`: None, just after start, black.
- `P2-005-debug-mix-track1-beat.png`: the preset drawn after connecting the whole mix.
- The other `*-mix-track*` shots are taken during the fixture's beat window for each track
  combination.
  - Fractopia keeps animating once it has started, even in silence.
  - So these stills show that the source renders in every state. Reaction is proven by the
    levels table above, per C-010.
- `*-temp-viz-before-remove.png` and `*-main-viz-after-temp-removals.png`.

## Cleanup

- The test inputs and scene were removed.
- The visualizer's settings were restored with `overlay: false` to the values captured at setup:
  `{"audio_source":"","preset_path":""}`.
- `Untitled` was restored. OBS was quit (0 leaks) and reopened on `Untitled` with the final build
  installed.
  - The installed `__text` hash matches the repo build, and `codesign --verify` passes.
  - The installed binary has no `audio debug` strings.
