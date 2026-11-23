# Requirements

## Request (verbatim)

> Start phase 2

Earlier context: the previous run (`20261123004446-browser-source-phase-1`) delivered Phase 1 and
the Phase 2 spike. It was merged as PR #1.

## Scope

`PLAN.md` → **Phase 2 — Native plugin, macOS**, *Tasks after the spike*, items 1–7:

1. CMake project. Started in the spike; the remaining work is per the spike results.
2. Source lifecycle: create/destroy inside the graphics context, plus `video_tick`,
   `video_render`, `get_width` and `get_height`.
3. Audio: pick any OBS audio source or the whole mix. The audio thread writes into a lock-free
   ring buffer, which is drained on the graphics side. Remove callbacks on destroy and when the
   source changes.
4. Presets: a folder picker, the current preset, random order, auto-cycle and transition times,
   and off-thread prepare with commit on the graphics side.
5. Hotkeys: next, previous, random, and pause auto-cycle.
6. Resolution: width × height (default 1920×1080) through `sv_resize`, and an optional render
   scale.
7. Packaging: a signed and notarized `.plugin` bundle, with install steps.

The Phase 2 follow-ups recorded in `PLAN.md` *Spike results* and `docs/concerns.md` are also in
scope where they touch these tasks:

- the `video_tick` / `video_render` thread race;
- opaque draw for alpha-0 presets;
- clamping `dt` after the source was hidden or inactive;
- 1080p60 measurement;
- the export-list catch;
- replacing the spike stand-ins (synthetic tone, `preset_path` with no UI);
- how presets ship.

**Done when** (from `PLAN.md`, macOS, Apple Silicon, OBS 31+):

- The source renders at 1080p60 without stalling OBS's renderer.
- It reacts to a chosen OBS audio source and to the whole mix.
- Changing preset or transition settings takes effect without restarting OBS.
- Removing the source, or quitting OBS, leaks nothing and crashes nothing.
- Other sources in the same scene still draw correctly (the GL-state check).

## Out of scope

Phase 3 (Windows), Phase 4 (CI, release, Linux), the root `README.md`, and editing the core
submodule.

## Standing constraints from the previous run

- The agent drives OBS only through obs-websocket and never uses GUI or accessibility automation.
  If the websocket is unavailable, the agent stops and asks the user.
- OBS config files are never edited, and the user's profile, canvas and fps are never changed
  without asking.
- Testing happens in a separate scene collection.
- No secrets, credential paths or machine paths go into repo files.
- Phase 1 is still open, pending the user's loopback checklist. That is not this run's work.
