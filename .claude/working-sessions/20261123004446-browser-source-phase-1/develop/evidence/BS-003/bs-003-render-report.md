# BS-003 evidence — render path

Google Chrome 157.0.8103.79 on macOS (Apple M1), headless (`--headless=new`), 1920×1080, ANGLE on Metal.
The page was served with `python3 -m http.server 8080 --directory browser-source`. The PNGs and
`.txt` logs in this folder are git-ignored (only `.md`/`.py` are tracked), so the key lines are
quoted here.

## Screenshots (downscaled to 1280 px wide)

| File | What it shows |
|---|---|
| `BS-003-render-1920x1080.png` | Happy path, 6 s after load (CDP `Page.captureScreenshot`). "Eo.S. + Geiss - glowsticks v2 02 (Relief Mix)" fills the frame edge to edge: an embossed bronze relief with a spiral of glowstick bands in the centre. No scrollbars, no text, no controls. |
| `BS-003-render-1920x1080-later.png` | Same session 3 s later. The bands and relief have moved (a different spiral, larger arms), so the preset is animating, not a still. |
| `BS-003-render-chrome-cli.png` | Independent capture with the plain CLI: `Google Chrome --headless=new --use-gl=angle --use-angle=metal --enable-unsafe-swiftshader --window-size=1920,1080 --virtual-time-budget=8000 --screenshot=… http://localhost:8080/`. Same preset, earlier in its animation. |
| `BS-003-missing-dist.png` | `dist/` absent. The whole frame is black: every RGB channel is 0 across all 1920×1080 pixels (PIL `getextrema()` → `((0,0),(0,0),(0,0))`). The happy-path frame reads `((0,255),(0,255),(0,212))`. |

## Page state (CDP `Runtime.evaluate`, 6 s after load)

Happy path:

```
{"compatMode":"CSS1Compat","viewport":"1920x1080","canvasBacking":"1920x1080","canvasCss":"1920x1080",
 "scroll":"1920x1080","bodyBackground":"rgb(0, 0, 0)","bodyChildren":"canvas,script","visibleText":0}
```

- Standards mode, so the header comment before `<!doctype html>` does not trigger quirks mode.
- The canvas backing store is 1920×1080 at DPR 1, matching the viewport. The scroll size equals the
  viewport, so there is no overflow.
- The body is black and holds only the canvas and the script. It has no visible text.

## Console — happy path (`BS-003-happy-console.txt`)

```
console.info: ResourceLoader wrote 10 presets, 23 textures, 0 audio files (2040516 bytes) in 42 ms
console.log: sv_init_gl OK
console.log: GL_VERSION=OpenGL ES 3.0 (WebGL 2.0 (OpenGL ES 3.0 Chromium))
console.log: AUDIO_TEXTURE_FORMAT=R32F
console.error: render_gl: audio textures R32F (GL_OES_texture_float_linear present) GL_RENDERER "WebKit WebGL"
console.error: synesthesia-vibe: GL pipeline ready texsize=1920x1080 grid=64x48
console.info: obs-page: preset Eo.S. + Geiss - glowsticks v2 02 (Relief Mix)
```

- No uncaught exceptions and no network errors. The first run also showed a `favicon.ico` 404; it
  was fixed in `index.html` with `<link rel="icon" href="data:,">`, and the re-run above is clean.
- The two `console.error` lines are **not failures**. They are the core's startup diagnostics,
  which the wasm writes to stderr. Emscripten's glue sends stderr to `console.error` by default
  (`err = Module["printErr"]`). The page does not reroute them, because the same channel carries
  real core errors. See *Known limitations* in `output-BS-003.md`.

## Console — missing `dist/` (`BS-003-missing-dist-console.txt`)

The page alone (`index.html` + `obs-page.js`) was served from a scratch copy on port 8081, so the
real `browser-source/dist/` stayed untouched.

```
log.error (network): Failed to load resource: the server responded with a status of 404 (File not found) http://localhost:8081/dist/synesthesia-vibe.js
console.error: obs-page: could not load ./dist/synesthesia-vibe.js, run browser-source/fetch-dist.sh TypeError: Failed to fetch dynamically imported module: http://localhost:8081/dist/synesthesia-vibe.js
```

One prefixed line names the missing file and the fix. Nothing is uncaught, and the page stays
black (TC-008).

## Console — manifest 404 (`BS-003-manifest-404-console.txt`)

This used the same scratch copy, with `dist/*` files symlinked in but no `dist/resources/`.

```
console.error: obs-page: could not load ./dist/resources/manifest.json SynesthesiaError: SV_ERR_IO: http://localhost:8081/dist/resources/manifest.json: HTTP 404
```

## Console — WebGL2 unavailable (`BS-003-no-webgl2-console.txt`)

The real page was loaded with extra flags `--disable-gpu --disable-software-rasterizer --disable-webgl2`.

```
console.error: obs-page: could not start the renderer SynesthesiaError: SV_ERR_GL: WebGL2 is not available on this canvas
    at createWebGl2Context (…/dist/webgl-context.js:30:15)
    at SynesthesiaCanvas.create (…/dist/synesthesia-canvas.js:78:24)
```

The page stays black, with no uncaught exception (TC-009).

## Reproduce

```
python3 -m http.server 8080 --directory browser-source
node .claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/BS-003/bs-003-cdp-check.mjs http://localhost:8080/ <out-prefix> ["<extra chrome flags>"]
```

`bs-003-cdp-check.mjs` (Node 24, no dependencies, git-ignored by the evidence rule) launches
headless Chrome with a throwaway profile. It records `Runtime.consoleAPICalled`,
`Runtime.exceptionThrown` and `Log.entryAdded`, probes the page state, and writes
`<prefix>-t6s.png`, `<prefix>-t9s.png` and `<prefix>-console.txt`.
