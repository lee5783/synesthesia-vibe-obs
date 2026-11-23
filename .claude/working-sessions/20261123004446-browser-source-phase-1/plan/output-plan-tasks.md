# Tasks

> **Revision 2 (after Gate 1, 2026-11-23).**
> - BS-006 (Pages) is removed.
> - D1 tasks LIC-001 and LIC-002 are added.
> - Phase 2 spike tasks SPK-001 to SPK-010 are added. They are **blocked on the second Gate 1**
>   (concerns S-001 to S-012) before any spike code is written.
> - BS-001 is unchanged; it is in progress in parallel.

| ID | Description | Specialist | Dependencies | Order | Status | Notes |
|----|-------------|------------|--------------|-------|--------|-------|
| BS-001 | `browser-source/fetch-dist.sh [version]`: download the web dist zip and `SHA256SUMS.txt` from the pinned core release, verify, unpack into `browser-source/dist/` | developer | — | 1 | in_progress | PLAN item 1. Version handling per C-004. **Definition unchanged** |
| BS-002 | Extend `fetch-dist.sh` to fetch presets, textures and the core's LICENSE/NOTICE from the pinned tag's source tarball into `dist/`, and generate `dist/resources/manifest.json` | developer | BS-001 | 2 | pending | C-001/C-002/C-010 resolved: local only |
| BS-003 | `browser-source/index.html` + `browser-source/obs-page.js`: full-window canvas, black background, no UI; loads module, resources and the first preset | developer | BS-002 | 3 | pending | PLAN item 2 |
| BS-004 | URL settings `preset`, `cycle`, `blend`, `fps` (and parse `audio`); preset-list resolution; auto-cycle through the engine timer | developer | BS-003 | 4 | pending | PLAN item 3. Defaults per C-005/C-007 (accepted) |
| BS-005 | Audio input from the device picked by `audio=` (label match, default, fallback, `none`, error handling) | developer | BS-004 | 5 | pending | PLAN item 4. C-006/C-008 (accepted) |
| ~~BS-006~~ | ~~GitHub Pages workflow~~ | — | — | — | removed | Dropped at Gate 1; D5 stays open. C-011/C-012 are moot |
| BS-007 | `browser-source/README.md`: OBS setup for **local serving only** (URL + params, 1920×1080, launch flag per OS, loopback routing, OBS settings, troubleshooting, credits) | developer | BS-005 | 6 | pending | PLAN item 6. No Pages URL |
| BS-008 | Agent OBS check on this Mac (OBS 33.0.3, built-in mic) plus a manual checklist for the user's BlackHole + VB-Cable runs | developer | BS-005, BS-007 | 7 | pending | "Done when". C-003 resolved: Phase 1 is done only after the user reports back |
| LIC-001 | D1: root `LICENSE` with the canonical GPL-2.0 text fetched from gnu.org; lock D1 in `PLAN.md` | developer | — | 8 | pending | User approved the `PLAN.md` edit. Can run in parallel with BS-002…BS-008 |
| LIC-002 | D1: create `docs/decisions.md` (D1 locked) and list it in `docs/README.md` | document-writer | LIC-001 | 9 | pending | `docs/**` is reserved for the document-writer (`developer.md`) |
| BS-009 | PR-style review of BS-001…BS-008, LIC-001 and LIC-002 against this plan and CLAUDE.md | code-reviewer | BS-001…BS-008, LIC-001, LIC-002 | 10 | pending | |
| BS-010 | Doc pass: generate the first `docs/` pages for `browser-source/`, update `docs/README.md`, the CLAUDE.md docs note and the PLAN.md Phase 1 status | document-writer | BS-009 | 11 | pending | C-013 resolved (default accepted). D5 stays open |
| SPK-001 | Add the core as a git submodule at `external/synesthesia-vibe/`, pinned to `v0.7.0` | developer | second Gate 1 | 12 | pending | **Blocked on the second Gate 1** |
| SPK-002 | Spike Q1: does the core's CMake build as a subdirectory with tools/samples off? Confirm empirically, then prove the fallback (separate configure + install into a prefix) | developer | SPK-001 | 13 | pending | S-003, S-005. Answer goes in PLAN.md |
| SPK-003 | Minimal `plugin/` CMake project on the obs-plugintemplate layout (libobs from the template's buildspec) that links the core library and registers an empty "SynesthesiaVibe Visualizer" source; build, install locally, load in OBS 33.0.3 | developer | SPK-002 | 14 | pending | S-001…S-004, S-006, S-007 |
| SPK-004 | Spike Q5: renderer check — `gs_get_device_type()` at load; on a non-OpenGL renderer, log clearly and register nothing | developer | SPK-003 | 15 | pending | S-008. Answer goes in PLAN.md |
| SPK-005 | Spike Q2: render with `sv_render_frame` into OBS's texrender FBO between `gs_texrender_begin`/`end` in `video_render`, then draw the result with OBS's default effect | developer | SPK-004 | 16 | pending | S-009, S-010. Answer goes in PLAN.md |
| SPK-006 | Spike Q3: GL-state audit — what the core changes vs what `libobs-opengl` tracks; decide between save/restore and a shared context, and prove the choice with other sources in the scene | developer | SPK-005 | 17 | pending | Answer goes in PLAN.md |
| SPK-007 | Spike Q4: glad loader isolation — verify with `nm`/`otool` that the plugin's glad symbols cannot bind to `libobs-opengl`'s, and hide all but the module entry points | developer | SPK-005 | 18 | pending | Answer goes in PLAN.md (+ Linux note) |
| SPK-008 | Consolidate the spike results in PLAN.md (each question answered, D4 state, risks, upstream asks drafted but not filed) and record time spent against the 2-day box | developer | SPK-002…SPK-007 | 19 | pending | S-011 |
| SPK-009 | PR-style review of SPK-001…SPK-008 | code-reviewer | SPK-008 | 20 | pending | |
| SPK-010 | Doc pass for the spike: `docs/` pages for `plugin/` and the submodule build (structure, stack, integrations); add decisions to `docs/decisions.md` if D4 changes | document-writer | SPK-009 | 21 | pending | |

**Specialist values:** `developer` | `code-reviewer` | `document-writer`
**Status:** `pending` | `in_progress` | `done` | `removed`

---

## Shared rules for every developer task

- Read `output-plan-summary.md` → *Web dist API facts* first. It is the verified API surface, so
  do not re-research the dist.
- Plain ES modules, no bundler, no npm, no `package.json`. POSIX `sh`/`bash` for the script.
- **No code comments.** The standard file header from root `CLAUDE.md` is required in every new
  source file, with author `lee5783`, project `SynesthesiaVibeOBS` and date `2026-11-23`, in the
  language's own comment syntax:
  - `obs-page.js`: `//` lines.
  - `index.html`: the same block inside one `<!-- -->` comment, after `<!doctype html>`.
  - `fetch-dist.sh`: `#` lines after the shebang.
  - `README.md`: no header (C-014, accepted).
- The dist's own files are a style reference for the JS: 4-space indent, double quotes,
  semicolons, `const` UPPER_SNAKE constants at the top, small named functions. Nothing in this
  repo can serve as a sibling yet.
- kebab-case file names. No absolute machine paths, secrets or personal emails (public repo).
- No unit tests. Evidence goes to `<root>/develop/evidence/BS-00N-<what>.<ext>`. Downscale stills
  to ≤1280 px wide. Logs and JSON are git-ignored, so quote the key lines in `output-BS-00N.md`.
- Serve locally with `python3 -m http.server 8080 --directory browser-source` and open
  `http://localhost:8080/`. Localhost is a secure context, so getUserMedia works.
- Do not commit or push unless the orchestrator asks.

---

## BS-001 — fetch-dist.sh: download, verify, unpack

**What to do**

- Create `browser-source/fetch-dist.sh` (executable, `#!/usr/bin/env bash`, `set -euo pipefail`).
- Take a version argument. Accept `0.7.0` and `v0.7.0` and normalise to the tag `v<ver>` and the
  asset name `synesthesia-vibe-web-<ver>.zip`. Without an argument, use the default from C-004:
  the pinned `0.7.0` held in one constant in the script.
- Download the zip and `SHA256SUMS.txt` with `curl -fsSL` from
  `https://github.com/lee5783/synesthesia-vibe/releases/download/v<ver>/…` into a `mktemp -d`
  directory, removed on exit by a `trap`.
- Verify: take the line for the zip from `SHA256SUMS.txt` and check it with `shasum -a 256 -c`,
  falling back to `sha256sum -c` when `shasum` is missing. On a mismatch, a missing line or a
  failed download, exit non-zero with a one-line message on stderr and leave any existing `dist/`
  untouched.
- Unpack: the zip is flat. Replace `browser-source/dist/` atomically: unzip into a temp dir, then
  remove the old `dist/` and move the new one in. Resolve paths relative to the script's own
  directory so it works from any cwd.
- Print a short summary (version, file count) on success.

**Acceptance criteria**

- `browser-source/fetch-dist.sh 0.7.0` from the repo root and `./fetch-dist.sh` from inside
  `browser-source/` both produce `browser-source/dist/` with the 15 files, and `dist/VERSION`
  reads `0.7.0`.
- A corrupted download or wrong checksum exits non-zero and leaves an existing `dist/` intact.
  Test it by pointing the script at a tampered copy or a bogus version, without changing the
  committed script.
- A non-existent version exits non-zero with a clear message.
- `git status` shows no files under `browser-source/dist/`.
- Works with the macOS stock tools (bash 3.2, BSD `mktemp`, `shasum`) and on `ubuntu-latest`. Avoid
  bash-4-only features.

**Evidence:** `BS-001-fetch-dist.log` with the terminal output of a success run, a bogus-version
run and a checksum-failure run (exit codes shown), and a `ls browser-source/dist` listing. Quote
the result in the output file.

---

## BS-002 — presets, textures, notices and manifest

C-001, C-002 and C-010 are resolved at Gate 1: the tag's source tarball, into the git-ignored
`dist/resources/`, local serving only, never published.

**What to do**

- Extend `fetch-dist.sh`: after the dist is unpacked, download the core **source tarball for the
  same tag** (`https://github.com/lee5783/synesthesia-vibe/archive/refs/tags/v<ver>.tar.gz`).
  Extract only:
  - `resources/mvp-presets/*.milk` → `dist/resources/presets/`
  - `resources/beatdrop/textures/*` (no dotfiles) → `dist/resources/textures/`
  - `LICENSE` and `NOTICE` → `dist/`
- Generate `dist/resources/manifest.json` in the `ResourceLoader` format:
  `{"presets":["presets/<file>.milk",…],"textures":["textures/<file>",…],"audio":[]}`, sorted, with
  paths relative to the manifest. The `.milk` names contain spaces, `&`, `'`, `+` and parentheses;
  JSON only needs `"` and `\` escaped. Use `python3 -I` or careful shell, the developer's choice.
  Both are present on macOS and `ubuntu-latest`.
- The tarball has no published checksum, so record that as a limitation (C-010). The release-zip
  verification from BS-001 stays as is.

**Acceptance criteria**

- After running the script, `dist/resources/manifest.json` lists 10 presets and 23 textures and is
  valid JSON (`python3 -I -m json.tool`).
- `dist/LICENSE` and `dist/NOTICE` exist.
- A re-run replaces everything cleanly.
- Nothing under `dist/` is tracked by git.

**Evidence:** `BS-002-resources.log` with the script output, `ls dist/resources/presets | wc -l`,
`ls dist/resources/textures | wc -l`, and the first lines of `manifest.json`.

---

## BS-003 — index.html + obs-page.js (render path, no settings yet)

**What to do**

- `browser-source/index.html`:
  - Header comment, `<meta charset>` and `<meta name="viewport">`.
  - Inline CSS: `html, body { margin:0; height:100%; background:#000; overflow:hidden }`, and a
    canvas that fills the viewport (`display:block; width:100vw; height:100vh`).
  - One `<canvas>` and `<script type="module" src="./obs-page.js">`.
  - No other visible elements.
- `browser-source/obs-page.js`:
  1. Import from `./dist/synesthesia-vibe.js`.
  2. `loadSynesthesiaModule()`.
  3. `new ResourceLoader(module).load("./dist/resources/manifest.json")`.
  4. `SynesthesiaCanvas.create(canvas, module, { configuration: {...}, onError })`.
  5. `view.loadPreset(firstPreset, 0)`.
- Every failure goes to `console.error` with a clear prefix: missing dist, manifest 404, WebGL2
  unavailable, preset compile error. The page stays black and shows no text (pending C-009).
- Structure the module so BS-004 and BS-005 add to it without rewrites: small functions such as
  `readSettings`, `resolvePresets`, `startAudio`.

**Acceptance criteria**

- Served locally and opened in Chrome at a 1920×1080 viewport, the page shows an animating
  preset edge-to-edge on a black background, with no scrollbars and no UI.
- With `dist/` missing, the page stays black and the console names the missing file.
- No console errors in the happy path.

**Evidence:**

- `BS-003-render-1920x1080.png`: Chrome, served from localhost, 1920×1080 window, downscaled to
  1280 wide. Use the argent Chromium tools if they are installed; otherwise headless Chrome:
  `"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" --headless=new --window-size=1920,1080 --use-angle=metal --enable-unsafe-swiftshader --screenshot=… --virtual-time-budget=5000 http://localhost:8080/`.
  Confirm WebGL2 actually rendered and the frame is not blank.
- `BS-003-missing-dist-console.txt`: the console line for the missing-dist case.

---

## BS-004 — URL settings and preset cycling

**What to do.** Read `location.search` with `URLSearchParams`:

| Param | Meaning | Default (C-007) | Validation |
|---|---|---|---|
| `preset` | one name or a comma-separated list | all manifest presets, sorted | split on `,`, trim, drop empties; each item resolved per C-005 (case-insensitive match on the basename without `.milk`, exact match first, then unique substring); unresolved items → `console.warn` and skip; nothing resolved → all presets |
| `cycle` | auto-cycle seconds, `0` = off | `30` | finite number ≥ 0, else default + warn; ignored when only one preset resolves |
| `blend` | transition seconds | `2` | finite number in 0–10 (clamp), else default + warn |
| `audio` | device label or `default` | `default` | parsed and stored here; used in BS-005 |
| `fps` | frame-rate cap, `0` = uncapped | `60` | finite number ≥ 0, else default + warn |

- Wiring, matching the core sample:
  - `configuration.maxFps = fps` at `SynesthesiaCanvas.create`.
  - `view.setTransitionConfig({ softCutSeconds: blend, presetDuration: cycle, presetDurationRand: 0, hardCutsEnabled: false })`.
  - `view.setBlendPattern(SynesthesiaContext.BLEND_PATTERN_RANDOM)`.
  - `view.onPresetRequest = req => loadNext(req.reason === "hardCut" ? 0 : req.suggestedBlendSeconds)`,
    where `loadNext` advances through the resolved list in order and wraps around.
  - The first preset loads with blend `0`.
- If a preset fails to load (shader or parse error), log it, then try the next one. Stop after one
  full pass so a list of broken presets cannot spin forever.
- Log the effective settings once at startup (`console.info`) so OBS's log or devtools shows what
  was applied.

**Acceptance criteria**

- `?preset=fractopia` shows Rovastar - Fractopia and does not cycle.
- `?preset=fractopia,swarm&cycle=10&blend=3` alternates between the two about every 10 s with a
  visible ~3 s blend.
- `?cycle=0` stays on the first preset.
- `?fps=30`: `onDebugInfo.display_fps` (temporarily hooked, or read in devtools) reads ≈30.
- Bad values (`cycle=abc`, `blend=-5`, `preset=nope`) fall back as specified, with a warning, and
  the page never goes blank.

**Evidence:**

- `BS-004-cycle-a.png` and `BS-004-cycle-b.png`: the same URL captured before and after one cycle
  interval, showing different presets.
- `BS-004-console.txt`: the effective-settings line, preset-request lines and fallback warnings for
  the bad-value URL.
- Screenshots are downscaled.

---

## BS-005 — audio input by device label

**What to do**

- When `AudioInput.isMicrophoneSupported()` is false (not a secure context, or no AudioWorklet),
  log it and run without audio. The visuals continue.
- For `audio=default`, call `AudioInput.fromMicrophone(view)` with no `deviceId`.
- For any other value:
  1. Open a throwaway `getUserMedia({ audio: true })` and stop its tracks right away. This only
     unlocks device labels.
  2. `enumerateDevices()`, filtered to `kind === "audioinput"`.
  3. Match per C-006: case-insensitive, exact label first, then substring, so `audio=blackhole`
     matches "BlackHole 2ch".
  4. `AudioInput.fromMicrophone(view, { deviceId: { exact: id } })`.
  5. On no match: `console.warn` listing the available labels, then fall back to the default
     device (C-006, accepted).
- `audio=none`: skip audio entirely and open no `getUserMedia` (C-006 (d), accepted).
- Error handling:
  - Permission denied or no device → `console.error` with the `SynesthesiaError` message, plus the
    DOMException name from the page's own label-unlock call when there was one. The visuals keep
    running without audio.
  - The track ends (device unplugged) → log it. No retry in Phase 1 (C-006, accepted).
- Autoplay: if the `AudioContext` is suspended after start, add a one-shot invisible
  `pointerdown`/`keydown` listener that resumes it (C-008). The page stays free of visible UI.
- Log the chosen device label and `inputSampleRate` once. The dist logs
  `AudioInput start (...)` itself.

**Acceptance criteria**

- Chrome with fake-device flags: the visuals visibly react to the fake tone and
  `audioInput.framesPushed` grows. Flags:
  `--use-fake-ui-for-media-stream --use-fake-device-for-media-stream --autoplay-policy=no-user-gesture-required`.
- `audio=<real label substring>` picks that device; the console shows the label.
- `audio=nonexistent` follows the fallback and lists the available labels.
- Denied permission leaves the page rendering with a clear console error.

**Evidence:**

- `BS-005-fake-device.png`: Chrome with the fake device, visuals reacting.
- `BS-005-console.txt`: device list, chosen label, the `AudioInput start` line, and lines for the
  fallback and denied cases.
- `BS-005-default-mic.png`: real Chrome with the built-in mic and music playing nearby or from the
  speakers, if achievable.

---

## BS-006 — removed

The GitHub Pages workflow was dropped at Gate 1 ("Skip Pages for now"). D5 stays open in
`PLAN.md`. No `.github/workflows/` file is written in this run. Nothing else may reference a Pages
URL.

---

## BS-007 — browser-source/README.md

**What to do.** Write OBS setup instructions for streamers. Use plain prose, short sections and
numbered steps. Cover:

1. What it is, and the known limit: it cannot hear OBS's own mix.
2. Source URL, **local serving only**: `fetch-dist.sh`, then
   `python3 -m http.server 8080 --directory browser-source`, then `http://localhost:8080/`. Do not
   mention a Pages or hosted URL. Say that the server must keep running while OBS uses the source,
   and that localhost counts as a secure context.
3. Browser Source properties: Width 1920, Height 1080. Leave "Use custom frame rate" off or match
   `fps`. Mention the "Shutdown source when not visible" behaviour: the page pauses while hidden.
4. URL parameter table (`preset`, `cycle`, `blend`, `audio` including `none`, `fps`) with
   defaults, examples and the name-matching rule. Note that `+` and `&` in names must be URL-encoded, or a substring that
   avoids them can be used.
5. Launch flag `--enable-media-stream`:
   - macOS: `open -a OBS --args --enable-media-stream`, plus the microphone permission for OBS in
     System Settings → Privacy & Security → Microphone.
   - Windows: append the flag to the shortcut target.
   - Linux: one line.
6. Loopback routing:
   - macOS: BlackHole 2ch plus a Multi-Output Device in Audio MIDI Setup so you still hear the
     music.
   - Windows: VB-Cable, routing the player to CABLE Input with "Listen to this device" on.
   - Then `audio=BlackHole` / `audio=CABLE Output`.
7. OBS setting: Settings → Advanced → Sources → "Enable Browser Source Hardware Acceleration" on
   (WebGL2 needs it).
8. Troubleshooting:
   - Black screen → check the console via "Interact", or OBS's `--remote-debugging-port`.
   - No reaction → check the flag, the permission and the device label.
9. Credits and licences:
   - The core is MIT; its LICENSE and NOTICE are in `dist/`.
   - The presets are BeatDrop's, with author names in the file names. Include the CC-BY-NC-SA note
     for ShaderToy-converted presets, and say they are fetched for local use only.
   - This repo is GPL-2.0-or-later (LIC-001).
10. Also mention the GPU cost of several sources (C-019), "tested on OBS 33.0.3" (C-018), and one
    untested Linux line (C-020).

No header block (C-014). Do not touch the root `README.md`. Facts not verified this session must be
marked as such or checked: exact OBS menu names on 33.x, and VB-Cable/BlackHole UI names.

**Acceptance criteria**

- A reader can go from nothing to a working source by following the README alone.
- Every parameter and default matches the BS-004/BS-005 code.
- No absolute machine paths or personal data.

**Evidence:** `BS-007-readme-walkthrough.md`, a short note that the developer followed the
local-serving steps verbatim and they worked (commands and results), plus which steps could not be
exercised (Windows, loopback).

---

## BS-008 — OBS end-to-end check (best effort) + manual checklist

**What to do**

- On this Mac (OBS 33.0.3 installed; **no loopback device installed**):
  1. Quit any running OBS, then launch it with `--enable-media-stream`.
  2. Add a 1920×1080 Browser Source pointing at `http://localhost:8080/?cycle=10&blend=2&audio=default`.
  3. Play music through the speakers so the built-in mic hears it.
  4. Capture the OBS window with `screencapture -l <window-id>` (or `-R`) twice, across one cycle.
  5. Keep the relevant OBS log lines (browser-source console lines appear in the OBS log).
- If OBS cannot be driven (it needs GUI interaction the agent cannot perform, or permission
  prompts), say so explicitly with the fresh blocker. Do not guess.
- Do **not** install BlackHole or any driver: that needs admin rights and user consent (C-003).
- Write `BS-008-manual-checklist.md` for the user, covering the "Done when" on macOS with BlackHole
  and on Windows with VB-Cable: the steps, the expected result and where to put screenshots.

**Acceptance criteria**

- Either OBS screenshots show the visualizer filling a 1920×1080 source and changing presets,
  ideally reacting to sound, or a freshly verified blocker is recorded.
- The checklist exists and covers both OSes.

**Evidence:**

- `BS-008-obs-a.png` and `BS-008-obs-b.png`.
- `BS-008-obs-log.txt` (trimmed).
- `BS-008-manual-checklist.md`.

---

## LIC-001 — D1: LICENSE file and PLAN.md

The user decided at Gate 1: GPL-2.0-or-later, with a LICENSE file in this run, D1 locked in
`PLAN.md`, and the MIT core keeping its own licence.

**What to do**

- Fetch the canonical text with
  `curl -fsSL https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt -o LICENSE` at the repo root.
  **Do not type it from memory or edit it.** It is the licence text, verbatim, with no header added
  (the file header rule does not apply to a licence file).
- Sanity-check it:
  - The first lines read "GNU GENERAL PUBLIC LICENSE / Version 2, June 1991".
  - It contains "END OF TERMS AND CONDITIONS".
  - Line endings are LF.
  - Record `wc -l` and `shasum -a 256` in the output file.
- `PLAN.md`:
  - D1 row → state **Locked (2026-11-23)**: "GPL-2.0-or-later. `LICENSE` holds the GPL-2.0 text; the
    'or any later version' grant is stated here and in `docs/decisions.md`. The MIT core
    (`external/synesthesia-vibe/`, release assets) keeps its own licence and NOTICE."
  - Remove "No `LICENSE` file until then, so the repo is all-rights-reserved by default."
  - Update the "Last updated" line.
  - Leave the other decisions as they are (D5 stays open).
- Do not add per-file SPDX lines and do not touch the root `README.md` (C-021, default).

**Acceptance criteria**

- `LICENSE` is byte-identical to the gnu.org file as fetched.
- `PLAN.md` D1 reads Locked, with nothing else in PLAN.md changed except the date line.
- `git diff --stat` touches only `LICENSE` and `PLAN.md`.

**Evidence:** `LIC-001-license.log` with the curl command, head/tail of the file, `wc -l`,
sha256, and the `git diff PLAN.md` excerpt, quoted in the output file.

## LIC-002 — D1: docs/decisions.md

The document-writer creates `docs/decisions.md`, the page CLAUDE.md asks for once the first PLAN.md
decision locks. One section per locked decision; for now only D1:

- the decision
- the date
- the rationale (libobs is GPL-2.0-or-later, so a linking plugin must be GPL-compatible)
- the consequences (the repo is GPL-2.0-or-later, the "or later" grant, the MIT core and its
  vendored notices stay under their own licences, the presets keep their BeatDrop / CC-BY-NC-SA
  terms and are not redistributed by this repo)
- a link back to `PLAN.md` → *Decisions*

Also add a `decisions.md` row to the page table in `docs/README.md` and adjust its "No pages exist
yet" line so it stays true. The user approved these docs edits at Gate 1.

**Acceptance criteria:** `docs/decisions.md` exists. `docs/README.md` lists it. The text agrees
with the PLAN.md D1 row word for word on the licence identifier.

## BS-009 — review

Review all Phase 1 and D1 files against this plan, root `CLAUDE.md` (no comments, headers,
naming, public-repo hygiene, no unit tests expected) and `PLAN.md` Phase 1. Focus on:

- Checksum verification that cannot be bypassed: what happens on a missing line or a partial
  download.
- Safe `dist/` replacement.
- URL-param validation: preset names are inserted only into the FS path, never into the DOM.
- Audio error paths, including `audio=none`.
- The README matching the code and containing no Pages URL.
- `LICENSE` matching gnu.org.
- The D1 wording being consistent across `PLAN.md` and `docs/decisions.md`.

Do not fix anything.

## BS-010 — doc pass (Phase 1)

The document-writer runs the `codebase-mapper` skill for `tech` and `arch` (at least), so the first
`docs/` pages describe `browser-source/`:

- `structure.md`
- `architecture.md`
- `integrations.md` (the fetch script, the release and tarball URLs, local serving)
- `stack.md`

It updates `docs/README.md` to list exactly the pages that exist.

C-013 is resolved with the default accepted. The document-writer may also update:

- the root `CLAUDE.md` note "The `docs/` pages do not exist yet…" and its routing table, to match
  the pages that now exist
- the `PLAN.md` status line and Phase 1 progress, marking Phase 1 as "implemented, awaiting the
  user's loopback check", not done (C-003)

D5 stays open.

---

# Phase 2 spike (SPK-*)

**Blocked on the second Gate 1** (concerns S-001…S-012 in `output-plan-concerns.md`). Nothing below
starts until the orchestrator records those answers. The time-box is **2 days** in total across
SPK-001…SPK-008, per `PLAN.md` "Spike first". If a question cannot be answered inside the box,
write "open" with what was learned and move on. Use `HARNESS.md` *Stuck / long-running task
capture* for any wall you hit.

## Shared rules for every SPK developer task

- Read `output-plan-summary.md` → *Phase 2 spike facts* first. Toolchain, core CMake behaviour,
  the header, the GL census and the template are already verified there.
- Load the `shader-programming` skill before touching GL state, render targets or shaders
  (`developer.md` preflight). `cpp-coding-standards` and `cpp-testing` are manual-only and must
  not be loaded.
- C++17 for plugin code, and C only where libobs callbacks want it. **No code comments.** The
  standard header goes on every new file the spike writes: `//` for C/C++, `#` for CMake. Files
  copied verbatim from obs-plugintemplate follow S-006.
- Each SPK task writes its answer into `PLAN.md` under the matching "Spike first" bullet, using an
  "Answer (2026-11-2x):" sub-bullet: a short finding plus a pointer to the evidence. Keep the
  question text intact.
- Never edit `external/synesthesia-vibe/`. Build output goes outside the submodule (`-B` outside
  it; nothing under `external/`). No absolute machine paths in committed files: use
  `$ENV{VCPKG_ROOT}`, `$HOME` and relative paths.
- Plugin install for testing goes to `~/Library/Application Support/obs-studio/plugins/` (create
  it). Remove or keep it per S-012 at the end.
- OBS evidence:
  - Quit OBS, then relaunch it from the terminal (`open -a OBS` or the binary with `--verbose`).
  - Capture the window with `screencapture -l <window-id>`, downscaled to ≤1280 px.
  - Trim the latest log from `~/Library/Application Support/obs-studio/logs/` to the plugin's lines
    (prefix them, e.g. `[synesthesia-vibe]`).
  - Logs are git-ignored bulk evidence, so quote the key lines in `output-SPK-00N.md`.
- If OBS needs GUI clicks the agent cannot perform (adding a source to a scene), check whether
  obs-websocket is enabled before falling back to the user. OBS 33 ships it: create the source
  with a websocket `CreateInput` request; S-010 covers the client. Otherwise record the fresh
  blocker and ask the orchestrator to have the user click.

## SPK-001 — core submodule

- `git submodule add https://github.com/lee5783/synesthesia-vibe.git external/synesthesia-vibe`,
  then check out tag `v0.7.0` inside it and stage the gitlink. Use the HTTPS URL; it is public.
- Confirm there are no nested submodules (none at `v0.7.0`) and that the working tree is clean.
- **Acceptance:**
  - `git submodule status` shows the `v0.7.0` commit.
  - `.gitmodules` has the one entry.
  - `git -C external/synesthesia-vibe describe --tags` prints `v0.7.0`.
- **Evidence:** `SPK-001-submodule.log`.

## SPK-002 — Q1: core as a CMake subdirectory

- **Step 1 (empirical "no" or "yes"):** write a throwaway parent `CMakeLists.txt` in the session
  scratch or a temporary build dir, not committed. It calls
  `add_subdirectory(external/synesthesia-vibe)` with `SYNESTHESIA_VIBE_GL_DEV=ON`,
  `SYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS=OFF`, `SYNESTHESIA_VIBE_BACKEND=gl` and the vcpkg
  toolchain. Configure and build `SynesthesiaVibeCore`, then record exactly what fails. Expected
  failures: `CMAKE_SOURCE_DIR` paths, the parent's flags being clobbered, and a vcpkg manifest that
  is not at the parent root.
- **Step 2 (fallback that works):**
  1. Configure the core on its own: `cmake -S external/synesthesia-vibe -B build/core-macos-arm64`
     plus the `macos-arm64` preset's cache variables, `SYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS=OFF`
     and `CMAKE_INSTALL_PREFIX=build/core-install`, using `$VCPKG_ROOT`.
  2. Build `SynesthesiaVibeCore`, then `cmake --install`.
  3. Record the outputs, their sizes, `lipo -info` (arm64) and the time taken.
  4. Choose how the plugin build drives this: `ExternalProject_Add` from `plugin/CMakeLists.txt`,
     or a `plugin/build-core.sh` the developer runs first (S-005).
- Check that CMake 4.4.5 accepts the core's `cmake_policy(SET CMP0091 OLD)` and
  `cmake_minimum_required(3.25)`. CMake 4 dropped compatibility only below 3.5, so this should
  pass, but verify.
- **Answer to PLAN.md:** subdirectory yes/no with the concrete reasons; the working build path; and
  a draft of upstream ask #1 ("library-only build usable via `add_subdirectory`: use
  `CMAKE_CURRENT_SOURCE_DIR`/`PROJECT_SOURCE_DIR`, don't FORCE global flag caches, guard
  top-level-only settings with `PROJECT_IS_TOP_LEVEL`").
- **Evidence:** `SPK-002-subdirectory.log` (failing configure excerpt) and `SPK-002-core-build.log`
  (success, plus `ls -l` of the install prefix).

## SPK-003 — minimal plugin/ on the obs-plugintemplate layout

- Bring in the template at a pinned commit (S-006). Files go under `plugin/`, matching PLAN.md
  *Repository layout*:
  - `plugin/CMakeLists.txt`
  - `plugin/CMakePresets.json`
  - `plugin/buildspec.json`
  - `plugin/cmake/**`
  - `plugin/src/`
  - `plugin/data/locale/en-US.ini`
  - Leave out the template's `.github/` (Phase 4) and `build-aux/` format scripts (no formatter
    here).
- Set `buildspec.json` `name`/`displayName`/`version`/`author`/`website` and the bundle ID per S-007.
  **No personal email:** use an empty or project-neutral value.
- Use obs-studio 31.1.1 and obs-deps as the template pins (S-001). Set `CMAKE_OSX_ARCHITECTURES=arm64`
  through a local preset or cache override (S-004). Generator per S-007: Xcode as the template
  does, or Ninja.
- Source:
  - `obs_module_load` registers one `obs_source_info` with id `synesthesia_vibe_visualizer`,
    display name "SynesthesiaVibe Visualizer" from the locale, and type `OBS_SOURCE_TYPE_INPUT`
    with `OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW`.
  - `create`/`destroy`, `get_width`/`get_height` = 1920×1080, and a `video_render` that draws
    nothing yet.
  - Link the core library from SPK-002 and call one harmless symbol (e.g. `sv_validate_preset_memory`
    on a tiny string, or just reference `sv_create`) so the link is proven.
- Build, then copy the `.plugin` bundle into `~/Library/Application Support/obs-studio/plugins/`.
  Ad-hoc signing is enough because OBS has `disable-library-validation` (S-002).
- **Acceptance:**
  - The OBS log shows the module loading with no errors.
  - The source type appears in OBS's "Add Source" list (screenshot).
  - The `.deps/` and build dirs are git-ignored.
- **Evidence:** `SPK-003-build.log`, `SPK-003-obs-log.txt` and `SPK-003-add-source.png`.

## SPK-004 — Q5: renderer check

- In `obs_module_load`, read `gs_get_device_type()` inside `obs_enter_graphics()`/`obs_leave_graphics()`.
  - If it is not `GS_DEVICE_OPENGL`, `blog(LOG_WARNING, …)` a clear message ("SynesthesiaVibe
    Visualizer needs the OpenGL renderer; current renderer is …") and return `true` without
    registering the source, so OBS still loads the module cleanly.
  - Verify how the graphics context is available at module load time; if it is not, move the check
    to the first `create`.
- Test under OpenGL, the current setting. Test Metal only per S-008: changing the user's renderer
  setting needs consent, a restart, and restoring the setting afterwards.
- **Answer to PLAN.md:** where the check runs and what the user sees. Note that this Mac runs
  "OpenGL 4.1 Metal - 90.5" (Apple's GL-on-Metal), which counts as OpenGL.
- **Evidence:** `SPK-004-obs-log-opengl.txt`, and `SPK-004-obs-log-metal.txt` if allowed.

## SPK-005 — Q2: render via sv_render_frame inside video_render

- `create`:
  1. Inside `obs_enter_graphics()`, call `sv_init_gl` with a get-proc that resolves from the OpenGL
     framework: `dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY)` + `dlsym`.
     That system path is fine; it is not a machine-specific path.
  2. Call `sv_create` with 1920×1080 and `resource_path`/`cache_path` per S-009.
  3. Load one preset with `sv_load_preset`. For the spike, read it from a source setting or a
     path relative to `obs_module_file()`, per S-009.
  4. Create a `gs_texrender_t` (`GS_RGBA`, `GS_ZS_NONE`).
- `video_tick` stores `dt`. No audio capture in the spike: push a synthetic sine through
  `sv_push_pcm_float` each tick so motion is visible.
- `video_render`:
  1. `gs_texrender_reset`.
  2. `gs_texrender_begin(tr, 1920, 1080)`.
  3. `glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING)`, then `sv_render_frame(ctx, dt, fbo)`.
  4. `gs_texrender_end`.
  5. Draw `gs_texrender_get_texture` with `obs_get_base_effect(OBS_EFFECT_DEFAULT)` (`gs_draw_sprite`).
- `destroy`: inside graphics, `sv_destroy` and `gs_texrender_destroy`.
- Check the frame for orientation: OBS texrender vs GL bottom-left origin may flip it. Check colour
  and alpha too (OBS expects premultiplied or opaque).
- **Acceptance:** a scene containing the visualizer source **plus** an image source, a text source
  and a colour source shows all of them correctly over at least 60 s and through scene switches.
  Before SPK-006 lands, artefacts are expected and must be recorded, not hidden.
- **Answer to PLAN.md:** does the frame look right? Does OBS still draw its other sources? Record
  the orientation fix if one was needed.
- **Evidence:** `SPK-005-obs-scene-a.png`, `SPK-005-obs-scene-b.png` and `SPK-005-obs-log.txt`
  (including `sv_last_error` and any GL errors from `sv_debug_info.gl_errors`).

## SPK-006 — Q3: GL-state audit and decision

- Instrument once (spike-only code, removed or kept per S-012): around `sv_render_frame`, snapshot
  the state listed in the summary's *GL calls in the core* with `glGet*`, and log a before/after
  diff for the first few frames. Cover each texture unit the core touches.
- Map each changed item against what `libobs-opengl` caches. Read obs-studio's
  `libobs-opengl/gl-subsystem.h`/`.c` at tag 31.1.1 in the `.deps` source tree, which is already
  downloaded by the template.
- Decide:
  - (a) A save/restore guard around `sv_render_frame` covering exactly the changed and tracked
    items. This is the recommended first choice: simplest, and the same context.
  - (b) A separate CGL context sharing objects with OBS's context, rendering into a shared texture.
  - Pick (a) unless it measurably fails or costs too much. Measure with OBS's "Stats" render-time
    figure, or the `sv_debug_info` fps.
- **Acceptance:** with the chosen approach, the SPK-005 scene draws every source correctly, the
  before/after diff of OBS-tracked state is empty, and OBS's render time stays sane at 1080p60.
- **Answer to PLAN.md:** the table of state changed vs tracked, the decision and why. This feeds
  upstream ask #2 ("document GL state changed by `sv_render_frame`, or offer a restore flag").
- **Evidence:** `SPK-006-state-diff.txt`, `SPK-006-obs-scene.png` and `SPK-006-stats.png`.

## SPK-007 — Q4: glad loader isolation

- Run `nm -m` on the built plugin binary. Check whether the core's `glad_gl*` / `gladLoad*` symbols
  are defined in the plugin and whether they are external.
- Run `nm -m` on `/Applications/OBS.app/Contents/Frameworks/libobs-opengl.dylib` and check whether
  it exports its own glad symbols.
- `otool -hv` on the plugin should show `TWOLEVEL` and no `-flat_namespace`.
- Hide everything except the module entry points: `-fvisibility=hidden` on the plugin, plus an
  exported-symbols list or `obs_module_*` visibility. Confirm with `nm -gU`.
- **Answer to PLAN.md:** macOS is isolated, or not, and why. Add a Linux note (needs
  `-fvisibility=hidden` or a version script, per PLAN), and decide whether upstream ask #3 is
  needed.
- **Evidence:** `SPK-007-symbols.txt`.

## SPK-008 — consolidate in PLAN.md

- In `PLAN.md`:
  - Ensure each of the five "Spike first" bullets has its answer.
  - Update D4 (proposed → the state the spike supports, e.g. "submodule + separate core build").
  - Update Risks with what the spike retired or found.
  - Mark which *Upstream asks* are now confirmed, with draft issue text in the session dir
    (`develop/evidence/SPK-008-upstream-asks.md`). **Do not file issues** without the user's
    go-ahead (S-011).
- Record the time spent against the 2-day box, and list what is ready for the Phase 2 feature
  tasks.
- **Evidence:** the `git diff PLAN.md` excerpt in the output file.

## SPK-009 — spike review

Review against `PLAN.md` "Spike first", this plan and `CLAUDE.md`. Focus on:

- threading: every `sv_*` call happens inside a graphics context, nothing from other threads
- `create`/`destroy` symmetry and leaks
- GL-state restore completeness
- symbol visibility
- no absolute paths
- headers and no comments
- template files handled per S-006
- git-ignore coverage of `.deps/` and build dirs

Do not fix anything.

## SPK-010 — spike doc pass

The document-writer updates `docs/structure.md`, `docs/stack.md` and `docs/integrations.md` with:

- `plugin/`
- the submodule
- the core build path and its vcpkg/CMake/Xcode prerequisites
- local plugin install

It adds a D4 section to `docs/decisions.md` if SPK-008 locked D4, and keeps `docs/README.md`
listing exactly the pages that exist.
