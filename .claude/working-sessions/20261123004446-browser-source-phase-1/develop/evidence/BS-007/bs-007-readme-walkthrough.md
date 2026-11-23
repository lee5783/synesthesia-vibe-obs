# BS-007 README walkthrough

Date: 2026-11-23. Machine: macOS (arm64), OBS 33.0.3 installed but **not launched**, Chrome
installed, Python 3.9.6, Node 24. No loopback driver installed. All commands run from the
repository root unless stated.

## 1. Prerequisites (README "What you need")

```
bash: ok   curl: ok   unzip: ok   tar: ok   python3: ok   shasum: ok   sha256sum: ok
```

## 2. Step 1, fetch (verbatim)

```
$ browser-source/fetch-dist.sh
Installed synesthesia-vibe web dist 0.7.0 (15 files, 10 presets, 23 textures) into browser-source/dist
exit=0
$ browser-source/fetch-dist.sh 0.7.0
Installed synesthesia-vibe web dist 0.7.0 (15 files, 10 presets, 23 textures) into browser-source/dist
exit=0
$ browser-source/fetch-dist.sh v0.7.0
Installed synesthesia-vibe web dist 0.7.0 (15 files, 10 presets, 23 textures) into browser-source/dist
exit=0
$ browser-source/fetch-dist.sh 0.0.0-nope
fetch-dist: not found (HTTP 404), check that release v0.0.0-nope exists: https://github.com/lee5783/synesthesia-vibe/releases/download/v0.0.0-nope/SHA256SUMS.txt
exit=1
$ browser-source/fetch-dist.sh 'bad;ver'
fetch-dist: invalid version 'bad;ver' (expected e.g. 0.7.0 or v0.7.0)
exit=1
```

The success line matches the README text exactly. `dist/` holds `LICENSE`, `NOTICE`, `VERSION`,
the module files and `resources/{manifest.json,presets,textures}`.
`git check-ignore -v browser-source/dist/resources/manifest.json` → `.gitignore:10:browser-source/dist/`.

## 3. Step 2, server (verbatim)

```
$ python3 -m http.server 8080 --bind 127.0.0.1 --directory browser-source
$ lsof -iTCP:8080 -sTCP:LISTEN   → Python 127.0.0.1:8080   (loopback only)
```

| Request | Status, type |
|---|---|
| `/` | 200 text/html (contains `<canvas id="visualizer">` and `obs-page.js`) |
| `/obs-page.js` | 200 text/javascript |
| `/dist/synesthesia-vibe.js` | 200 text/javascript |
| `/dist/synesthesia-vibe-wasm.wasm` | 200 application/wasm |
| `/dist/resources/manifest.json` | 200 application/json |
| `/dist/LICENSE`, `/dist/NOTICE` | 200 |
| `/?preset=fractopia,swarm&cycle=30&blend=2` | 200 text/html |
| `/?audio=CABLE+Output` | 200 text/html |

After stopping the server, `curl http://localhost:8080/` → connection refused (exit 7): the case the
README's "Couldn't load that page!" entry covers.

## 4. README example URLs in headless Chrome

Each README example URL loaded in headless Chrome (fake media device, no real mic), page log lines:

```
== /                                   settings cycle=30s blend=2s fps=60 audio=default presets=<all 10, table order>
== /?preset=fractopia                  settings cycle=off blend=2s fps=60 audio=default presets=Rovastar - Fractopia (Upspoken Mix)
== /?preset=fractopia,swarm,tiger&cycle=60&blend=5
                                       settings cycle=60s blend=5s fps=60 audio=default presets=Rovastar - Fractopia (Upspoken Mix) | shifter - swarm | Stahlregen & Flexi + Geiss - Tiger No.5 (Random texture flow)
== /?audio=BlackHole&fps=30            settings cycle=30s blend=2s fps=30 audio=BlackHole presets=<all 10>
== /?audio=none                        settings … audio=none …   then: audio off (audio=none)
== /?preset=Esotic%20%26%20Rozzer      settings cycle=off … presets=Esotic & Rozzer - Now And Later
== /?preset=Eo.S.+Geiss                settings cycle=off … presets=Eo.S. + Geiss - glowsticks v2 02 (Relief Mix)
== /?preset=fractopia,swarm&cycle=30&blend=2&audio=none
                                       settings cycle=30s blend=2s fps=60 audio=none presets=Rovastar - Fractopia (Upspoken Mix) | shifter - swarm
```

The last one is the exact line quoted in the README's "Checking what the page understood". The two
core startup lines quoted under Troubleshooting (`render_gl: audio textures R32F …`,
`synesthesia-vibe: GL pipeline ready texsize=… grid=64x48`) appeared in the same runs.

## 5. Parameter rules checked against `obs-page.js`

`readSettings`, `resolvePresets` and `chooseDevice` were evaluated in Node straight from
`browser-source/obs-page.js` (the trailing `main()` call stripped) against the real
`dist/resources/manifest.json`. Scripts are in the session scratchpad, not committed. Results:

| Query | Result |
|---|---|
| (none) | cycle 30, blend 2, fps 60, audio `default`, all 10 presets |
| `preset=glowsticks,cat+scan,dark+side,now+and+later,transcendance,fractopia,jester,tiger,fuzzy,swarm` | each short name in the README preset table resolves to exactly its preset, no ambiguity warning |
| `preset=eo.s.` | first of 3 matches, warning `matches 3 presets, using Eo.S. + Geiss …` |
| `preset=Esotic & Rozzer` (raw `&`) | value cut to `Esotic `; still resolves only because that prefix is unique (README: raw `&` cuts the name short) |
| `preset=nope` | `no preset matches "nope", skipping it`, then `matched nothing, using all presets` |
| `preset=nope,swarm` | skip warning, plays `shifter - swarm` |
| `cycle=0` | 0 (the page logs `cycle=off`) |
| `cycle=2.5&blend=0.5` | decimals accepted |
| `cycle=-1&blend=-5&fps=abc` | cycle 30 (warn), blend clamped 0 (warn), fps 60 (warn) |
| `blend=12&fps=0` | blend 10 (warn), fps 0 = uncapped |
| `audio=NONE`, `audio=Default`, `audio=` | `none`, `default`, `default` |
| `audio=BlackHole`, `audio=blackhole+2ch` vs label `BlackHole 2ch (Virtual)` | match, `deviceId: exact` |
| `audio=CABLE+Output`, `audio=cable%20output` vs label `CABLE Output (VB-Audio Virtual Cable)` | match |
| `audio=Loopback` | warning `no audio input matches "Loopback", using Default - …; available: …`, falls back to default |

The device labels in the audio rows are assumed sample labels; no real BlackHole or VB-Cable device
was present.

The `cycle + blend` timing in the README comes from BS-004's measurement (blend 0/1/3 with
`cycle=3` gave 3.0/4.0/6.0 s between changes; the first change came `cycle + blend` after load)
and the user's mid-develop decision to keep that meaning.

## 6. Test hooks

```
$ FETCH_DIST_BASE_URL="file://<scratch>/mirror/releases" \
  FETCH_DIST_ARCHIVE_URL="file://<scratch>/mirror/archive" \
  browser-source/fetch-dist.sh
Installed synesthesia-vibe web dist 0.7.0 (15 files, 10 presets, 23 textures) into browser-source/dist
exit=0
```

The mirror held `releases/v0.7.0/{SHA256SUMS.txt,synesthesia-vibe-web-0.7.0.zip}` and
`archive/v0.7.0.tar.gz`, the layout the README describes.

## 7. OBS UI strings (checked without launching OBS)

These strings were checked against the installed OBS 33.0.3 bundle's `en-US.ini` locale files (main
app and `obs-browser.plugin`): "Browser", "Local file", "URL", "Width", "Height", "Use custom frame
rate", "Control audio via OBS", "Shutdown source when not visible", "Refresh browser when scene
becomes active", "Refresh cache of current page", "Enable Browser Source Hardware Acceleration",
"Interact", "Settings", "Advanced", "Sources", "Couldn't load that page!", "Server refused the
connection". The Add Source dialog strings ("Create New", "Add existing") show a newer dialog than
the classic "+ → Browser → Add Existing" menu, so the README words those steps generically.

## Not exercised

- OBS itself: adding the source, `--enable-media-stream`, the macOS microphone permission, hardware
  acceleration placement under Advanced → Sources, Interact, hidden-source pausing,
  `--remote-debugging-port` + `chrome://inspect`. BS-008 covers the OBS check on this Mac.
- Windows: the shortcut flag, VB-Cable, Git Bash/WSL for `fetch-dist.sh`, `py` for the server.
- Linux: the one untested line.
- Loopback: BlackHole and Multi-Output Device setup (not installed here); its menu names come from
  the driver documentation.
- Real-device audio lines (`audio inputs …`, `audio <label> at … Hz`) in this walkthrough: the
  headless runs ended before audio started. BS-005 captured them with Chrome's fake device.
