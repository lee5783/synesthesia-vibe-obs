# SynesthesiaVibe Browser Source page

A full-window music visualizer for OBS Studio, added as a **Browser Source**. It runs the
[SynesthesiaVibe](https://github.com/lee5783/synesthesia-vibe) core (a MilkDrop-preset renderer)
in OBS's built-in browser, cycles through presets and reacts to an audio **input device** on your
computer. The page has no on-screen UI: it is a black canvas with the visualizer on it.

You serve the page yourself, from this folder, on `http://localhost:8080/`. There is no hosted
version.

**Known limits**

- The page cannot hear OBS's own audio mix. It listens to one system input device. To make it
  react to the music you stream, route that music into a loopback device (step 5) and point the
  page at it.
- Checked on OBS 33.0.3 on macOS: the page renders, cycles presets and pauses while hidden. Audio
  reaction inside OBS, older OBS versions, Windows and Linux are not tested yet.

## Contents

1. [What you need](#what-you-need)
2. [Step 1: fetch the visualizer files](#step-1-fetch-the-visualizer-files)
3. [Step 2: start the local server](#step-2-start-the-local-server)
4. [Step 3: start OBS with the media-stream flag](#step-3-start-obs-with-the-media-stream-flag)
5. [Step 4: add the Browser Source](#step-4-add-the-browser-source)
6. [Step 5: route your music into an input device](#step-5-route-your-music-into-an-input-device)
7. [URL parameters](#url-parameters)
8. [Hidden sources, several sources and performance](#hidden-sources-several-sources-and-performance)
9. [Reading the page's console](#reading-the-pages-console)
10. [Troubleshooting](#troubleshooting)
11. [Testing in desktop Chrome](#testing-in-desktop-chrome)
12. [For developers: fetch-dist test hooks](#for-developers-fetch-dist-test-hooks)
13. [Credits and licences](#credits-and-licences)

## What you need

- OBS Studio (checked on 33.0.3).
- This repository, cloned or downloaded. The Browser Source page does not need the
  `external/synesthesia-vibe` submodule.
- A shell with these tools on the `PATH`:
  - `bash`, `curl`, `unzip`, `tar`
  - `shasum` or `sha256sum`
  - `python3` (for the manifest step and the local server)

  macOS and most Linux systems have all of them. On Windows, Git Bash or WSL should work, but
  this is untested; `python3` may be called `py` there.
- For music-reactive visuals: a loopback audio device (BlackHole on macOS, VB-Cable on Windows).
  See step 5.

All commands below run from the **repository root**.

## Step 1: fetch the visualizer files

```sh
browser-source/fetch-dist.sh
```

This downloads the pinned SynesthesiaVibe release (currently `0.7.0`) and installs it into
`browser-source/dist/`. On success it prints:

```
Installed synesthesia-vibe web dist 0.7.0 (15 files, 10 presets, 23 textures) into browser-source/dist
```

What it does:

1. Downloads `SHA256SUMS.txt` and `synesthesia-vibe-web-<version>.zip` from the core's GitHub
   release, and checks the zip's SHA-256.
2. Downloads the same tag's source archive and copies out:
   - the 10 presets into `dist/resources/presets/`
   - the textures into `dist/resources/textures/`
   - the core's `LICENSE` and `NOTICE` into `dist/`
3. Writes `dist/resources/manifest.json`, the list of presets and textures the page loads.
4. Replaces the old `dist/` only when everything succeeded. On any error it prints one
   `fetch-dist: …` line, exits with status 1 and leaves the old `dist/` as it was.

To fetch another release, pass its version: `browser-source/fetch-dist.sh 0.7.0` or
`browser-source/fetch-dist.sh v0.7.0`. A version that does not exist fails with
`fetch-dist: not found (HTTP 404), check that release v<version> exists: …`.

`dist/` is git-ignored and never committed. The presets stay on your machine on purpose. The core
project approves its bundled presets and textures for local serving only, not for public hosting.
Many ShaderToy-converted MilkDrop presets are also licensed CC-BY-NC-SA 3.0 (see
[Credits and licences](#credits-and-licences)). So this repo fetches them for your own local use
and does not publish them.

## Step 2: start the local server

```sh
python3 -m http.server 8080 --bind 127.0.0.1 --directory browser-source
```

Leave this running for as long as OBS uses the source. If you close the terminal, the source shows
OBS's "Couldn't load that page!" error the next time it loads.

- `--bind 127.0.0.1` keeps the server on your own machine. Without it, Python serves the folder,
  presets included, to your whole network.
- The page needs a **secure context** for audio capture. `http://localhost` counts as one, so plain
  HTTP is fine here. Opening the page through a LAN IP or a hostname instead turns audio off.
- Any static HTTP server works, as long as it serves `browser-source/` on `localhost`.

Quick check: open `http://localhost:8080/` in a browser. You should see visuals on a black
background.

## Step 3: start OBS with the media-stream flag

OBS's browser only allows audio capture when OBS starts with `--enable-media-stream`. Quit OBS
fully first, then start it like this.

**macOS**

```sh
open -a OBS --args --enable-media-stream
```

If OBS is already running, `open` only brings it to the front and the flag is ignored. Also allow
microphone access for OBS: System Settings → Privacy & Security → Microphone → turn on OBS.

If microphone access for OBS was turned off earlier, macOS does not ask again. The OBS log
(Help → Log Files → View Current Log) then starts with
`[macOS] Permission for audio device access denied.`, and the page reports
`audio unavailable (NotFoundError: Requested device not found)`. Turn access on in System Settings
and restart OBS.

**Windows** (untested here)

1. Right-click the OBS shortcut → Properties.
2. Append the flag to **Target**, after the closing quote, for example:
   `"C:\Program Files\obs-studio\bin\64bit\obs64.exe" --enable-media-stream`
3. Leave **Start in** as it is, click OK, and start OBS from that shortcut.

**Linux** (untested): start OBS with `obs --enable-media-stream` and use a PulseAudio/PipeWire
"Monitor of …" source as the input in step 5.

Without the flag the visuals still run, but they do not react to sound.

## Step 4: add the Browser Source

1. Turn on hardware acceleration for browser sources. WebGL2 needs it.
   - Settings → Advanced → Sources → **Enable Browser Source Hardware Acceleration**.
   - Restart OBS (with the flag from step 3) if you changed it.
   - To check it, look for `Browser Hardware Acceleration: true` near the top of the OBS log
     (Help → Log Files → View Current Log).
2. In the **Sources** dock, click **+**, choose **Browser**, create a new source, name it and
   confirm.
3. In the properties:

   | Property | Value |
   |---|---|
   | Local file | off |
   | URL | `http://localhost:8080/` plus any [URL parameters](#url-parameters) |
   | Width | `1920` |
   | Height | `1080` |
   | Use custom frame rate | off, or set it to match `fps` |
   | Control audio via OBS | off (the page plays no sound) |
   | Shutdown source when not visible | off if you want presets to keep going (see [below](#hidden-sources-several-sources-and-performance)) |
   | Refresh browser when scene becomes active | off, for the same reason |

4. Click OK. The visualizer should appear within a second or two.

Use your canvas size if it is not 1920×1080. The page fills whatever size the source has.

Changing the URL in the properties reloads the page, so new settings take effect at once. The page
reads its settings only when it loads.

## Step 5: route your music into an input device

The page listens to an **input** device. A loopback driver gives you an input that carries what
your music player plays. Then you point the page at it with `audio=`.

The device and menu names below come from each driver's documentation. They were not checked on a
machine with the driver installed.

### macOS: BlackHole

1. Install **BlackHole 2ch** (from Existential Audio, or `brew install blackhole-2ch`).
2. Open **Audio MIDI Setup** (Applications → Utilities).
3. Click **+** at the bottom left → **Create Multi-Output Device**. Tick your speakers or
   headphones **and** BlackHole 2ch. This lets you hear the music while BlackHole carries a copy.
4. Pick the Multi-Output Device as the output, in System Settings → Sound → Output or in your
   music player.
5. Use `audio=BlackHole` in the source URL:
   `http://localhost:8080/?audio=BlackHole`

The macOS volume keys do not control a Multi-Output Device; use the player's volume instead.

### Windows: VB-Cable

1. Install **VB-CABLE** from VB-Audio (run the setup as administrator), then reboot.
2. Set your music player's output, or the Windows default output, to
   **CABLE Input (VB-Audio Virtual Cable)**.
3. To keep hearing the music: Control Panel → Sound → **Recording** tab → **CABLE Output** →
   Properties → **Listen** tab → tick **Listen to this device** and pick your speakers or
   headphones.
4. Use `audio=CABLE+Output` in the source URL:
   `http://localhost:8080/?audio=CABLE+Output`

### Finding the exact device label

Every time the page starts audio, it logs the inputs it can see:

```
obs-page: audio inputs MacBook Pro Microphone | BlackHole 2ch
```

Copy any distinctive part of a label into `audio=`. The match is case-insensitive and a part of the
label is enough (see [URL parameters](#url-parameters)). To see this line, open the page's console
([Reading the page's console](#reading-the-pages-console)), or open `http://localhost:8080/` in
desktop Chrome, which sees the same devices.

### Getting the music into the stream as well

The page only *listens*; it does not add sound to your stream. Capture the music in OBS the way you
normally would, for example with an audio capture source or Desktop Audio.

## URL parameters

Add parameters after `?` and join them with `&`:

```
http://localhost:8080/?preset=fractopia,swarm&cycle=30&blend=2&audio=BlackHole
```

| Parameter | Default | Values | What it does | Invalid value |
|---|---|---|---|---|
| `preset` | all presets, in alphabetical file-name order | comma-separated preset names or parts of names | Which presets to play, in the order given. After the last one it starts again at the first. | A name that matches nothing is skipped with a warning. If nothing matches, all presets play. |
| `cycle` | `30` | seconds, `0` or more, decimals allowed | How long each preset plays on its own after its blend has finished. `0` stays on the first preset. With only one preset, cycling is off. | Negative or not a number → `30`, with a warning. |
| `blend` | `2` | seconds, `0` to `10` | How long the cross-fade into the next preset takes. `0` is a hard cut. | Outside 0–10 → clamped to `0` or `10`. Not a number → `2`. Both warn. |
| `fps` | `60` | frames per second, `0` or more; `0` = uncapped | The page's frame cap. OBS's own frame rate also limits the page, so values above it gain nothing. Lower it (for example `fps=30`) to save GPU. | Negative or not a number → `60`, with a warning. |
| `audio` | `default` | `default`, `none`, or a device label or part of one | Which input device to listen to. `default` uses the system default input. `none` skips audio entirely, with no permission request. | A label that matches nothing → the default input, with a warning that lists the available labels. |

### How `cycle` and `blend` add up

Presets change every **`cycle + blend`** seconds. Each preset plays alone for `cycle` seconds after
its blend has finished, then the next one starts blending in.

Example: `cycle=30&blend=2` (the defaults). The first preset appears at once with no blend. The
second starts blending in about 32 s after the page loads, the third at about 64 s, and so on: a
change about every 32 s.

For a change every 20 s with a 5 s blend, use `cycle=15&blend=5`.

### How names are matched

`preset` and `audio` use the same rules:

1. Case is ignored, `+` counts as a space, and runs of spaces count as one.
2. An exact match on the whole name wins. For presets, the name is the file name without `.milk`.
3. Otherwise, any name that **contains** what you typed matches. If several do, the first one is
   used, with a warning: for presets, the first in the [default order](#the-presets-in-release-070);
   for audio, the first in the `audio inputs` list.

So `preset=fractopia` picks *Rovastar - Fractopia (Upspoken Mix)*, and `audio=blackhole` picks
*BlackHole 2ch*.

### Typing names in a URL

In a URL, `&` separates parameters, `+` means a space, and `,` separates presets in the list.

- Type spaces as `+` or `%20`: `preset=now+and+later`.
- Type `&` as `%26`: `preset=Esotic%20%26%20Rozzer`. A raw `&` cuts the name short at that point.
- A `+` that is part of a name can be typed as `+`. It turns into a space, and matching treats `+`
  and spaces the same: `preset=Eo.S.+Geiss` matches *Eo.S. + Geiss - glowsticks…*.
- Apostrophes and parentheses can be typed as they are.
- Easiest of all: use a short part of the name that avoids these characters.

### The presets in release 0.7.0

The default order, with a short name that picks each one:

| Preset (file name without `.milk`) | Short name |
|---|---|
| Eo.S. + Geiss - glowsticks v2 02 (Relief Mix) | `glowsticks` |
| Eo.S. + Phat - CAT Scan (Nirvana flux) | `cat+scan` |
| Eo.S. - dark side of the moon (plus a few more hits and a pill) | `dark+side` |
| Esotic & Rozzer - Now And Later | `now+and+later` |
| Goody - Need - Transcendance remix | `transcendance` |
| Rovastar - Fractopia (Upspoken Mix) | `fractopia` |
| Rovastar - Jester's Calling 2 | `jester` |
| Stahlregen & Flexi + Geiss - Tiger No.5 (Random texture flow) | `tiger` |
| Unchained - Fuzzy Sciences | `fuzzy` |
| shifter - swarm | `swarm` |

`dist/resources/manifest.json` lists the presets of the release you fetched.

### Checking what the page understood

On load the page logs one line with the settings it is using, for example:

```
obs-page: settings cycle=30s blend=2s fps=60 audio=none presets=Rovastar - Fractopia (Upspoken Mix) | shifter - swarm
```

Warnings about ignored values or unmatched names appear just before it.

### Examples

| URL | Result |
|---|---|
| `http://localhost:8080/` | All presets, a change every ~32 s, default input. |
| `http://localhost:8080/?preset=fractopia` | One preset, no cycling. |
| `http://localhost:8080/?preset=fractopia,swarm,tiger&cycle=60&blend=5` | Three presets in that order, a change every ~65 s. |
| `http://localhost:8080/?audio=BlackHole&fps=30` | All presets, listening to BlackHole, capped at 30 fps. |
| `http://localhost:8080/?audio=none` | Visuals without audio; no permission needed. |

## Hidden sources, several sources and performance

- **Hidden sources.** When you hide the source (the eye icon in the Sources dock) or switch to a
  scene that does not contain it, OBS reports the page as hidden. The page then stops rendering, so
  it uses no GPU, and the preset timer stops too. When the source is shown again, it carries on with
  the same preset from where it stopped. Checked on OBS 33.0.3 on macOS.
  - **Shutdown source when not visible** unloads the page while it is hidden. When it comes back
    it starts again from the first preset and reopens the audio device.
  - **Refresh browser when scene becomes active** reloads the page every time you switch to that
    scene, with the same effect.
  - Leave both off if you want the presets to carry on. Turn the first on if you would rather save
    GPU and do not mind the restart.
- **Several sources.** Each Browser Source running this page has its own WebGL2 renderer and its
  own audio capture, so two sources cost about twice the GPU. To show the same visualizer in
  several scenes, add the **existing** source to each scene (click **+**, choose **Browser**, then
  pick your source from the existing ones) rather than creating a new one.

## Reading the page's console

The page reports everything to its browser console, with lines that start with `obs-page:`.

OBS also copies the console's **error** lines into its own log (Help → Log Files → View Current
Log), as `[obs-browser: '<source name>'] Error: …`. That is the quickest way to see why audio or
loading failed. The settings and preset lines are not errors, so they appear only in the console.

To see the whole console from OBS:

1. Quit OBS, then start it with a remote-debugging port as well as the media flag:
   - macOS: `open -a OBS --args --enable-media-stream --remote-debugging-port=9222`
   - Windows: append `--remote-debugging-port=9222` to the shortcut target as in step 3.
2. In desktop Chrome, open `chrome://inspect`, click **Configure…** next to *Discover network
   targets*, and add `localhost:9222`.
3. The page `http://localhost:8080/` appears under *Remote Target*. Click **inspect** to open its
   console.
4. When you are done, start OBS without `--remote-debugging-port` again. While it is on, any
   program on your computer can control OBS's browser.

With OBS 33.0.3 on macOS, the page is listed at `http://localhost:9222/json/list` (title
*SynesthesiaVibe*), and its console also shows the lines logged before you attached. The
`chrome://inspect` screens themselves were not tried. Once, the first time OBS ever loaded a
Browser Source with this flag, the source stayed blank and the port did not answer; quitting and
restarting OBS fixed it.

To give the page a click or a key press, right-click the source → **Interact** and click inside
the window. OBS 33 does not need this for audio (see the autoplay line under
[Troubleshooting](#troubleshooting)).

## Troubleshooting

**OBS shows "Couldn't load that page!" ("Server refused the connection")**
The local server is not running, or runs on another port. Start it (step 2) and check that the URL
says `8080`.

**The source stays black**
Check the console:

- `obs-page: could not load ./dist/synesthesia-vibe.js, run browser-source/fetch-dist.sh` →
  `dist/` is missing. Run step 1, then reload the source.
- `obs-page: could not load ./dist/resources/manifest.json` → `dist/` is incomplete. Run step 1
  again.
- `obs-page: could not start the renderer … WebGL2 is not available on this canvas` → turn on
  **Enable Browser Source Hardware Acceleration** (step 4) and restart OBS.
- `obs-page: no preset in the list could be loaded` → every chosen preset failed to compile. Try
  without `preset=`.

**Two red error lines at startup**
These two lines come from the core and are harmless. They are status messages printed on the error
channel, so they also appear in the OBS log as `[obs-browser: '<source name>'] Error: …`:

```
render_gl: audio textures R32F (GL_OES_texture_float_linear present) GL_RENDERER "…"
synesthesia-vibe: GL pipeline ready texsize=1920x1080 grid=64x48
```

**Visuals run but do not react to music**
Check the console:

- `audio unavailable (NotFoundError: Requested device not found)` → on macOS this is what you see
  when microphone access for OBS is off: the OBS log starts with
  `[macOS] Permission for audio device access denied.` Turn access on (step 3) and restart OBS.
- `audio unavailable (NotAllowedError: …)` → OBS was probably not started with
  `--enable-media-stream` (step 3).
- `no audio input matches "…", using …; available: …` → the `audio=` value does not match. Copy part
  of a label from the list.
- `audio capture needs getUserMedia, an AudioWorklet and a secure context (https or localhost)` →
  the URL does not use `localhost`. Use `http://localhost:8080/`.
- `the AudioContext is suspended by the autoplay policy, audio starts on the first click or keypress`
  → OBS 33 lets audio start without a click (checked on macOS), so this line normally appears only
  in desktop Chrome. If you see it in OBS, right-click the source → **Interact**, and click once
  inside the window.
- `audio off (audio=none)` → remove `audio=none` from the URL.
- `audio <label> at 48000 Hz` (or another rate) but still no reaction → the page is listening, but
  no sound reaches that device. Check the routing in step 5, and that the music is playing to the
  Multi-Output Device or to CABLE Input.

**The visuals stopped reacting after I unplugged or changed a device**
The console shows `audio input <label> stopped, running without audio; reload the page to retry`.
The page does not retry by itself. Reconnect the device, then reload the source: open its
properties and click **Refresh cache of current page**.

**The presets restart from the first one**
**Shutdown source when not visible** or **Refresh browser when scene becomes active** is on. See
[Hidden sources](#hidden-sources-several-sources-and-performance).

**A preset I asked for does not play**
Look for `no preset matches "…", skipping it` or `preset "…" matches N presets, using …`. Use a
longer or more distinctive part of the name, and encode `&` as `%26`.

## Testing in desktop Chrome

You can try URLs in desktop Chrome before using them in OBS. Open `http://localhost:8080/` with
any parameters, and allow microphone access when Chrome asks. Chrome's autoplay policy keeps audio
off until you interact with the page, so click once or press a key in it. The console (View →
Developer → JavaScript Console) shows the same `obs-page:` lines as OBS.

## For developers: fetch-dist test hooks

Two optional environment variables point `fetch-dist.sh` at other download locations, for example a
local `file://` mirror used to test failure cases:

- `FETCH_DIST_BASE_URL` replaces the release download base
  (`https://github.com/lee5783/synesthesia-vibe/releases/download`). The script fetches
  `$FETCH_DIST_BASE_URL/v<version>/SHA256SUMS.txt` and `…/synesthesia-vibe-web-<version>.zip`.
- `FETCH_DIST_ARCHIVE_URL` replaces the source-archive base
  (`https://github.com/lee5783/synesthesia-vibe/archive/refs/tags`). The script fetches
  `$FETCH_DIST_ARCHIVE_URL/v<version>.tar.gz`.

```sh
FETCH_DIST_BASE_URL="file://$PWD/mirror/releases" \
FETCH_DIST_ARCHIVE_URL="file://$PWD/mirror/archive" \
browser-source/fetch-dist.sh
```

The zip is checked against `SHA256SUMS.txt`. The source archive has no published checksum, so it
is fetched over HTTPS without one.

## Credits and licences

- **SynesthesiaVibe core** ([lee5783/synesthesia-vibe](https://github.com/lee5783/synesthesia-vibe))
  is MIT-licensed. After step 1, its licence is in `dist/LICENSE` and its third-party notices are in
  `dist/NOTICE`. These cover BeatDrop and MilkDrop 2, Emscripten and the other components the web
  release contains.
- **Presets and textures** come from the
  [BeatDrop](https://github.com/OfficialIncubo/BeatDrop-Music-Visualizer) distribution, unmodified.
  Each preset's file name carries its authors' names (see the
  [preset table](#the-presets-in-release-070)). The textures are under BeatDrop's BSD 3-Clause
  licence. BeatDrop's preset notice, reproduced in `dist/NOTICE`, says:

  > Majority of ShaderToy shader MilkDrop presets are licensed with CC-BY-NC-SA 3.0 […] You aren't
  > allowed to use these ShaderToy converted presets for commercial purposes. Use them with care and
  > you must credit shader authors of you have used them.

  If your stream earns money, check whether that counts as commercial use before you show these
  presets, and credit the preset authors. This repo fetches the presets for your local use only and
  never publishes them.
- **This repository** is licensed under the GNU General Public License, version 2 or (at your
  option) any later version (GPL-2.0-or-later). See [`../LICENSE`](../LICENSE).
