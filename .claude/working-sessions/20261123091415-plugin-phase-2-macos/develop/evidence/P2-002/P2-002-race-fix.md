# P2-002 evidence: thread-race fix, delta reset and cap

Run on 2026-11-23, macOS (Apple Silicon), OBS 33.0.3 on the OpenGL renderer, canvas 1080×1920 at
30 fps, scene collection `synesthesia-spike`, program scene `Scene` (the visualizer at 960×540 next
to `SPK-005 Image`, `SPK-005 Colour` and `SPK-005 Text`). Driven only through obs-websocket. The
user's `Untitled` collection was restored at the end.

## Builds

Three builds of `plugin/src/visualizer-source.cpp` with `cmake --build --preset macos-arm64`, all
`** BUILD SUCCEEDED **`, no warning from `plugin/src`, `nm -gU` exactly 7 exports, arm64 only:

| Build | Source | Use |
|---|---|---|
| spike + detector | `HEAD` (the spike) + `P2-002-add-detector.py` | baseline: proves the detector sees the race |
| final + detector | this task's source + `P2-002-add-detector.py` | 5-minute stress |
| final | this task's source, no instrumentation | installed and left installed |

`P2-002-add-detector.py` is the scratch-only re-entrancy detector (C-009): an atomic in-flight
counter in `with_core_gl_state` and in `push_synthetic_tone` (the spike's tick-side `sv_*` call),
a warning per overlap (first 20) and a total in `visualizer_destroy`. It was applied to scratch
copies only. The final source and the installed binary contain none of it:
`strings … | grep -c re-entrancy` = 0 on the installed binary; its `__text` is identical to the
repo's `plugin/build_macos_arm64` build (the bytes differ only by the ad-hoc re-sign).

## Re-entrancy detector

Spike + detector, 60 s stress (`log-before`):

```
09:36:54.938: [synesthesia-vibe-obs] [re-entrancy detector] overlap #1 in with_core_gl_state (thread 0x16b537000, 1 already in flight)
09:37:01.005: [synesthesia-vibe-obs] [re-entrancy detector] overlap #2 in with_core_gl_state (thread 0x16b537000, 1 already in flight)
09:37:03.637: [synesthesia-vibe-obs] [re-entrancy detector] overlap #3 in with_core_gl_state (thread 0x16b537000, 1 already in flight)
09:37:48.883: [synesthesia-vibe-obs] [re-entrancy detector] destroy: 7199 core calls, 3 overlaps
09:37:49.123: Number of memory leaks: 0
```

The spike's race is real and the detector catches it: an off-thread `video_render`
(`GetSourceScreenshot`) entered `sv_render_frame` while `video_tick` was inside
`sv_push_pcm_float`.

Final + detector, off-air test + 5-minute stress + second off-air test, about 7 minutes on air
(`log-after-detector`):

```
09:37:51.967: [synesthesia-vibe-obs] sv_init_gl and sv_create succeeded (1920x1080)
09:37:52.068: [synesthesia-vibe-obs] preset loaded: …/presets/Rovastar - Fractopia (Upspoken Mix).milk
09:44:40.599: [synesthesia-vibe-obs] [re-entrancy detector] destroy: 22450 core calls, 0 overlaps
09:44:40.718: [synesthesia-vibe-obs] plugin unloaded
09:44:40.817: Number of memory leaks: 0
```

## Stress loop (`P2-002-stress.mjs`)

`GetSourceScreenshot` of the visualizer (PNG, 320 wide) every 50 ms with at most 3 in flight; every
500 ms one of the image, colour and text items toggles; every 10 s the visualizer item hides for
0.7 s and shows again (exercises `show`/`activate`).

| Run | Duration | Requests | OK | Failed | Rate | Latency p50 / p95 / max | Toggles (others / visualizer) | avg render time before → after |
|---|---|---|---|---|---|---|---|---|
| spike + detector | 61.3 s | 1160 | 1160 | 0 | 18.9/s | 14 / 22 / 163 ms | 113 / 5 | 6.55 → 5.36 ms |
| final + detector | 302.1 s | 5802 | 5802 | 0 | 19.2/s | 13 / 22 / 112 ms | 560 / 28 | 8.94 → 5.66 ms |
| final (smoke) | 61.4 s | 1159 | 1159 | 0 | 18.9/s | 10 / 20 / 56 ms | 113 / 5 | 5.41 → 8.74 ms |

`GetStats` in every run: `activeFps` 30.0, `renderSkippedFrames` flat (1 → 1, 2 → 2, 2 → 2),
`averageFrameRenderTime` 5.4–8.9 ms, within the spike's 6.9–10.3 ms range. No crash.

## GL errors

`grep -c -i 'gl[A-Za-z]* failed\|GL_INVALID\|device_load_texture (GL) failed'` = 0 in all three
OBS logs (spike + detector, final + detector, final). The only `Error` lines are from the
`BS-008 Browser` source (its local page server is not running), unrelated.

## Off-air (`P2-002-offair.mjs`)

Hide the visualizer item for 12 s (15 s in the second run), then `GetSourceScreenshot` of the
hidden visualizer, show it, screenshot at once and after 1 s, then the whole scene. Figures are
mean RGB of the 960×540 PNG and the share of pixels with any channel above 16.

| Build | Hidden 12–15 s | Just shown | Shown + 1 s |
|---|---|---|---|
| spike | `before-2-hidden-12s`: mean (5.7, 3.4, 5.6), 0.0 % non-black, **black** | `before-3-just-shown`: 0.1 %, **black** | `before-4-shown-1s`: 86.1 % |
| final + detector | `after-2-hidden-12s`: mean (227, 227, 227), 89.5 % | `after-3-just-shown`: 89.5 % | 89.5 % |
| final + detector, after the stress | `after-stress-2-hidden-15s`: mean (213, 186, 202), 86.3 % | `after-stress-3-just-shown`: 86.1 % | 87.8 % |
| final | `final-2-hidden-12s`: mean (107, 157, 228), 89.7 % | `final-3-just-shown`: 89.7 % | 89.4 % |

The hidden and just-shown frames of the final build continue the on-air frame (same shapes,
`final-1-on-air` → `final-2-hidden-12s` → `final-3-just-shown`), so the animation resumes without a
jump; the spike's frames were near-black after the gap. Every screenshot was opened and looked at.
`*-5-scene.png` show the image, colour and text sources drawing correctly next to the visualizer.

## Final build, restore and quit (`log-final`)

```
09:44:43.510: [synesthesia-vibe-obs] renderer check passed (OpenGL, device type 1)
09:44:43.717: [synesthesia-vibe-obs] sv_init_gl and sv_create succeeded (1920x1080)
09:46:34.609: Switched to scene collection 'Untitled' (Untitled.json)
09:46:37.954: [synesthesia-vibe-obs] plugin unloaded
09:46:38.019: Number of memory leaks: 0
```

OBS was quit with `osascript -e 'tell application "OBS" to quit'` (C-027; prints the harmless
`-128`) and reopened afterwards on `Untitled`. The final plugin stays installed for later tasks.

## `sv_*` reachability (final source)

| Call | Enclosing function | Reached from |
|---|---|---|
| `sv_push_pcm_float` | `push_synthetic_tone` | `render_core_frame` ← `visualizer_video_render` |
| `sv_load_preset`, `sv_last_error` | `load_pending_preset` | `render_core_frame` ← `visualizer_video_render` |
| `sv_render_frame` | `render_core_frame` | `visualizer_video_render` |
| `sv_init_gl`, `sv_create`, `sv_last_error(nullptr)` | `visualizer_create` | under `obs_enter_graphics` |
| `sv_destroy` | `visualizer_destroy` | under `obs_enter_graphics` |

`visualizer_video_tick`, `visualizer_update` and `visualizer_reset_pending_delta`
(`show`/`activate`) contain no `sv_*` call.

Screenshots (`P2-002-*.png`) and the `.mjs` helpers sit next to this file locally; per the
session's ignore rules only this `.md` and `P2-002-add-detector.py` are committed.
