# P2-007 evidence: auto-cycle, random order and blend

OBS 33.0.3 (OpenGL, Apple M1), 1080×1920 canvas at 30 fps, collection `synesthesia-spike`, program
scene `Scene`, using the spike visualizer. `<scratch>` is the P2-006 scratch folder outside the
repo:

- `presets-a`: the core's 10 mvp presets, their textures, and `zz broken.milk` (plain text, not a
  preset). That makes 11 entries, and the broken one sits in the cycle.
- `presets-one`: swarm only.

The build is installed as a clean `ditto` copy, byte-identical to `plugin/build_macos_arm64`
(`cmp`). `codesign --verify` passes and there are still 7 exports. The full plugin log is in
`P2-007-plugin-log.txt` (paths shown as `<scratch>`).

## Timer semantics observed

A commit re-arms the core timer at `now + soft_cut_seconds + preset_duration`, with
`preset_duration_rand = 0`. The request fires when that time passes. The prepare runs on the worker
(7–210 ms), and the commit lands on the next rendered frame. Commit to commit is therefore
`cycle + blend` plus about 30–60 ms:

| Settings | Commit-to-commit intervals (s) |
|---|---|
| cycle 10, blend 2 (run 1) | 12.191, **24.342**, 12.025, 12.071, 12.064, 12.241, 12.059, 12.067, 12.033, 12.085 |
| cycle 10, blend 2 (run 2) | 12.055, 12.067, 12.034, 12.084, 12.050, 12.033, 12.064, 12.045, 12.057, **24.148**, 12.020 |
| cycle 3, blend 0.5, random | 3.584, 3.550, 3.585, …, **7.134**, … 3.550 (24 intervals) |
| cycle 3, blend 0.5, one preset | 3.534, 3.568, 3.566, 3.534, 3.566 |

The bold intervals are a broken preset being skipped. The prepare fails, the core releases the
latch and restarts the timer, and the next request moves past the broken file. The gap is two
periods. In the new-source case (defaults 16 s and 2.7 s, no folder), the first request came
18.77 s after create (`11:11:34.092` to `11:11:52.859`).

## Sequential order wraps, broken preset skipped (cycle 10, blend 2)

```
11:12:31.712: auto-cycle: every 10 s, blend 2.0 s, sequential order
11:12:31.747: preset committed: <scratch>/presets-a/shifter - swarm.milk (blend 0.0 s, 1.6 ms)
11:12:43.780: auto-cycle requested a preset (timer, blend 2.0 s)
11:12:43.781: auto-cycle picked the next preset: …/Stahlregen & Flexi + Geiss - Tiger No.5 (Random texture flow).milk
11:12:43.988: preset prepared: …/Tiger No.5 … (206.9 ms)
11:12:44.022: preset committed: …/Tiger No.5 … (blend 2.0 s, 11.4 ms)
11:12:56.213: preset committed: …/Unchained - Fuzzy Sciences.milk (blend 2.0 s, 2.5 ms)
11:13:08.253: auto-cycle requested a preset (timer, blend 2.0 s)
11:13:08.253: auto-cycle picked the next preset: <scratch>/presets-a/zz broken.milk
11:13:08.254: preset failed to prepare: <scratch>/presets-a/zz broken.milk (not a MilkDrop preset: no [preset00] section or known key)
11:13:20.315: auto-cycle requested a preset (timer, blend 2.0 s)
11:13:20.316: auto-cycle picked the next preset: …/Eo.S. + Geiss - glowsticks v2 02 (Relief Mix).milk
11:13:20.555: preset committed: …/glowsticks v2 02 … (blend 2.0 s, 10.3 ms)
… CAT Scan, dark side of the moon, Now And Later, Goody, Fractopia, Jester's Calling 2 …
11:14:45.115: preset committed: <scratch>/presets-a/shifter - swarm.milk (blend 2.0 s, 2.4 ms)
11:14:57.200: preset committed: …/Tiger No.5 … (blend 2.0 s, 21.9 ms)
```

The order follows the case-insensitive sorted list, wraps from the last file to the first, and
comes back round to swarm and Tiger. The first commit is the manual pick, and it commits as a hard
cut because nothing was committed before it (see *Manual picks* below).

## Live changes, no restart (TC-017)

Script phase `live`: a manual pick of Fractopia at cycle 10 and blend 2. After 5 s, cycle 5 and
blend 1. After 30 s, cycle 0. After 40 s, cycle 10 and blend 2 again.

```
11:19:31.085: preset committed: …/Rovastar - Fractopia (Upspoken Mix).milk (blend 2.0 s, 1.7 ms)
11:19:36.050: auto-cycle: every 5 s, blend 1.0 s, sequential order
11:19:43.087: auto-cycle requested a preset (timer, blend 1.0 s)         <- 12.0 s after the commit: the armed timer (10 + 2) is kept
11:19:43.119: preset committed: …/Rovastar - Jester's Calling 2.milk (blend 1.0 s, 2.4 ms)
11:19:49.186: preset committed: …/shifter - swarm.milk (blend 1.0 s, 2.2 ms)   <- 6.07 s
11:19:55.271: preset committed: …/Tiger No.5 … (blend 1.0 s)                    <- 6.09 s
11:20:01.286: preset committed: …/Unchained - Fuzzy Sciences.milk (blend 1.0 s)  <- 6.02 s
11:20:06.049: auto-cycle: off, blend 1.0 s
                                                                          <- 40 s: no request
11:20:46.118: auto-cycle: every 10 s, blend 2.0 s, sequential order
11:20:46.125: auto-cycle requested a preset (timer, blend 2.0 s)         <- at once: the armed timer had already passed
11:20:46.128: preset failed to prepare: <scratch>/presets-a/zz broken.milk (…)
11:20:58.237: preset committed: …/glowsticks v2 02 … (blend 2.0 s, 19.2 ms)  <- 12.1 s later
11:21:10.287: preset committed: …/Eo.S. + Phat - CAT Scan (Nirvana flux).milk (blend 2.0 s, 2.1 ms)
```

- A new cycle or blend applies on the next rendered frame.
- The interval already armed for the current preset follows the old values, as the core header
  documents.
- With cycle 0 there are no requests.
- Turning the cycle back on after the armed time has passed fires one request at once.

## Random order (TC-018)

Script phase `random` (cycle 3, blend 0.5, random on) ran for 90 s. It made 26 requests: 25 commits
and 1 broken skip. No preset followed itself, including across the skip (swarm, broken skipped,
then Jester's Calling 2):

```
11:21:26 swarm (manual) → CAT Scan → glowsticks → Fractopia → Tiger → Fuzzy Sciences → CAT Scan →
glowsticks → Goody → Fuzzy Sciences → swarm → dark side → Jester → glowsticks → swarm →
[zz broken: failed to prepare] → Jester → Fractopia → Now And Later → CAT Scan → swarm → Goody →
Fuzzy Sciences → Jester → Fuzzy Sciences → glowsticks → swarm
```

The log scan reported `REPEAT` for no pair of consecutive commits. A later window (`11:23:02`) shows
the second exclusion at work. The broken file failed, and the next random pick excluded both the
current preset and the file that just failed.

## Single preset

With `presets-one` (swarm only), each request picks swarm again in both modes. Swarm is
re-committed with the blend every 3.53–3.57 s, so the latch clears and the timer keeps running
(`auto-cycle picked the random preset: <scratch>/presets-one/shifter - swarm.milk`).

## Manual picks use the blend setting

Script phase `manual`: cycle 0, blend 6, swarm shown, then pick Fractopia.

```
11:23:43.555: preset committed: …/Rovastar - Fractopia (Upspoken Mix).milk (blend 6.0 s, 1.6 ms)
11:23:52.320: auto-cycle: off, blend 0.0 s
11:23:52.355: preset committed: <scratch>/presets-a/shifter - swarm.milk (blend 0.0 s, 1.5 ms)
```

| Screenshot | Shows |
|---|---|
| `P2-007-manual-before-swarm.png` | Swarm before the pick |
| `P2-007-manual-blend-1s.png` | 1.2 s into the 6 s blend: swarm's red kaleidoscope merging into Fractopia's warp |
| `P2-007-manual-blend-3s.png` | 3.4 s in: mostly Fractopia, with the transition pattern still visible |
| `P2-007-manual-blend-5s.png` | 5.6 s in: almost fully Fractopia |
| `P2-007-manual-after-fractopia.png` | Fractopia after the blend |
| `P2-007-manual-hard-cut-300ms.png` | Blend 0, pick swarm, 300 ms later: swarm geometry at once. The feedback buffer still carries Fractopia's colours, which is normal for a MilkDrop hard cut |

The first commit after a source is created or a collection loads is a hard cut, because nothing
was committed before it. That shows in the reload log below as `blend 0.0 s` on the restored
preset.

## Skipped frames (GetStats)

| Window | `renderSkippedFrames` delta | Frames |
|---|---|---|
| Run 1, sequential cycling, 150 s, unsampled, right after launch | +6 (2 → 8) | 4,501 |
| Baseline, cycle 0, 90 s, sampled every 250 ms | 0 | 2,707 |
| Cycling at 10/2, 150 s, sampled every 250 ms (12 commits, including glowsticks and Goody) | 0 | 4,507 |
| Live-change phase, 105 s | 0 (8 → 8) | 3,153 |
| Random at 3/0.5, 90 s (25 commits) | 0 (8 → 8) | 2,701 |
| Manual blend phase | 0 (8 → 8) | 280 |

The six frames skipped in run 1 were not reproduced in the five windows that followed, which cover
about 9 minutes and 50+ commits, some of the same presets among them. Run 1 had no per-sample
data, so its cause was not isolated. It came in the first minutes after an OBS launch.
`averageFrameRenderTime` stayed between 4.4 and 11 ms against a 33 ms budget.

## Teardown while cycling (TC-024 subset)

`P2-007-remove.mjs` created 10 temporary visualizers in `Scene`, each with cycle 1 s and blend
0.2 s, alternating sequential and random. It removed each one 2.5–3.9 s after creating it, while
requests were firing (16 requests in total). Each removal logged `preset worker stopped (0.1 ms)`,
with no crash and no inputs left over.

## Persistence, leaks and GL errors

- After a quit and relaunch, the saved `preset_folder`, `preset`, `cycle_seconds`,
  `blend_seconds` and `random_order` came back, and the saved preset committed on load:

  ```
  11:25:56.005: sv_init_gl and sv_create succeeded (1920x1080)
  11:25:56.014: preset prepared: <scratch>/presets-a/shifter - swarm.milk (8.8 ms)
  11:25:56.409: auto-cycle: off, blend 0.0 s
  11:25:56.410: preset committed: <scratch>/presets-a/shifter - swarm.milk (blend 0.0 s, 1.0 ms)
  ```

- Quit 1 (after every test): `preset worker stopped`, `plugin unloaded`,
  `Number of memory leaks: 0`.
- Quit 2 (after cleanup, on `Untitled`): `Number of memory leaks: 0`.
- GL error lines (`GL_INVALID…`, `device_load_texture … failed`, `… failed (GL)`): 0 in both
  session logs. No `preset failed to commit`.
- The only plugin warnings were the 7 expected `preset failed to prepare: …/zz broken.milk`.

## Properties dialog

`P2-007-props.png` was opened with `OpenInputPropertiesDialog` and captured with
`screencapture -l`. It shows "Change preset every (0 = off)" with the ` s` suffix under the preset
list. "Blend time" and "Random order" sit below the fold of the fixed-size dialog. Scrolling would
need GUI automation, which is not allowed. Both values were set and read back through the
websocket.

## Cleanup

- The spike visualizer's settings were restored exactly with `overlay: false`:
  `{"audio_source":"","preset_path":""}`.
- The 10 temporary inputs were removed, and `GetInputList` shows only the collection's original
  inputs.
- `Untitled` was restored. OBS was quit with 0 leaks and reopened on `Untitled`.
- The P2-007 build stays installed.

## Scripts

All scripts take `OBS_WS_CREDS=<creds file>` and import `./ws-session.mjs` (rename
`P2-007-ws-session.mjs`).

- `P2-007-cycle.mjs <sequential|live|random|one|manual|cleanup> <scratch> <shots dir>`
- `P2-007-skips.mjs <scratch> <baseline s> <cycling s> [cycle] [blend]`
- `P2-007-remove.mjs <scratch>`
