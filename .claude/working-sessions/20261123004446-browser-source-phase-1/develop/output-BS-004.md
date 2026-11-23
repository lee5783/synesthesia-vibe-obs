# BS-004

## Subtasks executed

- `readSettings(location.search)` at the top of `main()`, with three readers:
  - `readNumber`: a missing parameter gives the default; a non-number (including empty) gives the
    default plus a warning.
  - `readNonNegative` (`cycle`, `fps`): a negative value gives the default plus a warning.
  - `readClamped` (`blend`): a finite value outside 0–10 is clamped, with a warning.
  - `readAudio`: empty gives `default`; `default` and `none` are lowercased; any other value is kept
    verbatim as a label for BS-005.
- `resolvePresets(presets, query)` follows C-005:
  - Split on `,`, trim each item, drop empty items.
  - `matchKey` lowercases, turns `+` into a space and collapses whitespace. An exact match on the
    basename (without `.milk`) wins; otherwise the first substring match in sorted order, with a
    warning when several match.
  - Unmatched items are skipped with a warning. If nothing matches, a warning is logged and all
    presets are used. The list keeps the order it was given in.
- `createView(..., fps)` passes `configuration: { maxFps: fps }` to `SynesthesiaCanvas.create`.
- `createPlaylist(view, presets)` returns `loadNext(blend)`. It moves through the list in order and
  wraps. A preset that fails is logged through `reportError` and the next one is tried. It gives up
  after one full pass with `no preset in the list could be loaded`.
- `startCycling(...)` makes these calls:
  - `setTransitionConfig({ softCutSeconds: blend, presetDuration: cycle, presetDurationRand: 0, hardCutsEnabled: false })`
  - `setBlendPattern(SynesthesiaContext.BLEND_PATTERN_RANDOM)`
  - `onPresetRequest`, which logs `preset request (<reason>)` and calls `loadNext` with blend 0 on
    `hardCut`, else with `suggestedBlendSeconds`.
- If only one preset resolves, the effective cycle is 0, so the engine timer is off.
- `logSettings` writes one `console.info` line giving the effective cycle, blend, fps, audio and
  preset list.
- The first preset loads through `loadNext(0)`.
- The old `main().catch` body is now `reportError(error)` and is reused by the playlist and the
  request handler. Its output is unchanged.

## Files changed

- `browser-source/obs-page.js` (extended)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md` (BS-004 section appended)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-BS-004.md` (new)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/BS-004/` (new: `bs-004-cdp-check.mjs`, PNGs, console `.txt`; all git-ignored by the evidence rule)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/session.log` (appended)

## What changed and why

PLAN Phase 1 item 3. The page now takes its preset list, auto-cycle, blend, frame cap and the audio
choice from the URL. It uses the engine's own timer, as the core sample does, so the page has no
`setInterval`. Defaults follow C-007 (`cycle=30`, `blend=2`, `fps=60`, `audio=default`, all presets
sorted) and C-016 (sequential order, random blend pattern, no hard cuts).

`settings.audio` is parsed but nothing reads it yet. BS-005 adds `startAudio(sv, view, settings)`
after `await loadNext(0)` in `main()`.

## Verification

- `node --check browser-source/obs-page.js` printed `syntax-ok`. The only `//` lines are the 6
  header lines, so the code has no comments.
- There is no build and no existing test suite for the page.
- I served the page with `python3 -m http.server 8080 --directory browser-source` (stopped
  afterwards) and drove it with headless Chrome over CDP:
  `node evidence/BS-004/bs-004-cdp-check.mjs <url> <out-prefix> <shot seconds,...> [fps] [chrome flags]`.
  `BS004_INJECT=<js>` adds a test hook before the page loads.
- No run logged an `UNCAUGHT` line.

## Evidence

All files are in `evidence/BS-004/`. PNGs are downscaled to 1280×720. Console times are measured
from the start of the script.

- **Single preset, `?preset=fractopia`.** Files: `BS-004-single-t6s.png`, `BS-004-single-t12s.png`,
  `BS-004-single-console.txt`.
  - Console: `settings cycle=off blend=2s fps=60 audio=default presets=Rovastar - Fractopia (Upspoken Mix)`,
    then `preset Rovastar - Fractopia (Upspoken Mix)`. There are no request lines in 12 s.
  - Both shots show Fractopia's silent state: a grey ellipse on black, inside the preset's own thin
    outer border (`ob_size=0.01` in the `.milk`).
- **Cycle, `?preset=fractopia,swarm&cycle=3&blend=1`.** Files: `BS-004-cycle-a.png` (= `-t3s`,
  Fractopia), `BS-004-cycle-b.png` (= `-t6s`, swarm, a blue/yellow/white kaleidoscope),
  `-t9s` (Fractopia again), `-t12s`, and `BS-004-cycle-console.txt`.
  ```
  [  1.1s] obs-page: settings cycle=3s blend=1s fps=60 audio=default presets=Rovastar - Fractopia (Upspoken Mix) | shifter - swarm
  [  1.1s] obs-page: preset Rovastar - Fractopia (Upspoken Mix)
  [  5.1s] obs-page: preset request (timer)
  [  5.1s] obs-page: preset shifter - swarm
  [  9.1s] obs-page: preset request (timer)
  [  9.1s] obs-page: preset Rovastar - Fractopia (Upspoken Mix)
  ```
- **Unmatched names and bad values, `?preset=nope,alsonope&blend=abc&cycle=-5&fps=xyz`.** Files:
  `BS-004-console.txt` (the plan's name; a copy of `BS-004-unmatched-bad-values-console.txt`) and
  `BS-004-unmatched-bad-values-t6s.png`, which shows glowsticks rendering (bronze relief).
  ```
  obs-page: cycle=-5 is negative, using 30
  obs-page: blend=abc is not a number, using 2
  obs-page: fps=xyz is not a number, using 60
  obs-page: no preset matches "nope", skipping it
  obs-page: no preset matches "alsonope", skipping it
  obs-page: preset=nope,alsonope matched nothing, using all presets
  obs-page: settings cycle=30s blend=2s fps=60 audio=default presets=<all 10, sorted>
  obs-page: preset Eo.S. + Geiss - glowsticks v2 02 (Relief Mix)
  ```
- **Partial match and the other way round, `?preset=fractopia,nope&cycle=abc&blend=-5`.**
  - Console: `cycle=abc is not a number, using 30`, `blend=-5 is outside 0-10, using 0`,
    `no preset matches "nope", skipping it`, then `settings cycle=off blend=0s …` (one preset left,
    so the timer is off).
  - Files: `BS-004-partial-match-*`.
- **Lenient names, `?preset=Eo.S. + Geiss,GLOWSTICKS,eo.s.`** (raw `+`, which decodes to a space).
  - All three resolve to glowsticks.
  - `eo.s.` logs `matches 3 presets, using Eo.S. + Geiss - glowsticks v2 02 (Relief Mix)`.
  - Files: `BS-004-plus-and-substring-*`.
- **fps cap, `?preset=swarm&fps=N`, 5 s window.** During the window the script wraps
  `SynesthesiaContext.prototype.render` (the same module instance the page uses) to count frames
  the engine actually renders. It also counts rAF ticks.
  - `fps=30`: `{"renders":151,"rendersPerSecond":30.2,"rafPerSecond":60.1}`
  - `fps=0` (logged `fps=uncapped`): `{"rendersPerSecond":60,"rafPerSecond":60}`
  - `fps=60`: `{"rendersPerSecond":60,"rafPerSecond":60}`
  - Files: `BS-004-fps-{30,0,60}-console.txt` and `-t4s.png` (swarm renders at each cap).
- **`cycle=0`, `?preset=fractopia,swarm&cycle=0&blend=0`, watched for 25 s.** One preset line and no
  request. The engine default (16 s plus a random 10 s) or a 30 s cycle with no blend would have
  fired inside 25 s only for the first. A full 60 s watch (TC-014) was not run. Files:
  `BS-004-cycle-off-*`.
- **Broken preset (TC-018), simulated.** The hook makes `preparePreset` throw for any Fractopia path.
  - `?preset=fractopia,swarm,glowsticks&cycle=2&blend=0`: the console shows
    `could not load preset Rovastar - Fractopia (Upspoken Mix)`, then `preset shifter - swarm`.
    Each later wrap repeats the skip: fractopia fails, swarm loads.
    `BS-004-broken-next-t2s.png` shows swarm rendering.
  - `?preset=fractopia` (every preset broken): one failure line, then
    `no preset in the list could be loaded`. It does not loop. The page stays black
    (`BS-004-broken-only-t5s.png`).

## Known limitations / follow-ups

- **Measured interval is `cycle + blend`, not `cycle`.** With `cycle=3`: blend 0 gave 3.0 s,
  blend 1 gave 4.0 s and blend 3 gave 6.0 s between requests. The first request also came
  `cycle + blend` after the first load, which used blend 0. The engine seems to add
  `softCutSeconds` to `presetDuration`, so each preset holds alone for `cycle` seconds after its
  blend ends.
  - I implemented the plan's wiring as written (`presetDuration: cycle`, the same as the core
    sample).
  - If "every `cycle` seconds" must mean start to start, use `presetDuration: max(cycle - blend, 0)`.
    That is a decision for the orchestrator or user. Either way BS-007's README should state it
    (with defaults, a change every ~32 s).
- **`fps=0` (uncapped) looks the same as `fps=60` on a 60 Hz headless display.** The measurement
  proves that the cap throttles (30 renders/s against 60 rAF/s). It cannot show "above 60".
- **Duplicates are kept.** `preset=a,a` reloads the same preset on each cycle. That is harmless,
  and the input-order rule did not ask for de-duplication.
- **`&` cannot be fixed.** A raw `&` in a name splits the query string, so it has to be typed as
  `%26`. A raw `+` works because `+` and spaces are equivalent for matching. This belongs in the
  BS-007 README.
- `blend=-5` clamps to 0 rather than falling back to 2. This is my reading of the plan's "clamp
  0–10". `cycle` and `fps` fall back to their defaults on negative values.
- Settings are read once at load. OBS reloads the page when the URL changes, so nothing needs to be
  live.
- Not checked in OBS (BS-008). Headless Chrome only.
- Gaps for the document-writer: the URL parameter table, the matching rules, the fallback warnings
  and the `obs-page: settings …` line should go into BS-007's README and later the `docs/` pages.
- No test added (project default). If anything gets a unit test, `resolvePresets`/`readSettings`
  are the pure functions worth testing.

## Handoff for downstream review

- **Acceptance criteria addressed:**
  - `?preset=fractopia` stays on Fractopia.
  - A two-preset list alternates and wraps with the blend applied.
  - `cycle=0` stays on the first preset (25 s watch).
  - `fps=30` renders at about 30/s.
  - Bad values fall back or clamp with warnings and the page keeps rendering.
  - A failing preset is skipped, with at most one pass.
  - The effective-settings line is logged.
  - TC-010, TC-012, TC-013, TC-016, TC-017 (partly) and TC-018 (simulated) were exercised. TC-011
    ran with shorter timings.
- **Areas to scrutinize:**
  - The cycle timing note above.
  - `readClamped` clamping a negative `blend`.
  - `matchKey` treating `+` as a space.
  - `createPlaylist` sharing `index` between the first load and timer requests. With latest-wins
    loads, a superseded load returns `null` and logs nothing.
- **How to run:** serve `browser-source/`, then
  `node <develop>/evidence/BS-004/bs-004-cdp-check.mjs "http://localhost:8080/?preset=fractopia,swarm&cycle=3&blend=1" /path/out 3,6,9`.
  Add `fps` as the fourth argument to measure render rate.
