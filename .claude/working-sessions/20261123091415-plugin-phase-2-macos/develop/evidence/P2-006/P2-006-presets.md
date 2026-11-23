# P2-006 evidence: preset folder, list, off-thread prepare and commit

OBS 33.0.3 (OpenGL, Apple M1), 1080×1920 canvas at 30 fps, collection `synesthesia-spike`, program
scene `Scene` (the visualizer next to the SPK-005 image, colour and text sources). `<scratch>` is a
scratch folder outside the repo:

- `presets-a`: the core's 10 `resources/mvp-presets/*.milk`, the 23 `resources/beatdrop/textures`
  flattened next to them, and `zz broken.milk` (plain text, not a preset).
- `presets-b`: swarm and Fractopia only.
- `presets-empty`: one `readme.txt`. `presets-missing` does not exist.
- `presets-heavy`: swarm and Fractopia padded with 60,000 `per_frame_N` lines
  (`P2-006-make-heavy.py`), so that one prepare takes about 400 ms.

The installed bundle was replaced by a clean `ditto` of the build. It holds only `locale/`, with no
`presets/` or `textures/` dev copy (C-003), so the run matches a release install.

## Collection load with the old `preset_path` setting (C-021, C-004)

```
10:52:55.357: [synesthesia-vibe-obs] sv_init_gl and sv_create succeeded (1920x1080)
10:52:55.358: [synesthesia-vibe-obs] preset folder not set; choose a folder of .milk presets
```

The saved `preset_path: ""` is ignored. The list is empty (`GetInputPropertiesListPropertyItems`
→ `[]`). `P2-006-unset-fallback.png` shows what the core draws with no preset committed: an opaque
grey wave and mesh image, not garbage and not black.

## Choosing a folder, picking presets (TC-013)

`preset_folder = presets-a` with no `preset` lists 11 entries, sorted case-insensitively, labelled
without `.milk`, value = file name:

```
Eo.S. + Geiss - glowsticks v2 02 (Relief Mix) … shifter - swarm, Stahlregen & Flexi + Geiss - Tiger No.5 …,
Unchained - Fuzzy Sciences, zz broken
```

The first entry is used when `preset` is unset or not in the folder. Changing the folder to
`presets-b` lists 2 entries.

```
10:53:31.694: preset prepared: <scratch>/presets-a/Eo.S. + Geiss - glowsticks v2 02 (Relief Mix).milk (209.1 ms)
10:53:31.730: preset committed: <scratch>/presets-a/Eo.S. + Geiss - glowsticks v2 02 (Relief Mix).milk (11.8 ms)
10:53:34.661: preset prepared: <scratch>/presets-a/shifter - swarm.milk (9.4 ms)
10:53:34.686: preset committed: <scratch>/presets-a/shifter - swarm.milk (1.2 ms)
10:53:37.792: preset prepared: <scratch>/presets-a/Rovastar - Fractopia (Upspoken Mix).milk (6.9 ms)
10:53:37.819: preset committed: <scratch>/presets-a/Rovastar - Fractopia (Upspoken Mix).milk (1.4 ms)
10:53:40.891: preset prepared: <scratch>/presets-b/Rovastar - Fractopia (Upspoken Mix).milk (6.6 ms)
10:53:40.919: preset committed: <scratch>/presets-b/Rovastar - Fractopia (Upspoken Mix).milk (1.4 ms)
```

`prepared` is logged by the worker and `committed` by `video_render`. The commit time is the time
spent inside `with_core_gl_state` on the graphics thread. Screenshots:
`P2-006-folder-a-implicit-first.png` (glowsticks), `P2-006-pick-swarm.png`,
`P2-006-pick-fractopia.png`, `P2-006-folder-b-fractopia.png`.

The properties dialog was opened with `OpenInputPropertiesDialog` and captured with
`screencapture -l`:

- `P2-006-props-folder-set.png`: "Preset folder" (path plus Browse) and "Preset: shifter - swarm".
- `P2-006-props-folder-unset.png`: an empty folder, the list hidden, and the warning-styled hint
  "Choose a folder of .milk presets."

## Broken preset (TC-015)

```
10:54:06.488: preset committed: <scratch>/presets-a/Rovastar - Fractopia (Upspoken Mix).milk (2.9 ms)
10:54:09.586: preset failed to prepare: <scratch>/presets-a/zz broken.milk (not a MilkDrop preset: no [preset00] section or known key)
```

Fractopia keeps rendering: `P2-006-broken-before.png` and `P2-006-broken-after-keeps-previous.png`
show the same preset 3 s apart. There was no crash.

## Empty, missing and unset folder (C-004)

```
10:54:12.752: preset folder has no .milk presets: <scratch>/presets-empty
10:54:14.352: preset folder not found: <scratch>/presets-missing
10:54:15.952: preset folder not set; choose a folder of .milk presets
```

There is one warning per change, and the list is `[]` each time. The current preset keeps
rendering: `P2-006-{empty,missing,unset}-folder-keeps-previous.png` all show Fractopia. A new
source with no folder renders the core's no-preset image (above).

## Unrelated settings no longer restart the preset

`audio_source` was switched whole_mix → none → whole_mix → none, then `audio_mix_track` 2 → 1:

```
10:54:20.619: audio mix connected: track 1 (44100 Hz stereo)
10:54:22.152: audio mix disconnected: track 1
10:54:22.152: audio source: none
10:54:23.651: audio mix connected: track 1 (44100 Hz stereo)
10:54:25.152: audio mix disconnected: track 1
10:54:25.152: audio source: none
```

No `preset …` line was logged. Before P2-006, every one of these logged `preset loaded` and
restarted the preset.

## Switching cost: old synchronous path against P2-006 (TC-014)

`P2-006-switch.mjs` takes three windows in a row: 15 s steady, then 30 s switching through the 10
good presets every 1000 ms with `SetInputSettings`, then 10 s after. It samples `GetStats` every
100 ms.

- **Old path:** the P2-005 build that was installed before this task, using
  `preset_path = <scratch>/presets-a/<file>` and `sv_load_preset` in `video_render`.
- **New path:** the P2-006 build, using `preset_folder` and `preset`.

| Build / folder | Window | Switches | Render ms median / p95 / max | Skipped frames | Frames |
|---|---|---|---|---|---|
| old (P2-005) / presets-a | steady | 0 | 6.39 / 8.48 / 8.48 | 0 | 454 |
| old (P2-005) / presets-a | switching | 30 | 5.05 / 13.00 / 13.13 | **16** | 901 |
| old (P2-005) / presets-a | after | 0 | 3.13 / 5.89 / 5.89 | 0 | 303 |
| P2-006 / presets-a | steady | 0 | 5.08 / 9.32 / 9.32 | 0 | 451 |
| P2-006 / presets-a | switching | 30 | 4.86 / 7.36 / 8.27 | **0** | 904 |
| P2-006 / presets-a | after | 0 | 3.24 / 4.48 / 5.37 | 0 | 303 |
| P2-006 / presets-heavy (≈400 ms prepares) | switching | 30 | 3.06 / 6.19 / 6.20 | **0** | 902 |

`averageFrameRenderTime` is a smoothed average, so the skipped-frame count is the clearer signal.
The heavy prepares cost about 400 ms each. On the old path, each of those would block the graphics
thread for the whole parse. Here they run on the worker, and commits take 0.9–1.7 ms. The slowest
commit seen in the run was glowsticks, at 11.8–17.8 ms (GL program build), which is still under
the 33 ms frame budget.

## Rapid switching drops stale prepares

Light presets every 30 ms, 20 requests: `SetInputSettings` reaches `update` once per tick, and
prepares (6–17 ms) finish within a tick, so most requests commit. The coalescing still shows:
requests that were already superseded when the worker reached them are skipped without a
prepare. There were 0 skipped frames, and the final commit is the last request.

Heavy presets, 11 requests 100 ms apart (`P2-006-stale.mjs`):

```
10:55:21.711: preset prepared but superseded, dropped: <scratch>/presets-heavy/heavy fractopia.milk (424.3 ms)
10:55:22.110: preset prepared but superseded, dropped: <scratch>/presets-heavy/heavy fractopia.milk (397.3 ms)
10:55:22.520: preset prepared but superseded, dropped: <scratch>/presets-heavy/heavy fractopia.milk (399.7 ms)
10:55:22.917: preset prepared: <scratch>/presets-heavy/heavy fractopia.milk (397.2 ms)
10:55:22.920: preset committed: <scratch>/presets-heavy/heavy fractopia.milk (1.1 ms)
```

Only the last request (request 11, heavy fractopia) was committed. The stale handles were freed on
the worker. Requests for heavy swarm were skipped because swarm was already the current target.
Skipped frames: 2 → 2.

## Removing the source while a prepare is in flight (TC-016)

`P2-006-presets.mjs remove` / `remove-heavy` runs 20 iterations of the same steps:

1. Create `P2-006 Viz Temp N` in `Scene` with a preset.
2. Wait 50 or 400 ms, then switch its preset.
3. Wait 40 ms, then `RemoveInput`.

The program scene is used because libobs prunes a removed source's scene item only while that scene
renders. In an unrendered scene the source lives on until the scene goes.

| Run | Creates | `preset worker stopped` | Worker stop time | Dropped as superseded | Leftover inputs |
|---|---|---|---|---|---|
| presets-a | 20 | 20 | 0.0–0.1 ms; 130–166 ms when a glowsticks prepare was in flight | 5 | none |
| presets-heavy | 20 | 20 | 274–441 ms (always in flight) | 20 | none |

```
11:00:00.125: preset prepared but superseded, dropped: <scratch>/presets-heavy/heavy swarm.milk (403.1 ms)
11:00:00.126: preset worker stopped (285.8 ms)
```

`destroy` waits for the in-flight prepare, drops its result (`preset_closing`), and only then
calls `sv_destroy`. There was no crash.

libobs runs `info.destroy` on its destruction task thread (`obs_source_destroy` →
`destruction_task_thread`), and the worker is joined before `obs_enter_graphics`. So the wait
does not hold the graphics lock. Skipped frames during the churn were the same for both runs: 48
for the light run and 49 for the heavy run, about 2.4 per create/destroy cycle. They come from
`sv_create`/`sv_destroy` under the graphics lock, as in P2-005, not from the worker join.

## Persistence, leaks, GL errors

- Quit with `preset_folder = presets-a`, `preset = shifter - swarm.milk`, then relaunch:

  ```
  11:02:18.572: sv_init_gl and sv_create succeeded (1920x1080)
  11:02:18.582: preset prepared: <scratch>/presets-a/shifter - swarm.milk (9.7 ms)
  11:02:18.676: preset committed: <scratch>/presets-a/shifter - swarm.milk (1.5 ms)
  ```

  See `P2-006-reload-saved-swarm.png`.
- Quit 1, after every test above, including 40 temporary visualizers:
  `preset worker stopped (0.1 ms)`, `plugin unloaded`, `Number of memory leaks: 0`.
- Quit 2, after restoring the spike visualizer's settings and switching to `Untitled`:
  `Number of memory leaks: 0`.
- GL error lines (`GL_INVALID…`, `device_load_texture … failed`, `… failed (GL)`): 0 in both
  session logs.

## Cleanup

- The spike visualizer's settings were restored exactly (`{"audio_source": "", "preset_path":
  ""}`, `overlay: false`).
- The temporary inputs and the `P2-006 Props` scene were removed, and `GetInputList` shows only the
  collection's original inputs.
- `Untitled` was restored. OBS was quit with 0 leaks and reopened on `Untitled`.
- The P2-006 build stays installed, byte-identical to `plugin/build_macos_arm64` (`cmp`), with no
  dev presets or textures inside the bundle.
