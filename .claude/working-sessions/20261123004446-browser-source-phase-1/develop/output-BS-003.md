# BS-003

## Subtasks executed

- `browser-source/index.html`:
  - Standard header as an HTML comment block, then `<!doctype html>`.
  - `charset`, `viewport` and `<title>`.
  - An empty data-URI icon, so the page makes no `favicon.ico` request and the console gets no 404.
  - Inline CSS: `html, body { margin:0; height:100%; background:#000; overflow:hidden }` and
    `#visualizer { display:block; width:100vw; height:100vh }`.
  - One `<canvas id="visualizer">` and `<script type="module" src="./obs-page.js">`. Nothing else
    is visible.
- `browser-source/obs-page.js`, plain ES module, one step function per stage:
  - `importDist()`: dynamic `import("./dist/synesthesia-vibe.js")`.
  - `loadSynesthesiaModule()`.
  - `loadPresets(sv, module)`: `new ResourceLoader(module).load("./dist/resources/manifest.json")`,
    which fails on an empty list.
  - `createView(sv, module, canvas)`: `SynesthesiaCanvas.create` with `onError` → prefixed
    `console.error`.
  - `loadPreset(view, path, blendSeconds)`: `view.loadPreset`, then `console.info` with the preset
    name. It logs nothing when superseded (a `null` result).
  - `main()` runs them in order and loads `presets[0]` (sorted) with blend `0`.
  - Each step runs through `stage(description, action)`, which rethrows with the step's
    description and keeps the original as `cause`. A single `main().catch` logs
    `obs-page: <description>` plus the cause. Nothing escapes uncaught.

## Files changed

- `browser-source/index.html` (new, untracked)
- `browser-source/obs-page.js` (new, untracked)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md` (BS-003 section added)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-BS-003.md` (new)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/BS-003/bs-003-render-report.md` (new, tracked)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/BS-003/` PNGs, `.txt` logs and `bs-003-cdp-check.mjs` (new, git-ignored by the evidence rule)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/session.log` (appended)

## What changed and why

This is PLAN Phase 1 item 2, the render path only. The page follows the core sample's order:
module, then resources, then canvas, then first preset. It builds on `SynesthesiaCanvas`, which
owns the rAF loop, resize, the visibility pause and context-loss recovery.

The dist is loaded with a dynamic `import()` instead of a static `import`. A static import fails
inside the module graph, where the page cannot catch it. The dynamic import lets a missing
`dist/` produce one prefixed line naming the file and the fix (`run browser-source/fetch-dist.sh`).

Slots for later tasks, with no stubs added now:
- **BS-004** adds `readSettings()` at the top of `main()`. It passes `{ configuration: { maxFps } }`
  into `createView`. Between `loadPresets` and the first `loadPreset` it adds `resolvePresets(presets, settings.preset)`
  and the transition / `onPresetRequest` wiring. The existing `loadPreset(view, path, blend)`
  helper is the unit its `loadNext` / try-next loop calls.
- **BS-005** adds `startAudio(sv, view, settings)` after the first preset. `sv` already exposes
  `AudioInput`.

Errors go to the console only (C-009). The happy path logs one page line:
`obs-page: preset <name>`. No `configuration` is passed yet, so the fps cap stays the canvas
default until BS-004.

## Verification

- `node --check browser-source/obs-page.js` → `syntax-ok`.
- No build step and no existing test suite for the page.
- Served with `python3 -m http.server 8080 --directory browser-source` (stopped afterwards). The
  CLI screenshot plus four CDP runs are in *Evidence*.

## Evidence

All of it is in `evidence/BS-003/bs-003-render-report.md`, which quotes every log. The PNGs and
`.txt` files are git-ignored.

- `evidence/BS-003/BS-003-render-1920x1080.png` shows "Eo.S. + Geiss - glowsticks v2 02 (Relief
  Mix)" rendering edge to edge at 1920×1080 (downscaled to 1280×720): a bronze embossed relief
  with glowstick bands, no scrollbars or UI.
- `evidence/BS-003/BS-003-render-1920x1080-later.png`: the same session 3 s later, visibly
  different, so it is animating.
- `evidence/BS-003/BS-003-render-chrome-cli.png`: an independent capture with plain
  `--headless=new --use-gl=angle --use-angle=metal --enable-unsafe-swiftshader --window-size=1920,1080 --virtual-time-budget=8000 --screenshot`.
- `evidence/BS-003/BS-003-missing-dist.png`: all-black frame (every channel 0) with `dist/` absent.
- Page state probe: `compatMode CSS1Compat`, canvas backing `1920x1080`, scroll size equal to the
  viewport, body `rgb(0, 0, 0)`, body children `canvas,script`, no visible text.
- Console:
  - `BS-003-happy-console.txt`: no uncaught exceptions, no network errors,
    `obs-page: preset Eo.S. + Geiss - glowsticks v2 02 (Relief Mix)`.
  - `BS-003-missing-dist-console.txt`:
    `obs-page: could not load ./dist/synesthesia-vibe.js, run browser-source/fetch-dist.sh TypeError: Failed to fetch dynamically imported module: …/dist/synesthesia-vibe.js`.
  - `BS-003-manifest-404-console.txt`:
    `obs-page: could not load ./dist/resources/manifest.json SynesthesiaError: SV_ERR_IO: …manifest.json: HTTP 404`.
  - `BS-003-no-webgl2-console.txt`:
    `obs-page: could not start the renderer SynesthesiaError: SV_ERR_GL: WebGL2 is not available on this canvas`.

## Known limitations / follow-ups

- **Core stderr shows as `console.error` in the happy path.** The core prints two startup lines on
  stderr: `render_gl: audio textures R32F …` and `synesthesia-vibe: GL pipeline ready …`.
  Emscripten's default `printErr` sends them to `console.error`. They are diagnostics, not
  failures. I left them alone, because passing `printErr` to `loadSynesthesiaModule` to downgrade
  them would also downgrade real core errors. This needs a decision: accept it (and say so in the
  README troubleshooting section, BS-007), or reroute in the page. A possible upstream ask
  (`synesthesia-vibe#<n>`, not filed) is for the core to print info-level lines to stdout on web.
- A first-preset compile failure is logged and the page stays black. The try-next loop belongs to
  BS-004, per the plan.
- Not checked in OBS (another agent holds OBS for the plugin spike). OBS's CEF check is BS-008.
- Headless only. The initial CLI run reported `texsize=1920x993` before resizing to 1920×1080
  (headless window chrome). The CDP runs pin the viewport and start at 1920×1080.
- Document-writer: `docs/structure.md` / `docs/architecture.md` should cover the page's stage
  functions, the `obs-page:` console prefix, and the dynamic-import choice for the missing-dist
  message.
- No test added (project default).

## Handoff for downstream review

- Acceptance criteria addressed: animating preset edge to edge on black at a 1920×1080 viewport,
  with no scrollbars or UI; with `dist/` missing the page stays black and the console names the
  file; no page or network errors in the happy path (core stderr caveat above). TC-007, TC-008
  and TC-009 were exercised.
- Areas to scrutinise:
  - `stage()` wrapping and `main().catch` logging `error.cause ?? error`.
  - The header comment before `<!doctype html>`; standards mode is confirmed by the probe.
  - The `data:,` icon link.
  - That `loadPreset` treats a `null` result (superseded) as silent.
- How to run:
  1. `python3 -m http.server 8080 --directory browser-source`, then open `http://localhost:8080/`.
  2. For the automated check: `node <evidence>/BS-003/bs-003-cdp-check.mjs http://localhost:8080/ <out-prefix> ["<extra flags>"]`.
