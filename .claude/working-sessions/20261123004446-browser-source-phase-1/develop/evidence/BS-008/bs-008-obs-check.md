# BS-008 OBS check report (agent run, macOS)

Environment: OBS 33.0.3, obs-websocket 5.7.6, obs-browser 2.27.6 with CEF 150.0.7871.287, macOS 26,
Apple Silicon. OBS canvas 1080×1920 at 30 fps (left unchanged). Page served with
`python3 -m http.server 8080 --bind 127.0.0.1 --directory browser-source` (stopped afterwards),
`browser-source/dist/VERSION` 0.7.0.

OBS was driven only through the local obs-websocket (no GUI or accessibility automation, no clicks)
in the `synesthesia-spike` scene collection: a new scene `BS-008` with the input `BS-008 Browser`
(`browser_source`, URL `http://localhost:8080/?preset=fractopia,swarm&cycle=10&blend=2&audio=default`,
1920×1080, created with `CreateInput`). Screenshots come from `GetSourceScreenshot` (1280×720). The
page console was read over CDP through `--remote-debugging-port=9222`. Scripts:
`bs-008-obs-ws.mjs` (websocket requests; reads the local obs-websocket credentials from a file
named in an environment variable) and `bs-008-cdp-console.mjs` (console and `Runtime.evaluate`).
No OBS config file was edited. At the end the `Untitled` collection was made current again and OBS
was relaunched with a plain `open -a OBS`.

## Results

| Check | Result | Evidence |
|---|---|---|
| `open -a OBS --args --enable-media-stream` passes the flag | **Pass.** `ps` shows `OBS --enable-media-stream` (and `--remote-debugging-port=9222` when added). Quitting OBS first was needed each time. | process list in the session transcript |
| Browser hardware acceleration | **On.** OBS log: `Browser Hardware Acceleration: true`. WebGL2 works: `GL_VERSION=OpenGL ES 3.0 (WebGL 2.0 (OpenGL ES 3.0 Chromium))`. | `BS-008-obs-log.txt`, `BS-008-console.txt` |
| Renders a 1920×1080 source (TC-030) | **Pass.** The visualizer fills the source edge to edge. | `BS-008-obs-a.png`, `BS-008-obs-b.png` |
| Preset cycling, `cycle=10&blend=2` | **Pass.** `obs-page: preset request (timer)` every 12.0 s (`cycle + blend`): page load at 1.2 s, then 13.7 s, 25.7 s; in the screenshot run 4.7 s, 16.8 s, 28.8 s. Alternates Fractopia and swarm. | `BS-008-console.txt`, `BS-008-obs-swarm-before.png` → `BS-008-obs-a.png` → `BS-008-obs-b.png` |
| Autoplay in OBS's CEF (C-008, TC-025) | **No click needed.** A probe `new AudioContext()` reports `state: "running"`. The page's `startsSuspended()` would therefore not wait for Interact. | `BS-008-console.txt` (eval line) |
| Built-in mic, `audio=default` | **Blocked: macOS microphone access for OBS is off.** Every OBS start logs `[macOS] Permission for audio device access denied.` `enumerateDevices()` returns 0 audio inputs, and the page logs `obs-page: audio unavailable (NotFoundError: Requested device not found), running without audio`. No macOS prompt appeared (the permission was already decided), so nothing was clicked. Turning it on needs the user in System Settings. | `BS-008-obs-log.txt`, `BS-008-console.txt` |
| Hidden source (C-017, TC-034) | **Pauses and resumes.** rAF callbacks per 3 s: 90 visible (30 fps), 0 with the scene item hidden (`SetSceneItemEnabled false`, `visibilityState: "hidden"`), 0 with another program scene, 90 again after showing. No `preset request` while hidden (16 s and 20 s windows); after showing, the next change came 7 s later, so the preset timer pauses with rendering. Audio after resume not checked (no audio). | session transcript, summarised here |
| Flag missing (TC-033) | **Partly.** Without `--enable-media-stream`, visuals still render; the audio line is the same `NotFoundError`, because the macOS permission denial comes first. The flag-specific error is left to the user checklist. | `BS-008-obs-log.txt` run D |
| Console via `--remote-debugging-port=9222` | **Works (after one restart).** `http://127.0.0.1:9222/json/list` lists `title: "SynesthesiaVibe"`, `url: http://localhost:8080/?preset=…`. `Log.enable` replays the messages logged before attaching. The `chrome://inspect` UI itself was not used (no GUI automation). | `BS-008-console.txt` |
| First run with `--remote-debugging-port` | **Blank source once.** The very first Browser Source in this OBS profile (CEF profile created at that moment), launched with both flags: no page request reached the server, only the CEF GPU helper started, and the debugging port accepted connections without answering. A `refreshnocache` button press did not help. After a plain quit and relaunch, the page loaded every time, also with the debugging port. Cause not isolated (first CEF run vs the flag). | `BS-008-first-run-blank.png` (transparent frame), `BS-008-obs-log.txt` run A |
| Console errors in the OBS log | **Yes.** `console.error` lines appear as `[obs-browser: 'BS-008 Browser'] Error: …`, including the core's two harmless startup lines. `console.info` lines (settings, preset) do not. | `BS-008-obs-log.txt` |

## Key console lines (OBS run C, reload at 1.0 s)

```
  1.2 [info] obs-page: settings cycle=10s blend=2s fps=60 audio=default presets=Rovastar - Fractopia (Upspoken Mix) | shifter - swarm
  1.2 [info] obs-page: preset Rovastar - Fractopia (Upspoken Mix)
  1.2 [error] obs-page: audio unavailable (NotFoundError: Requested device not found), running without audio NotFoundError: Requested device not found
 13.7 [info] obs-page: preset request (timer)
 13.7 [info] obs-page: preset shifter - swarm
 25.7 [info] obs-page: preset request (timer)
 25.8 [info] obs-page: preset Rovastar - Fractopia (Upspoken Mix)
 28.3 >> eval state: {"visibility":"visible","hidden":false,"secure":true,"probeAudioContext":"running","inputs":0,...,"ua":"... Chrome/150.0.7871.287 OBS/33.0.3 ..."}
```

Hidden-source measurements:

```
visible        {"visibility":"visible","rafPer3s":90}
item hidden    bs008 visibilitychange hidden / {"visibility":"hidden","rafPer3s":0}
shown again    bs008 visibilitychange visible / {"visibility":"visible","rafPer3s":90}; preset request 7.0 s later
other scene    bs008 visibilitychange hidden / {"visibility":"hidden","raf":0,"ms":3011}
back to scene  {"visibility":"visible","rafPer3s":90}
```

No `UNCAUGHT` line in any run.

## Screenshots (git-ignored, local only)

- `BS-008-obs-swarm-before.png`: swarm (mandala, red/blue) at shot time 3.9 s.
- `BS-008-obs-a.png`: Fractopia (cyan and orange flow field filling the frame) at 11.7 s, after the
  timer switched at about 4.2 s.
- `BS-008-obs-b.png`: swarm again (green/blue mandala) at 23.2 s, after the switch at about 16.3 s.
- `BS-008-obs-first-load.png`: Fractopia from the first successful load (run B).
- `BS-008-first-run-blank.png`: the transparent frame of the source that never loaded (run A).

The visuals do not react to sound in any of these: there was no audio.

## Not verified here

- Audio reaction inside OBS (built-in mic blocked by the macOS permission; no loopback device).
- Audio continuing after hide/show.
- The error name when only `--enable-media-stream` is missing.
- The OBS UI path Settings → Advanced → Sources (the setting value was confirmed through the log).
- The `chrome://inspect` screens, Windows, Linux, BlackHole and VB-Cable: see
  `bs-008-user-checklist.md`.

## Leftovers

- The `synesthesia-spike` scene collection keeps the `BS-008` scene with `BS-008 Browser`. It cannot
  load whenever that collection is opened without the local server running. Remove it when the
  spike collection is cleaned up.
- The obs-browser CEF profile now exists in the OBS profile folder (created by OBS on the first
  Browser Source).
