# Browser Source startup — from URL to reacting visuals

> Part of the how-x-works corpus — see [`../HOW-X-WORKS.md`](../HOW-X-WORKS.md) for the index.
> User-facing parameters and troubleshooting: `browser-source/README.md`. Layer overview:
> `docs/architecture.md`.

What happens when OBS (or a browser) loads `http://localhost:8080/?…`. All citations are
`browser-source/obs-page.js` unless noted.

## 0. Before load: `dist/` must exist

`browser-source/fetch-dist.sh` installs the core web dist, presets, textures, `LICENSE`, `NOTICE`
and `dist/resources/manifest.json` (`docs/integrations.md`). The page has no fallback for a missing
`dist/` other than a clear error (step 2).

## 1. Page entry and settings

`browser-source/index.html` loads `obs-page.js` as a module; `main()` (`:307-321`) starts.
`readSettings` (`:76-86`) parses the query once: `cycle` / `fps` through `readNonNegative`
(`:47-55`), `blend` through `readClamped` 0–10 (`:57-65`), `audio` through `readAudio` (`:67-74`).
Bad values `warn` (`obs-page:` prefix) and fall back to defaults (30 / 2 / 60 / `default`).

## 2. Load the core

`importDist` (`:96-98`) dynamically imports `./dist/synesthesia-vibe.js` inside `stage(...)`
(`:88-94`), so a missing dist throws `could not load ./dist/synesthesia-vibe.js, run
browser-source/fetch-dist.sh`. Then `loadSynesthesiaModule()`, and `loadPresets` (`:100-107`) reads
the manifest through `ResourceLoader`; an empty preset list is fatal.

## 3. Resolve the playlist

`resolvePresets` (`:139-162`) splits `preset=` on commas and runs `findMatch` (`:124-137`) on each
item: normalised exact match first, else the first substring match (warns if several). Unmatched
items are skipped with a warning; if none match, all presets play. Back in `main`, a one-preset
list forces `cycle` to 0.

## 4. Renderer and cycling

`createView` (`:109-114`) calls `SynesthesiaCanvas.create` with `maxFps`. `createPlaylist`
(`:173-189`) returns `loadNext`, which advances the index and tries each preset at most once per
call. `logSettings` (`:205-211`) prints the `obs-page: settings …` line. `startCycling`
(`:191-203`) configures the engine timer (`presetDuration: cycle`, `softCutSeconds: blend`, no hard
cuts, random blend pattern) and answers each `onPresetRequest` with `loadNext`. Result: a change
every `cycle + blend` seconds. `await loadNext(0)` shows the first preset with no blend.

Failure: a preset that fails to compile is logged and the next one is tried; if a whole pass fails,
`no preset in the list could be loaded` reaches `main().catch(reportError)`.

## 5. Audio (after the first preset)

`startAudio` (`:282-305`):

1. `audio=none` → log and return. No secure context / AudioWorklet → warn and return.
2. `unlockDeviceLabels` (`:213-220`) calls `getUserMedia` once and stops the tracks, so labels are
   visible. Failure throws `audio unavailable (<DOMException name>: …)`; visuals keep running. On
   macOS with OBS's mic permission off this is `NotFoundError`
   (`.claude/common-issues/browser-source.md`).
3. `chooseDevice` (`:228-249`) logs `obs-page: audio inputs …`, then picks the default input or the
   `findMatch` result on device labels; no match falls back to the default with a warning.
4. If a probe `AudioContext` starts suspended (desktop Chrome autoplay policy), wait for the first
   `pointerdown` / `keydown` (`:251-270`). OBS 33's CEF starts `running`, so no wait there.
5. `AudioInput.fromMicrophone(view, constraints)`; logs `obs-page: audio <label> at <rate> Hz`.
6. `watchAudioInput` (`:272-280`) polls `isRunning` every second and warns once when the device
   stops. The page never retries; reload the source.

## Outcome

Visuals render from step 4 on, whatever happens to audio. While OBS hides the source, the page
stops rendering and the preset timer pauses; showing it resumes the same preset.

**Analysis date:** 2026-11-23 (core web dist `0.7.0`, OBS 33.0.3).
