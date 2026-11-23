# BS-008 user checklist: Browser Source page in OBS with a loopback device

Phase 1 is done only after you run this checklist and report back (C-003). The agent could check
rendering, preset cycling, autoplay and hidden-source pausing in OBS 33.0.3 on macOS. It could not
check audio reaction, because OBS's microphone access is off on the test Mac and no loopback
device is installed.

Run the macOS part, the Windows part, or both. For each step, write **pass** or **fail** in the
result table at the end, and paste the console or log lines asked for.

## Where to put screenshots

- Save them in this folder, under `user/`:
  `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/BS-008/user/`
- Name them `macos-<what>.png` or `windows-<what>.png`, for example `macos-react-a.png`.
- Images in this folder are git-ignored, so they stay on your machine. Crop out anything personal
  (other windows, device names you would rather not share).
- macOS: Cmd+Shift+4, then Space, then click the OBS window. Windows: Win+Shift+S.

## Before you start (both systems)

1. From the repository root, fetch the visualizer files:
   `browser-source/fetch-dist.sh`
   Expected: `Installed synesthesia-vibe web dist 0.7.0 (15 files, 10 presets, 23 textures) into browser-source/dist`
2. Start the local server and leave it running:
   `python3 -m http.server 8080 --bind 127.0.0.1 --directory browser-source`
   (On Windows, `python3` may be `py`.)
3. Open `http://localhost:8080/?audio=none` in a normal browser. Expected: visuals on a black
   background.

## macOS with BlackHole

1. **Microphone access for OBS (required on the test Mac).** System Settings → Privacy & Security →
   Microphone → turn on **OBS**. On the test Mac it is currently off, and macOS does not ask again,
   so the page cannot hear anything until you turn it on.
2. **Loopback.** Install BlackHole 2ch (`brew install blackhole-2ch` or the Existential Audio
   installer). In Audio MIDI Setup, create a **Multi-Output Device** with your speakers or
   headphones **and** BlackHole 2ch ticked. Set it as the sound output (System Settings → Sound →
   Output). Start some music.
3. **Start OBS with the flags.** Quit OBS fully, then:
   `open -a OBS --args --enable-media-stream --remote-debugging-port=9222`
   The debugging port is optional; it lets you read the page's full console (step 9).
4. **OBS log check.** Help → Log Files → View Current Log. Near the top, expect:
   - `[macOS] Permission for audio device access granted.` (it said `denied` during the agent run)
   - `Browser Hardware Acceleration: true`
5. **Add the source.** In a test scene, add a Browser source:
   - URL: `http://localhost:8080/?audio=BlackHole&cycle=15`
   - Width `1920`, Height `1080`
   - Leave "Shutdown source when not visible" and "Refresh browser when scene becomes active" off.
6. **Reacts to music (TC-031, the "Done when").** Expected: the visuals move with the music and
   change with the beat; pausing the music calms them down. Screenshots: `macos-react-a.png` with
   music playing, `macos-react-b.png` about 20 s later (a different preset).
7. **Preset cycling.** Expected: a new preset blends in about every **17 s** (`cycle=15` plus the
   default 2 s blend), and it wraps around after the last preset.
8. **No click needed.** Do not click or Interact with the source. Expected: audio reaction starts by
   itself.
9. **Console lines.** In the OBS log, expect **no** `obs-page: audio unavailable` line. If you used
   the debugging port, open `http://localhost:9222/json/list` in Chrome to confirm the page is
   listed, then use `chrome://inspect` → Configure… → add `localhost:9222` → **inspect**. Expect:
   - `obs-page: audio inputs … | BlackHole 2ch | …`
   - `obs-page: audio BlackHole 2ch at <rate> Hz`
   - no `the AudioContext is suspended …` line
   Paste these lines into the report (you may shorten other device names to `<device>`).
10. **Hide and show (TC-034).** Click the source's eye icon to hide it for 10 s, then show it.
    Expected: the visuals continue from the same preset and **still react to the music**. (The
    agent confirmed the pause and resume, but not that audio keeps working afterwards.)
11. **Without the flag (TC-033).** Quit OBS, start it normally (`open -a OBS`), and open the log.
    Expected: the visuals still run without reacting, and the log shows an
    `obs-page: audio unavailable (…)` error line. Paste that line: the README needs to know which
    error name appears (the agent saw only `NotFoundError`, because microphone access was off).
12. **Clean up.** Start OBS normally again afterwards (without `--remote-debugging-port`). You can
    turn OBS microphone access back off if you do not use it otherwise.

## Windows with VB-Cable

1. **Loopback.** Install VB-CABLE from VB-Audio (run the setup as administrator), then reboot. Set
   your music player's output, or the Windows default output, to
   **CABLE Input (VB-Audio Virtual Cable)**. To keep hearing the music: Control Panel → Sound →
   Recording → **CABLE Output** → Properties → Listen → tick **Listen to this device** and choose
   your speakers.
2. **Microphone privacy.** Settings → Privacy & security → Microphone: microphone access and
   "Let desktop apps access your microphone" must be on.
3. **Start OBS with the flags.** Right-click the OBS shortcut → Properties → append to Target,
   after the closing quote:
   ` --enable-media-stream --remote-debugging-port=9222`
   Start OBS from that shortcut.
4. **OBS log check.** Help → Log Files → View Current Log. Expect
   `Browser Hardware Acceleration: true`. If it says `false`: Settings → Advanced → Sources →
   **Enable Browser Source Hardware Acceleration**, then restart OBS from the shortcut.
5. **Add the source.** Browser source, URL `http://localhost:8080/?audio=CABLE+Output&cycle=15`,
   Width `1920`, Height `1080`, shutdown/refresh options off.
6. **Reacts to music (TC-032, the "Done when").** Same expectation as macOS step 6. Screenshots:
   `windows-react-a.png`, `windows-react-b.png`.
7. **Preset cycling.** A new preset about every 17 s.
8. **No click needed.** Audio reaction starts without clicking or Interact.
9. **Console lines.** Same as macOS step 9, with `CABLE Output (VB-Audio Virtual Cable)` as the
   expected device. Also note whether the OBS log shows any `obs-page:` error line.
10. **Hide and show.** Same as macOS step 10.
11. **Without the flag.** Remove the flags from the shortcut, start OBS, and paste the
    `obs-page: audio unavailable (…)` line from the log.
12. **Clean up.** Remove `--remote-debugging-port=9222` from the shortcut. Keep
    `--enable-media-stream` if you want to use the page.

## Result table (fill in and send back)

| Step | macOS (BlackHole) | Windows (VB-Cable) | Notes / pasted lines |
|---|---|---|---|
| OBS version |  |  |  |
| 4. Log: permission / HW acceleration |  |  |  |
| 6. Reacts to music |  |  |  |
| 7. Preset change about every 17 s |  |  |  |
| 8. No click needed |  |  |  |
| 9. Console lines |  |  |  |
| 10. Hide/show, audio still works |  |  |  |
| 11. Error line without the flag |  |  |  |
| Screenshots saved |  |  |  |

If any step fails, include the `obs-page:` lines from the OBS log or the console, and which step
of `browser-source/README.md` you were on.
