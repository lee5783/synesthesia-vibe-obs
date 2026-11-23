# BS-005

## Subtasks executed

- `startAudio(sv, view, settings.audio)` runs after `await loadNext(0)` in `main()`. The step
  functions are:
  - `audio=none`: logs `audio off (audio=none)` and returns. No `getUserMedia` call is made.
  - `AudioInput.isMicrophoneSupported()` false (insecure context, no AudioWorklet): warns
    `audio capture needs getUserMedia, an AudioWorklet and a secure context (https or localhost), running without audio`.
  - `unlockDeviceLabels()`: a throwaway `getUserMedia({ audio: true })` whose tracks are stopped
    straight away. On failure it throws
    `audio unavailable (<DOMException name>: <message>), running without audio`, which goes to
    `main().catch(reportError)`. The page keeps the DOMException name here (C-006 (e)), because the
    wrapper drops it.
  - `listAudioInputs()`: `enumerateDevices()`, filtered to `audioinput`.
  - `chooseDevice(inputs, audio)`:
    - Logs `audio inputs <label> | <label> …`. The virtual `default` / `communications` entries
      are left out of the list and out of matching, because they duplicate a real device.
    - `audio=default` uses `{}` constraints. Its logged label is the `default` entry's label.
    - Any other value goes through the shared matcher: case-insensitive, exact first, then
      substring, with a warning when several inputs match. A match passes
      `{ deviceId: { exact } }`.
    - No match: warns `no audio input matches "<x>", using <default label>; available: …` and
      falls back to the default input.
  - `startsSuspended()` / `nextGesture()` handle autoplay (C-008):
    - A probe `AudioContext` is created and closed straight away.
    - If the probe starts `suspended`, the page logs
      `the AudioContext is suspended by the autoplay policy, audio starts on the first click or keypress`.
      It then waits for the first `pointerdown` or `keydown`. The listeners are window
      capture-phase and removed through an `AbortController` after the first event. Nothing is
      added to the DOM.
  - `AudioInput.fromMicrophone(view, constraints)` runs inside `stage(...)`, so a failure logs
    `could not start audio from <label>, running without audio` with the `SynesthesiaError` as
    cause.
  - On success the page logs `audio <label> at <inputSampleRate> Hz` once. The dist logs its own
    `AudioInput start (microphone) …` line.
  - `watchAudioInput(input, label)`: a 1 s check on `input.isRunning`. When the wrapper stops
    itself (track `ended`, e.g. the device was unplugged), the page warns once
    `audio input <label> stopped, running without audio; reload the page to retry` and stops
    checking. Nothing is retried.
- `findPreset` became `findMatch(candidates, keys, item, kind, nameOf)`, so presets and audio
  labels share one matcher. The preset warning text is unchanged (checked by a regression run).

## Files changed

- `browser-source/obs-page.js` (extended)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md` (BS-005 section appended)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-BS-005.md` (new)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/BS-005/` (new; git-ignored by the evidence rule)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/session.log` (appended)

## What changed and why

This is PLAN Phase 1 item 4. The page now feeds the engine from the input named by `audio=`.
Failure handling follows C-006: unknown label → fallback; denied / no device → silent visuals and
an error line; unplugged → log, no retry; `none` → no capture. Autoplay follows C-008, with no
visible UI.

**Autoplay: why the audio start is deferred instead of resuming the context.** The plan said to
resume a suspended context on the first gesture. Probing Chrome 157 headless without the autoplay
flag showed two things:

- `AudioContext.resume()` stays **pending** until a gesture. A gesture by itself does not start
  the context; `resume()` has to be called again after it.
- `AudioInput.fromMicrophone` awaits `audioContext.resume()` internally and keeps its
  `AudioContext` private. Called before a gesture, it would hang, and the page has no handle to
  resume.

So the page checks autoplay with a probe context first. If the probe is suspended, the page calls
`fromMicrophone` only after the first click or keypress, which makes the wrapper's context start
`running`. The user sees the same result as in the plan (audio begins on the first click or
keypress, no UI). It needs no monkeypatching of the dist or of browser built-ins. OBS's CEF is
expected to allow autoplay, and in that case the probe is `running` and nothing waits.

**Default path also unlocks.** `audio=default` also makes the label-unlock call and logs the
input list. The plan only required this for named devices. Doing it for default too gives the
DOMException name on denial, a real label in the `audio …` line, and a console list a streamer
can copy a label from.

## Verification

- `node --check browser-source/obs-page.js` printed `syntax-ok`. The only `//` lines are the
  6 header lines.
- There is no build and no test suite for the page.
- I served the page with `python3 -m http.server 8095 --bind 127.0.0.1 --directory browser-source`
  and stopped it afterwards. I drove it with headless Chrome 157 over CDP:
  `node evidence/BS-005/bs-005-cdp-check.mjs <url> <out-prefix> '<actions JSON>' "<chrome flags>"`.
  - Actions: `shot`, `click`, `key`, `eval`.
  - Env: `BS005_PERMISSION=denied` (CDP `Browser.setPermission` for `microphone`),
    `BS005_INJECT=<js>` (runs before the page loads), and `BS005_FLAG=<one flag with spaces>`.
- No run logged an `UNCAUGHT` line.
- **Environment note:** `--use-file-for-fake-audio-capture` delivered pure silence on this macOS
  Chrome until I added `--disable-features=AudioServiceSandbox`, because the sandboxed audio
  service cannot read the file. Without the file, the default fake device's beep gave `pcmPeak` 1
  but zero engine levels.
- The WAV is 30 s of a 120 BPM kick, a 4 s 100→2000 Hz sweep and off-beat hats, made with
  `python3 -I evidence/BS-005/bs-005-make-beat-wav.py <scratch>/beat.wav`. The 5.7 MB file stays
  in the scratchpad.
- **Metric** (`bs-005-audio-metric.js`): wraps `SynesthesiaContext.prototype.pushPcm` and `render`
  for 3 s. It counts PCM frames, records the PCM peak, and samples `context.audioLevels()` 30 times.

## Evidence

All files are in `evidence/BS-005/`. PNGs are 1280×720. `BS-005-console.txt` collects every case,
with engine GL noise filtered out. Each case also has its own `*-console.txt`.

- **Visuals react, `audio=default` vs `audio=none`, same preset (`preset=fractopia`), fake WAV mic:**
  - `BS-005-fake-device.png` (= `BS-005-default-t5s.png`): blue water and white web shapes filling
    the frame. `BS-005-default-t9.5s.png` shows a purple fractal web, so the picture keeps
    changing.
  - `BS-005-none-t5s.png` / `-t9.5s.png`: Fractopia's silent state, a thin grey ellipse on black.
  - Metric with audio on:
    `{"pcmFramesPerSecond":44885,"pcmPeak":0.862,"bass":{"min":0.122,"max":6.725},"mid":{"min":0.186,"max":1.98},"treble":{"min":0.328,"max":2.205}}`
  - Metric with audio off: all zero.
  - Console:
    ```
    obs-page: audio inputs Fake Audio Input 1 | Fake Audio Input 2
    AudioInput start (microphone): 44100 Hz -> 44100 Hz, 2 ch, running
    obs-page: audio Fake Default Audio Input at 44100 Hz
    ```
- **Label match.** `bs-005-gum-trace.js` is injected to log each `getUserMedia` constraint and the
  label of the track it returns.
  - `audio=input%202` → `deviceId: { exact: … }` → track `"Fake Audio Input 2"`, then
    `obs-page: audio Fake Audio Input 2 at 44100 Hz`.
  - `audio=FAKE%20audio%20input%201` (exact, case-insensitive) → `Fake Audio Input 1`.
  - `audio=fake` warns `audio input "fake" matches 2 audio inputs, using Fake Audio Input 1`.
  - Files: `BS-005-label-*`.
- **Unknown label, `audio=BlackHole`:** the warning
  `no audio input matches "BlackHole", using Fake Default Audio Input; available: Fake Audio Input 1 | Fake Audio Input 2`.
  The capture call then carries no `deviceId` and returns the track `Fake Default Audio Input`.
  `BS-005-unknown-label-t5s.png` shows swarm rendering.
- **`audio=none`:** `obs-page: audio off (audio=none)`, with no getUserMedia line and zero PCM.
  Files: `BS-005-none-*`.
- **Permission denied:**
  - Via CDP `Browser.setPermission microphone=denied`, with the fake-ui flag left off because it
    overrides the permission:
    `obs-page: audio unavailable (NotAllowedError: Permission denied), running without audio`.
  - `BS-005-denied-t5s.png` / `-t9.5s.png`: the green swarm still renders. Metric: 0 PCM frames.
  - The same error appears when headless dismisses the prompt (`BS-005-no-prompt-ui-*`,
    `audio=BlackHole`).
- **Suspended context, click (no autoplay flag):**
  - Console: `the AudioContext is suspended by the autoplay policy, audio starts on the first click or keypress`.
  - Metric before the click: 0 frames. `BS-005-suspended-before-click.png` shows the silent
    ellipse.
  - After the CDP click at 7.8 s:
    `AudioInput start (microphone): … running`, then `obs-page: audio Fake Default Audio Input at 44100 Hz`.
    Metric: `pcmFramesPerSecond 44885`, `bass max 16.7`.
  - `BS-005-suspended-after-click.png` shows Fractopia reacting (a white and violet web).
  - `document.body.children` stays `CANVAS,SCRIPT`, so no UI was added.
- **Suspended context, keypress:** `BS-005-suspended-key-*` (`audio=input%201`). A CDP `keyDown`
  starts the capture of `Fake Audio Input 1`, and the metric goes non-zero.
- **Unplug (simulated):**
  - An `ended` event is dispatched on the capture track. The dist logs
    `AudioInput stop (microphone): pushed 147456 frames`. 0.7 s later the page logs
    `obs-page: audio input Fake Audio Input 2 stopped, running without audio; reload the page to retry`.
  - PCM then stays at 0 and nothing is retried.
  - `BS-005-unplug-after-unplug-later.png` (about 10 s later) shows the visuals still animating.
- **Insecure context:** `http://insecure.test:8095` is mapped to 127.0.0.1 with
  `--host-resolver-rules`, so `isSecureContext` is `false`. The page logs the "needs … a secure
  context" warning and `BS-005-insecure-t5s.png` shows it rendering.
- **Capture fails after lookup** (an injected `NotFoundError` on the deviceId call):
  `could not start audio from Fake Audio Input 1, running without audio SynesthesiaError: SV_ERR_IO: microphone unavailable: Requested device not found`.
  Swarm keeps rendering (`BS-005-start-fails-t5s.png`).
- **Preset regression** (`?preset=eo.s.,nope&audio=none`): the warnings `preset "eo.s." matches 3 presets, using …`
  and `no preset matches "nope", skipping it` are unchanged after the `findMatch` rename.

## Known limitations / follow-ups

- **No real-device run.** The plan's optional `BS-005-default-mic.png` (built-in mic) was not
  captured. Driving the real mic from a non-interactive session could raise a macOS TCC prompt on
  the user's screen. BS-008 covers the built-in mic inside OBS.
- **The unplug was simulated** by dispatching `ended` on the track. A real removal also fires
  `ended` in Chromium, but that was not exercised (TC-024 needs a USB or virtual device).
- **Unplug detection is a 1 s `isRunning` check.** The wrapper hides its track, and `devicechange`
  can fire before or after `ended`. The check is cheap and stops after the first hit.
- **The autoplay probe creates one extra short-lived `AudioContext` per page load.** OBS behaviour
  (expected `running`) is unverified until BS-008.
- **"Device unplugged → reload" is manual by design** (C-006). The page never retries.
- **Two `getUserMedia` opens per start.** The unlock call runs even for `audio=default` (see
  *What changed*). OBS/CEF grants both the same way.
- **The fake-WAV evidence needs `--disable-features=AudioServiceSandbox` on macOS Chrome.** Worth
  a `.claude/COMMON-ISSUES.md` entry, for document-writer or the orchestrator to decide.
- **Doc gaps for BS-007 / document-writer:**
  - the `audio=` values (label substring, `default`, `none`)
  - the `obs-page: audio inputs …` line as the way to find a label
  - the fallback warning
  - the click/keypress note for desktop Chrome testing
  - "reload the source after replugging"
  - `+` decodes to a space in labels too
- No test added (project default). If tests are wanted later, `chooseDevice` / `findMatch` are
  pure enough to unit test.

## Handoff for downstream review

- **Acceptance criteria addressed:**
  - With the fake device, the visuals react and the PCM frame count grows (about 44.9k frames/s).
  - A label substring picks that device, and the console shows the full label.
  - A nonexistent label falls back and lists the available labels.
  - Denied permission leaves the page rendering with a clear error that carries the
    DOMException name.
  - `audio=none` opens no capture.
  - A suspended context starts on the first click or keypress, with no UI added.
  - An unplug logs once and does not retry.
  - Test cases exercised: TC-019, TC-020 (fake labels), TC-021, TC-022, TC-023, TC-024
    (simulated), TC-025 and TC-029.
- **Areas to scrutinize:**
  - The deferred-start autoplay design in place of resume (rationale above).
  - The unlock call on the default path.
  - Excluding the `default` / `communications` virtual entries from matching.
  - Audio errors reach the console through `main().catch(reportError)` after rendering has
    already started, so rendering is unaffected.
- **How to run:**
  1. Serve `browser-source/`.
  2. Run `node <develop>/evidence/BS-005/bs-005-cdp-check.mjs "http://127.0.0.1:8095/?preset=fractopia&audio=default" /path/out '[{"at":5,"type":"shot","name":"t5s"}]' "--use-fake-ui-for-media-stream --use-fake-device-for-media-stream --disable-features=AudioServiceSandbox --use-file-for-fake-audio-capture=/path/beat.wav --autoplay-policy=no-user-gesture-required"`.
     Leave out the autoplay flag and add `{"at":6,"type":"click"}` for the resume path.
