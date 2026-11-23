# Test cases

> **Revision 2 (after Gate 1):** the Pages rows are removed, the defaults are concrete, and the D1
> (TC-036+) and Phase 2 spike (TC-040+) scenarios are added.

Manual QA scenarios for Phase 1, D1 and the Phase 2 spike. "Local page" means `browser-source/` served with
`python3 -m http.server 8080 --directory browser-source` and opened at `http://localhost:8080/`.
"Chrome fake audio" means Chrome launched with
`--use-fake-ui-for-media-stream --use-fake-device-for-media-stream --autoplay-policy=no-user-gesture-required`.

| ID | Component / area | Preconditions | Expected |
|----|--------------------|---------------|----------|
| TC-001 | `fetch-dist.sh` — happy path | Clean checkout, network, no `browser-source/dist/` | Exit 0. `dist/` holds the 15 web-dist files, and `dist/VERSION` reads `0.7.0`. The run works both from the repo root and from inside `browser-source/` |
| TC-002 | `fetch-dist.sh` — version forms | — | `0.7.0`, `v0.7.0` and no argument (pinned default) all fetch v0.7.0 |
| TC-003 | `fetch-dist.sh` — checksum mismatch | A tampered zip or checksum line is simulated | Exit non-zero with a clear "checksum" message. An existing `dist/` is unchanged |
| TC-004 | `fetch-dist.sh` — unknown version | `fetch-dist.sh 9.9.9` | Exit non-zero with a clear "not found / download failed" message. `dist/` is unchanged |
| TC-005 | `fetch-dist.sh` — resources | After TC-001 (BS-002 implemented) | `dist/resources/manifest.json` is valid JSON with 10 presets and 23 textures. `dist/LICENSE` and `dist/NOTICE` are present |
| TC-006 | Git hygiene | After TC-001/TC-005 | `git status` shows nothing under `browser-source/dist/`. The repo contains no absolute machine paths |
| TC-007 | `index.html` — layout | Local page in Chrome, 1920×1080 window | The visualizer fills the window edge to edge on a black background. There are no scrollbars, text or controls |
| TC-008 | `obs-page.js` — no dist | `dist/` deleted, local page | The page stays black. The console names the missing file. Nothing throws uncaught |
| TC-009 | `obs-page.js` — WebGL2 unavailable | Chrome with `--disable-gpu --disable-software-rasterizer` (or similar) | The page stays black, and the console shows a WebGL2 error from `SynesthesiaCanvas.create` |
| TC-010 | `preset` — single | `?preset=fractopia` | Rovastar - Fractopia (Upspoken Mix) loads and the page never cycles, even with `cycle` set |
| TC-011 | `preset` — list + `cycle` + `blend` | `?preset=fractopia,swarm&cycle=10&blend=3` | The page alternates between the two presets about every 10 s with a visible ~3 s transition and wraps around |
| TC-012 | `preset` — name with `+` / `&` | `?preset=Eo.S.%20%2B%20Geiss` and `?preset=glowsticks` | Both resolve to "Eo.S. + Geiss - glowsticks v2 02 (Relief Mix)" |
| TC-013 | `preset` — unknown names | `?preset=nope,alsonope` | Console warnings for both. The page falls back to all presets and still renders |
| TC-014 | `cycle=0` | `?cycle=0` | The page stays on the first preset indefinitely (watch >60 s) |
| TC-015 | Defaults | No query string | All 10 presets in sorted order, cycling every 30 s with a 2 s blend, fps capped at 60, `audio=default` (C-007 accepted). The effective settings appear in the console |
| TC-016 | Invalid numbers | `?cycle=abc&blend=-5&fps=xyz` | Each falls back or clamps as specified with a console warning. The page renders normally |
| TC-017 | `fps` cap | `?fps=30` and `?fps=0` | Measured display fps is ≈30 with `fps=30`. With `fps=0`, fps follows the display or OBS rate (uncapped) |
| TC-018 | Broken preset in list | A list containing a preset that fails to compile (simulated, or found in testing) | The failure is logged and the next preset loads. The page never stops on a black frame and never loops endlessly |
| TC-019 | `audio=default` | Chrome fake audio | The visuals react to the fake tone. The console shows `AudioInput start (microphone)` and the chosen device |
| TC-020 | `audio=<label substring>` | A machine with a named input (e.g. a "MacBook Pro Microphone" substring, or BlackHole) | That device is used. The console shows its full label |
| TC-021 | `audio=nonexistent` | Any machine | A console warning lists the available input labels, then the page falls back to the default input (C-006 accepted). Visuals continue |
| TC-022 | Microphone permission denied | Chrome with the mic blocked for localhost | A console error appears. Visuals keep animating without audio. No uncaught rejection |
| TC-023 | Insecure context | Page opened via a LAN IP over plain http (not localhost) | The console says audio is unsupported in an insecure context. Visuals render |
| TC-024 | Device unplugged mid-run | Using a USB or virtual input, then remove it | The input stops and logs. Visuals continue. No auto-retry in Phase 1 (C-006 accepted) |
| TC-025 | Autoplay without gesture | Normal Chrome (no autoplay flag) | The console logs that the AudioContext is suspended. Audio starts after the first click or keypress on the page (C-008 accepted), and no visible UI appears |
| TC-026 | Visibility pause | Local page, then switch tabs or minimise | Rendering pauses while hidden and resumes on return without errors |
| TC-027 | *Removed* | — | BS-006 (Pages) was dropped at Gate 1 |
| TC-028 | *Removed* | — | BS-006 (Pages) was dropped at Gate 1 |
| TC-029 | `audio=none` | Local page | No `getUserMedia` call and no permission prompt. Visuals animate without audio. The console says audio is off |
| TC-030 | OBS macOS — render | OBS ≥31 launched with `--enable-media-stream`, browser-source HW acceleration on, a 1920×1080 Browser Source on the local URL (`http://localhost:8080/`) | The source shows the visualizer filling 1920×1080. Other sources in the scene are unaffected |
| TC-031 | OBS macOS — loopback (Done when) | BlackHole 2ch plus a Multi-Output Device, music routed through it, URL `?audio=BlackHole&cycle=15` | The visualizer reacts to the music and cycles presets every ~15 s |
| TC-032 | OBS Windows — loopback (Done when) | Windows 10/11, OBS with the flag on its shortcut, VB-Cable, URL `?audio=CABLE%20Output&cycle=15` | Same as TC-031 |
| TC-033 | OBS — flag missing | OBS started without `--enable-media-stream` | Visuals render, and the console (Interact / remote debugging) shows the audio error. The README troubleshooting section covers it |
| TC-034 | OBS — source hidden/shown | Toggle the source's visibility, or switch scenes | It pauses while hidden and resumes on show. The audio input keeps working after resume |
| TC-035 | README | A fresh reader on macOS | Following `browser-source/README.md` alone (local serving) gets to TC-030/TC-031. The parameter table matches TC-010 to TC-021 and TC-029 behaviour. No Pages or hosted URL is mentioned |
| TC-036 | `LICENSE` (repo root) | LIC-001 done | It is byte-identical to `https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt` (same sha256 as a fresh download), starts "GNU GENERAL PUBLIC LICENSE / Version 2, June 1991", and has no header added |
| TC-037 | `PLAN.md` D1 row | LIC-001 done | D1 reads Locked (2026-11-23), GPL-2.0-or-later, and the MIT core keeps its own licence. The "No `LICENSE` file until then" sentence is gone. No other decision row changed |
| TC-038 | `docs/decisions.md` + `docs/README.md` | LIC-002 done | `decisions.md` has a D1 section (decision, date, rationale, consequences, link to PLAN.md). `docs/README.md` lists it, and its "no pages" line is accurate |
| TC-039 | Licence consistency | LIC-001/LIC-002/BS-007 done | `PLAN.md`, `docs/decisions.md` and `browser-source/README.md` all say GPL-2.0-or-later for this repo, MIT for the core, and the BeatDrop/CC-BY-NC-SA terms for the presets |
| TC-040 | Submodule `external/synesthesia-vibe/` | SPK-001 done, fresh clone | `git submodule update --init --recursive` checks out the `v0.7.0` commit; `git -C external/synesthesia-vibe describe --tags` prints `v0.7.0`. The submodule working tree stays clean after every build |
| TC-041 | Core build as subdirectory (Q1) | SPK-002 done | `PLAN.md` records yes/no with the concrete CMake errors. The fallback build produces one arm64 `libSynesthesiaVibeCore.a` plus `include/synesthesia-vibe/synesthesia-vibe.h` in an install prefix outside the submodule |
| TC-042 | `plugin/` configure + build | SPK-003 done, Xcode 27.2, CMake 4.4.5, `VCPKG_ROOT` set | Configuring from a clean checkout downloads the buildspec deps into `.deps/`, builds libobs and the core, and produces an arm64 `.plugin` bundle. `git status` shows no build output and no `.deps/` |
| TC-043 | OBS module load | SPK-003 done, plugin in `~/Library/Application Support/obs-studio/plugins/`, OBS 33.0.3 (OpenGL) | The OBS log shows the module loaded with no errors, and "SynesthesiaVibe Visualizer" appears in Add Source |
| TC-044 | Renderer check — OpenGL (Q5) | SPK-004 done, OBS on OpenGL | The source registers. The log names the detected renderer |
| TC-045 | Renderer check — non-OpenGL (Q5) | SPK-004 done; the **user** switches OBS to Metal and restarts (S-008) | The plugin logs a clear "needs the OpenGL renderer" warning, registers no source, and OBS runs normally. The user switches back afterwards |
| TC-046 | Rendering inside `video_render` (Q2) | SPK-005 done, a test scene collection with the visualizer + image + text + colour sources | The visualizer fills its 1920×1080 source, right way up, animating (synthetic audio). The other sources draw correctly for ≥60 s and across scene switches |
| TC-047 | GL state (Q3) | SPK-006 done | The before/after diff of OBS-tracked GL state around `sv_render_frame` is empty with the chosen approach. Other sources stay correct. OBS Stats render time stays reasonable at 1080p60 |
| TC-048 | glad isolation (Q4) | SPK-007 done | `otool -hv` shows TWOLEVEL. `nm -gU` on the plugin exports only the module entry points; the core's `glad_gl*` symbols are not exported |
| TC-049 | Source teardown | SPK-005/006 done | Removing the source, switching scene collections and quitting OBS log no GL errors and no crash. The OBS log shows no leak warnings for the source |
| TC-050 | PLAN.md spike write-back | SPK-008 done | Each of the five "Spike first" bullets has a dated answer with an evidence pointer. D4, Risks and Upstream asks are updated. Upstream drafts exist in the session dir and nothing has been filed |
