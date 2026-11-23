# Requirements

## Request (verbatim)

> /vibe-flow work on this @PLAN.md

## Scope for this run

The orchestrator scoped the run to **Phase 1: Browser Source** in `PLAN.md`, because:

- Phase 2 (native plugin, macOS) needs decision D1 (license) confirmed first.
- Phase 3 (Windows) is blocked upstream.
- Phase 4 comes after the plugin.

The user confirms this scope at Gate 1.

Phase 1 as `PLAN.md` states it:

1. `browser-source/fetch-dist.sh <version>` downloads `synesthesia-vibe-web-<ver>.zip` from the
   pinned core release (`lee5783/synesthesia-vibe`, currently `v0.7.0`), checks it against
   `SHA256SUMS.txt`, and unpacks it into `browser-source/dist/` (git-ignored).
2. `index.html` + `obs-page.js`: a full-window canvas, no on-screen UI, black body background.
3. URL settings: `preset` (one or comma-separated list), `cycle` (auto-cycle seconds, `0` = off),
   `blend` (transition seconds), `audio` (input device label or `default`), `fps` (frame-rate cap).
4. Audio from the device chosen with `audio=` (a loopback device or a microphone). OBS has to be
   started with `--enable-media-stream`. Serve over HTTPS or localhost.
5. A GitHub Pages workflow publishes `browser-source/` with the dist.
6. `browser-source/README.md`: OBS setup steps (source URL, size, launch flag, loopback routing).

**Done when:** a 1920×1080 Browser Source in OBS shows the visualizer reacting to music played
through a loopback device on macOS and Windows, and cycles presets from the URL settings.

## Facts checked at bootstrap

- Core release `v0.7.0` exists and has `synesthesia-vibe-web-0.7.0.zip` and `SHA256SUMS.txt`.
- `browser-source/dist/` is already listed in `.gitignore`.
- The repo has no source code yet. `.codegraph/` exists, but the index is empty.

## Out of scope for this run

Phases 2–4, every plugin task, and any change to `README.md` at the root.
