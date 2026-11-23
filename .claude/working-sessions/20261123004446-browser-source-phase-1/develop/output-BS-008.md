# BS-008

## Subtasks executed

- Served `browser-source/` on `127.0.0.1:8080` and quit OBS gracefully. Relaunched it with
  `open -a OBS --args --enable-media-stream --remote-debugging-port=9222`.
- Switched to the `synesthesia-spike` collection over obs-websocket. Created the scene `BS-008` and
  made it the program scene. Added `BS-008 Browser` (`browser_source`, 1920×1080,
  `?preset=fractopia,swarm&cycle=10&blend=2&audio=default`) with `CreateInput`.
- First launch: the source never loaded (details below). After a quit and relaunch with only
  `--enable-media-stream`, it loaded. I captured `GetSourceScreenshot` images and the OBS log.
- Relaunched with both flags again. Read the page console over CDP and measured preset timing,
  the autoplay state and the input count.
- Hidden-source test (C-017): scene item hidden/shown, and program scene switched away/back. I
  measured rAF rate, `visibilityState` and the preset timer.
- Flag-missing run (`--remote-debugging-port` only) for TC-033.
- Restored the `Untitled` collection, quit OBS and relaunched it with plain `open -a OBS`. Stopped
  the HTTP server.
- Applied to `browser-source/README.md` only the corrections this run proves.
- Wrote the user checklist (macOS + BlackHole, Windows + VB-Cable) and the evidence report.

## Files changed

- `browser-source/README.md` (corrections, see below)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md` (BS-008 section appended)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-BS-008.md` (new)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/BS-008/` (new):
  `bs-008-obs-check.md`, `bs-008-user-checklist.md`, `BS-008-obs-log.txt`, `BS-008-console.txt`,
  `BS-008-obs-a.png`, `BS-008-obs-b.png`, `BS-008-obs-swarm-before.png`,
  `BS-008-obs-first-load.png`, `BS-008-first-run-blank.png`, `bs-008-obs-ws.mjs`,
  `bs-008-cdp-console.mjs`. Only the `.md` files are tracked by git.
- `.claude/working-sessions/20261123004446-browser-source-phase-1/session.log` (appended)
- Outside the repo, by OBS itself and not by me:
  - the `synesthesia-spike` scene collection gained the `BS-008` scene
  - the OBS profile gained its obs-browser CEF profile, created on the first Browser Source
  - no OBS config file was edited

## What changed and why

**Verified in OBS 33.0.3 on macOS:**

- **Rendering.** The page fills the 1920×1080 source. Hardware acceleration is on and WebGL2
  works.
- **Preset cycling.** A change comes every 12.0 s with `cycle=10&blend=2`, which confirms the
  `cycle + blend` meaning.
- **Autoplay.** CEF's `AudioContext` starts `running`, so no Interact click is needed.
- **Hidden source.** Hiding the scene item or switching scenes sets the page to `hidden`, rAF
  drops to 0 and the preset timer stops. Showing it again resumes at 30 fps with the same preset.
- **Console access.** `--remote-debugging-port=9222` exposes the page at `/json/list`, and attaching
  replays the earlier console lines. OBS also writes the page's `console.error` lines into its own
  log.

**Blocked: built-in mic.** macOS microphone access for OBS is **off**. Every OBS start logs
`[macOS] Permission for audio device access denied.`, the page sees 0 audio inputs and reports
`audio unavailable (NotFoundError: Requested device not found)`. macOS showed no prompt because the
permission was already decided, so nothing was clicked. Enabling it needs the user: System Settings
→ Privacy & Security → Microphone → OBS. **Audio reaction inside OBS is therefore unverified.**

**README corrections (each proven by this run):**

- Known limits: "Checked on OBS 33.0.3 on macOS" now says what was checked (render, cycling,
  pausing). It also says that audio reaction in OBS is not tested.
- Step 3 (macOS): what a previously denied microphone permission looks like. There is no new
  prompt, the OBS log shows the `denied` line and the page shows `NotFoundError`. The fix is to turn
  access on and restart.
- Step 4: check hardware acceleration through the `Browser Hardware Acceleration: true` log line.
  The menu path comes from OBS 33's locale strings.
- Hidden sources: the "not checked yet" sentence is replaced with the verified behaviour. The
  preset timer stops too, and the page carries on from the same preset.
- Reading the console:
  - OBS copies the page's error lines into its log as `[obs-browser: '<source name>'] Error: …`.
    Info lines are not copied.
  - The `--remote-debugging-port` method is confirmed at `/json/list`. The `chrome://inspect`
    screens are still untried.
  - Added a note on the one-time blank source on the first run.
  - Removed "not tried with OBS yet".
  - Interact is not needed for audio in OBS 33.
- Troubleshooting:
  - New `NotFoundError` entry for macOS microphone access off.
  - The `NotAllowedError` entry is narrowed to the missing flag, marked "probably" because it is
    unverified.
  - The autoplay entry says the line normally appears only in desktop Chrome.
  - The two core startup lines also show in the OBS log.

The following README statements are kept unchanged because the run could not prove or disprove
them:

- `open` ignoring the flag when OBS is already running (I always quit first)
- the Settings → Advanced → Sources UI path
- the Windows, Linux and loopback steps
- "Couldn't load that page!"

## Verification

- No build or test suite applies (static page and docs). I re-checked all README anchor links
  with a script: every `](#…)` resolves to a heading. A grep found no absolute machine paths or
  personal data in the README.
- OBS state was checked after the restore:
  - `GetSceneCollectionList` returns `currentSceneCollectionName: "Untitled"`
  - the OBS process runs with no arguments
  - nothing listens on 9222
  - `curl http://localhost:8080/` is refused (server stopped)

## Evidence

- `evidence/BS-008/bs-008-obs-check.md` is the full report: results table, console excerpts,
  hidden-source measurements, screenshot descriptions and what was not verified.
- `evidence/BS-008/BS-008-obs-a.png`: Fractopia filling the source, after the timer switch.
- `evidence/BS-008/BS-008-obs-b.png`: swarm, one cycle (12 s) later.
- `evidence/BS-008/BS-008-obs-swarm-before.png`: swarm before the switch to Fractopia.
- `evidence/BS-008/BS-008-obs-first-load.png`: the first successful load.
- `evidence/BS-008/BS-008-first-run-blank.png`: the transparent frame of the source that never
  loaded on the first run.
- `evidence/BS-008/BS-008-obs-log.txt`: trimmed OBS log for runs A to E, home paths scrubbed.
- `evidence/BS-008/BS-008-console.txt`: CDP console for the reload run and the timed screenshot
  run.
- `evidence/BS-008/bs-008-user-checklist.md`: the user checklist.
- Screenshots were taken with `GetSourceScreenshot`, not `screencapture`, because the delegation
  allows no GUI automation. They are the source's own pixels. No screenshot shows music reaction.

## Known limitations / follow-ups

- **Needs the user (stop-and-report item):** OBS's macOS microphone access is off. The
  orchestrator should ask the user to turn it on (System Settings → Privacy & Security →
  Microphone → OBS) before any built-in-mic check. The user checklist covers this as step 1.
- **Phase 1 is not done** until the user runs `bs-008-user-checklist.md` and reports back (C-003).
- **First-run blank source.** The first Browser Source in a fresh OBS profile, launched with
  `--remote-debugging-port=9222`, never loaded until OBS was restarted. I did not isolate the cause
  (first CEF start vs the flag). It is a candidate for `.claude/COMMON-ISSUES.md` (orchestrator or
  document-writer).
- TC-033's flag-specific error name is unknown. With the permission denied, both runs gave
  `NotFoundError`. The user checklist asks for it, and the README's `NotAllowedError` entry may
  need a final wording after that.
- Audio after hide/show (TC-034, second half) is unverified.
- The `BS-008` scene stays in the `synesthesia-spike` collection. Remove it when that collection
  is cleaned up.
- obs-websocket authentication is still off, per the spike decision. The user should turn it back
  on after the run (`output-plan-gate-1-spike-decisions.md`).
- Doc gaps for document-writer (BS-010):
  - `docs/integrations.md` should mention that the OBS log carries the page's error lines.
  - It should also note that `--remote-debugging-port` exposes the page at `/json/list`.
- No test added (project default).

## Handoff for downstream review

- Acceptance criteria addressed:
  - OBS screenshots show the visualizer filling a 1920×1080 source and changing presets.
  - Reacting to sound was blocked by a freshly verified cause: macOS microphone permission denied
    for OBS, as the OBS log shows on every start.
  - The checklist exists and covers both OSes.
  - TC-030 pass. TC-034 visibility half pass. TC-025 resolved for OBS (no suspension).
  - TC-031 and TC-032 are left to the user. TC-033 is partial.
- Areas to scrutinize:
  - The README wording of the macOS `NotFoundError` entry, and the "probably" on `NotAllowedError`.
  - The first-run note in *Reading the page's console*. It reports one observation with no cause.
  - The claim that the preset timer stops while hidden. It rests on no `preset request` arriving
    in 16 s and 20 s hidden windows (cycle 12 s), and on the next change arriving 7 s after the
    source was shown.
- How to re-run:
  1. Serve the page and start OBS with both flags.
  2. Run `OBS_ENV=<local obs-websocket credentials file> node bs-008-obs-ws.mjs '<JSON request list>'`
     for OBS requests (add `"save":"<file>.png"` to a `GetSourceScreenshot` entry).
  3. Run `node bs-008-cdp-console.mjs <seconds> '[{"at":1,"type":"reload"}]'` for the console.
