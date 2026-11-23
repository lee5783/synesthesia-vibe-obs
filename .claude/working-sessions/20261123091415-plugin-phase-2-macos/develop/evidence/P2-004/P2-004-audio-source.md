# P2-004 evidence: audio from a chosen OBS source

Run on 2026-11-23, macOS (Apple Silicon), OBS 33.0.3 on the OpenGL renderer (libobs 31.1.1),
canvas 1080×1920 at 30 fps, OBS audio 48 kHz stereo. Test collection `synesthesia-spike`, test
scene `P2-004 Audio` (the existing `SynesthesiaVibe Visualizer` input plus the fixture). Driven
only through obs-websocket (`P2-004-audio.mjs`, `P2-004-render-time.mjs`). The user's `Untitled`
collection was restored at the end, and the test scene and inputs were removed.

## Fixture (C-019)

`P2-004-make-fixture.py` writes a 12 s, 48 kHz stereo 16-bit WAV: 6 s of beat (a 2 Hz kick, an
off-beat hi-hat and a 440 Hz tone) and then 6 s of digital silence. It plays in a looping
`ffmpeg_source` called `P2-004 Beat`, which is on air in the test scene and on track 1. Captures
are timed with `GetMediaInputStatus.mediaCursor`: "beat" means a cursor between 2.5 and 4.5 s, and
"silence" means a cursor between 9 and 11 s.

## Builds

Built with `cmake --build --preset macos-arm64`. Each build printed `** BUILD SUCCEEDED **` with
no warning from `plugin/src`. `nm -gU` lists exactly 7 exports, and the binary is arm64 only.

| Build | Source | Use |
|---|---|---|
| debug | final source + `P2-004-add-audio-debug.py` (C-010) | levels log for the audio scenarios |
| final | this task's source, no instrumentation | the same scenarios, render time, installed and left installed |
| P2-003 baseline | P2-003 source (synthetic tone, opaque draw) | render-time comparison in the same session |

`P2-004-add-audio-debug.py` is scratch-only. It is applied to a copy of the source, and the
original was restored before the final build. It does three things:

- registers `sv_set_debug_callback`, which logs `bass/mid/treb` once a second along with the
  frames pushed that second and the peak sample;
- logs each time the plugin pushes silence after 0.5 s with no audio;
- adds nothing to the capture callback.

The final source and the installed binary contain none of it: `strings … | grep -c "audio debug"`
returns 0. The installed binary's `__text` (`otool -t | md5`) matches the repo build. The only byte
difference is the ad-hoc re-sign.

A first debug build ran without the silence push. It showed that the core keeps analysing the last
2304 samples forever once pushes stop. The core's drain clock (`SetAudioDrainClock`) is never
installed through the public API, so it never drains. That is why the final code pushes 2304 zero
frames after 0.5 s with no captured audio:

```
10:07:15.096: audio source detached: P2-004 Beat 2
10:07:15.096: audio source: none
10:07:16.963: [audio debug] bass 0.589 mid 1.046 treb 0.718 | pushed 0 frames, peak 0.000, fps 30.0
10:07:17.996: [audio debug] bass 0.648 mid 1.036 treb 0.766 | pushed 0 frames, peak 0.000, fps 30.0
```

## Levels (debug build, sanitised excerpts)

Startup: the scene collection loads the visualizer before its audio source. The source attaches on
the next 1 s check:

```
10:08:49.135: sv_init_gl and sv_create succeeded (1920x1080)
10:08:49.135: audio source not found (8ed0ade5-…); checking again every second
10:08:50.162: audio source attached: P2-004 Beat (48000 Hz to 44100 Hz stereo)
10:08:51.294: [audio debug] bass 14.375 mid 4.721 treb 3.853 | pushed 27648 frames, peak 0.818, fps 30.0
```

"None" while the fixture is in a beat section goes silent within 0.5 s. Re-attaching, the beat
sections are clearly above 0 and the silence sections read 0.000:

```
10:09:02.360: audio source: none
10:09:02.829: [audio debug] no audio for 0.5 s: pushed 2304 frames of silence
10:09:03.493: [audio debug] bass 0.000 mid 0.000 treb 0.000 | pushed 0 frames, peak 0.000, fps 30.1
10:09:21.729: [audio debug] bass 0.000 mid 0.000 treb 0.000 | pushed 27648 frames, peak 0.000, fps 30.0   (silence section)
10:09:25.796: [audio debug] bass 1.269 mid 3.884 treb 2.732 | pushed 27648 frames, peak 0.914, fps 30.0   (beat section)
10:09:27.829: [audio debug] bass 8.071 mid 5.917 treb 5.557 | pushed 27648 frames, peak 0.870, fps 30.0
```

Muted (C-008). The source is muted from about 10:09:35 to 10:09:43, which covers a whole beat
section (about 10:09:37–10:09:43). Pushes continue, but they are silent:

```
10:09:37.063: [audio debug] bass 0.000 mid 0.000 treb 0.000 | pushed 27648 frames, peak 0.000, fps 30.0
10:09:40.096: [audio debug] bass 0.000 mid 0.000 treb 0.000 | pushed 27648 frames, peak 0.000, fps 30.0
10:09:49.296: [audio debug] bass 72.843 mid 76.686 treb 64.615 | pushed 27648 frames, peak 0.614, fps 30.0   (unmuted, next beat)
```

Switching between sources, to "None", and removing the chosen source while attached:

```
10:09:53.396: audio source detached: P2-004 Beat
10:09:53.397: audio source attached: P2-004 Beat 2 (48000 Hz to 44100 Hz stereo)
10:09:56.395: audio source detached: P2-004 Beat 2
10:09:56.395: audio source: none
10:09:56.897: [audio debug] no audio for 0.5 s: pushed 2304 frames of silence
10:09:59.396: audio source attached: P2-004 Beat 2 (48000 Hz to 44100 Hz stereo)
10:10:02.463: audio source gone (c2a6fec0-…); audio is silent until it returns
```

The chosen source is removed during its beat section (cursor 1.7 s). No crash; the levels fall to
0:

```
10:10:43.032: audio source attached: P2-004 Beat 3 (48000 Hz to 44100 Hz stereo)
10:10:43.263: [audio debug] bass 1.088 mid 1.270 treb 1.252 | pushed 27648 frames, peak 0.763, fps 30.0
10:10:44.064: audio source gone (39283515-…); audio is silent until it returns
10:10:44.165: [audio debug] no audio for 0.5 s: pushed 2304 frames of silence
10:10:45.297: [audio debug] bass 0.000 mid 0.000 treb 0.000 | pushed 0 frames, peak 0.000, fps 30.0
```

Scene collection switch (`Untitled` → `synesthesia-spike`). The source attaches within the 1 s
throttle:

```
10:12:24.604: audio source not found (8ed0ade5-…); checking again every second
10:12:25.634: audio source attached: P2-004 Beat (48000 Hz to 44100 Hz stereo)
```

Quit with the visualizer attached and audio flowing:

```
10:15:02.705: [audio debug] bass 16.204 mid 4.821 treb 4.438 | pushed 27648 frames, peak 0.818, fps 30.0
10:15:04.786: plugin unloaded
10:15:04.921: Number of memory leaks: 0
```

"pushed 27648 frames" per second is 12 × 2304. The Media Source delivers about 12 chunks a second of
roughly 3,700 frames each. Every render that finds a chunk pushes its newest 2304 frames, and the
core would discard the rest anyway: `SetAudioBuf` keeps only the newest 2304 of each packet.

## Final build: the same scenarios (log lines, sanitised)

```
10:15:07.791: audio source not found (8ed0ade5-…); checking again every second
10:15:08.794: audio source attached: P2-004 Beat (48000 Hz to 44100 Hz stereo)
10:16:35.997: audio source detached: P2-004 Beat
10:16:35.998: audio source attached: P2-004 Beat 2 (48000 Hz to 44100 Hz stereo)
10:16:38.997: audio source detached: P2-004 Beat 2
10:16:38.997: audio source: none
10:16:41.998: audio source attached: P2-004 Beat 2 (48000 Hz to 44100 Hz stereo)
10:16:45.064: audio source gone (32e13f36-…); audio is silent until it returns
10:17:02.066: audio source attached: P2-004 Beat 3 (48000 Hz to 44100 Hz stereo)
10:17:03.098: audio source gone (53844a59-…); audio is silent until it returns
10:17:08.421: sv_init_gl and sv_create succeeded (1920x1080)        (temporary 2nd visualizer, ×5)
10:17:08.429: audio source attached: P2-004 Beat (48000 Hz to 44100 Hz stereo)
10:17:12.533: audio source detached: P2-004 Beat                     (RemoveInput while audio flows)
…
10:22:08.059: plugin unloaded
10:22:08.167: Number of memory leaks: 0
```

The properties list via `GetInputPropertiesListPropertyItems` (labels, then values), before and
after removing the chosen `P2-004 Beat 2`:

```
[["None",""],["BS-008 Browser","5942…"],["P2-004 Beat","8ed0…"],["P2-004 Beat 2","32e1…"]]
[["None",""],["(missing)","32e1…"],["BS-008 Browser","5942…"],["P2-004 Beat","8ed0…"]]
```

## Teardown, leaks, GL errors

| Session | Ends with | Leaks | GL error lines |
|---|---|---|---|
| first debug build | quit after switch/remove tests | 0 | 0 |
| debug build | quit while attached, audio flowing | 0 | 0 |
| final build (scenarios) | quit while attached, audio flowing | 0 | 0 |
| P2-003 baseline | quit | 0 | 0 |
| final build (cleanup) | quit on `Untitled` after cleanup | 0 | 0 |

"GL error lines" counts lines matching `gl… failed`, `device_load_texture (GL) failed` or
`GL_INVALID`. Five temporary visualizers were removed while attached and receiving audio, with no
crash.

## Motion: beat vs silence (final build)

`P2-004-motion.py` measures the mean absolute RGB difference between consecutive 320-px
screenshots, taken about 100 ms apart, 12 per series. It reuses `P2-003-alpha-probe.py`'s PNG
decoder. Fractopia keeps animating in silence, but it moves more on the beat:

```
beat-r0        36.05
beat-r1        37.20
silence-r0     24.76
silence-r1     22.36
```

## Render time (`averageFrameRenderTime`, ms, sampled once a second for 30 s, same session)

| Build | Scene | min | median | max | skipped |
|---|---|---|---|---|---|
| final, attached | P2-004 Audio | 8.10 | 10.00 | 13.27 | 0 |
| final, None | P2-004 Audio | 6.57 | 9.52 | 13.27 | 0 |
| final, attached again | P2-004 Audio | 6.66 | 10.23 | 13.53 | 0 |
| final | spike `Scene` | 5.84 | 9.62 | 11.77 | 0 |
| P2-003 baseline (tone) | P2-004 Audio | 7.55 | 9.21 | 12.71 | 0 |
| P2-003 baseline (tone) | spike `Scene` | 7.74 | 8.80 | 13.31 | 0 |

Attached against None on the same build is within noise, so the audio path adds no measurable
render cost. Against the P2-003 build the median is +0.4 to +0.8 ms, within the min/max spread.
The machine was busier than during P2-003's own run (6.0–6.8 ms then), so compare rows from this
table only.

## Screenshots (final build, 960 px, all viewed)

- `P2-004-final-fresh-none-during-beat.png`: a new visualizer with Audio source = None while the
  fixture plays a beat and no other instance feeds the core. Black: the preset gets no audio.
- `P2-004-final-fresh-attached-during-beat.png`: the same visualizer about 12 s after choosing
  `P2-004 Beat`. The preset draws in full.
- `P2-004-final-attached-beat.png` / `-attached-silence.png` / `-attached-2-beat.png` /
  `-attached-2-silence.png`: attached, in beat and silence sections.
- `P2-004-final-muted-during-beat.png` / `-unmuted-during-beat.png`: muted, then unmuted.
- `P2-004-final-removed-source-during-beat.png`: after the chosen source was removed. No crash;
  the image keeps rendering.
- `P2-004-final-beat3-attached-during-beat.png` / `-beat3-removed-2s-later.png`: removal during a
  beat section.
- `P2-004-final-temp-viz-before-remove.png` / `-main-viz-after-temp-removals.png`: a second
  visualizer attached before its removal, and the main visualizer still rendering after five
  removals.
- `P2-004-final-none-beat.png` / `-none-silence.png`: the main visualizer on None after it had
  been fed. The image persists in the preset's feedback buffer and is not black.
- `P2-004-debug-reloaded-beat.png`: after the scene collection switch.

Screenshots alone cannot separate beat from silence, because presets keep animating (C-010). The
levels log and the motion numbers are the proof of reaction. The fresh None/attached pair is the
clearest still.

## Cleanup

`P2-004-audio.mjs cleanup` set `audio_source` to `""` and removed `P2-004 Beat` and the scene,
then switched to `Untitled`. The collection is back to its three original scenes and five inputs.
OBS was quit (`osascript`, C-027) and reopened on `Untitled`. The final plugin stays installed.
