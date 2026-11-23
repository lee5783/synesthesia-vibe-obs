## Summary

This PR implements Phase 2 of `PLAN.md` on macOS (Apple Silicon): the SynesthesiaVibe
Visualizer source for OBS.
- **Audio:** the source reacts to a chosen OBS audio source or to the whole mix.
- **Presets:** presets come from a user folder and load off the graphics thread. Auto-cycle,
  random order, blend times and four hotkeys are included.
- **Output:** width, height and render scale are configurable.
- **Packaging:** an arm64 `.pkg` and a zip.

It also fixes the spike's thread race, the black frame after a source comes back on air, and
transparent presets.

**Three checks are deferred** because OBS was stuck at a startup prompt:
- the "Done when" run (P2-011)
- the 1080p60 measurement (P2-012)
- the in-OBS check of the dialog fix (P2-010b)

See *Risks and follow-ups*.

**Ticket:** none
**Design:** none

## Changes

**Plugin** (`plugin/src/visualizer-source.cpp` unless noted)
- **P2-001 build guard:** `plugin/CMakeLists.txt` fails configure with one clear message unless
  the host is macOS, the generator is Xcode and the architecture is arm64 alone.
- **P2-002 thread-race fix:** `video_tick` now only writes atomics. `video_render` consumes the
  frame delta (capped at 0.1 s, reset on show/activate) and makes every `sv_*` call under the
  graphics context.
- **P2-003 opaque draw:** the source draws with `OBS_EFFECT_OPAQUE`, so presets that write
  alpha 0 (e.g. swarm) still cover what is under them.
- **P2-004 audio from a chosen source:**
  - new `plugin/src/audio-ring.{hpp,cpp}`: a lock-free single-producer ring;
  - an "Audio source" list, stored by UUID and resolved lazily through a weak reference;
  - the audio capture callback resamples to 44.1 kHz stereo;
  - the ring is drained into `sv_push_pcm_float` in render;
  - after 0.5 s with no audio, render pushes silence, because the core never expires old PCM;
  - the synthetic tone is removed.
- **P2-005 whole mix:** `audio_output_connect` on a chosen mix track (1–6). Only one audio feed
  is attached at a time, and it is disconnected on change and on destroy.
- **P2-006 presets:**
  - a folder picker and a preset list;
  - a per-source `os_task_queue` worker runs `sv_prepare_preset`, drops stale results, and
    `sv_commit_preset` runs in render inside the GL guard;
  - the worker is drained before `sv_destroy`;
  - the synchronous load and `preset_path` are removed.
- **P2-007 auto-cycle:** cycle seconds, blend seconds and random order (never repeats the
  current preset), through `sv_set_transition_config` and the preset-request callback. Settings
  changes apply live.
- **P2-008 hotkeys:** next, previous (50-entry history), random, and pause/resume. Pause defers
  core requests. Hotkeys are unregistered first in `destroy`.
- **P2-009 resolution:** width × height (default 1920×1080, 128–8192) and a 100/75/50 % render
  scale. `sv_resize` runs inside the GL guard.
- **P2-010 settings:** properties in four groups (Presets, Transitions, Audio, Output), defaults
  for all 10 settings, and every UI string in `plugin/data/locale/en-US.ini`.
- **P2-010b dialog fix:** a "(first preset)" list entry whose value is `""`, so opening the
  properties dialog no longer saves a preset.
- **P2-013 packaging:** project-owned `plugin/packaging/macos/distribution.in` declares
  `hostArchitectures="arm64"`, overriding the template without editing it. The release is an
  ad-hoc `.plugin`, an unsigned `.pkg` and a `ditto` zip, with no presets bundled.
- **P2-014 `plugin/README.md`:** install (pkg/zip, unnotarized Gatekeeper steps), settings by
  group, hotkeys, presets, the OBS log, troubleshooting and uninstall.

**Docs and agent knowledge**
- `docs/`: architecture (frame flow, audio path, preset path, threading table), structure,
  integrations (dev install, packaging, notarization placeholders), conventions (locale keys,
  concurrency rules), testing (websocket verification, dialog-safety rule), stack and concerns.
  The concerns page covers what was fixed, the limits, the review nits, the deferred checks, and
  drafts of upstream asks #4 and #6.
- `.claude/how-x-works/`: `plugin-render-frame.md` rewritten; `plugin-audio-path.md` and
  `plugin-preset-loading.md` are new.
- `.claude/common-issues/`: entries for the build guard, the `release`/`Release` folder
  collision, the quoted pkg receipt id, `._` provenance sidecars, `sv_resize` state, the
  properties-dialog wedge and `RemoveInput` in a scene that isn't rendering.
- `PLAN.md` (user-approved scope only): status line, Phase 2 task states, *Done-when state*, and
  upstream ask #5 (host-settable random-texture folder, not filed).
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/`: this run's artifacts.
- `.claude/working-sessions/20261123004446-browser-source-phase-1/session.log`: one line
  recording that PR #1 was opened.

## Verification

- [x] Builds clean: `cd plugin && cmake --preset macos-arm64 && cmake --build --preset macos-arm64`
  gives `** BUILD SUCCEEDED **` under `-Werror`, with 0 warnings from `plugin/src`. A from-scratch
  configure also passed (P2-001), and the Release build passed (P2-013).
- [x] `nm -gU` shows exactly the 7 `obs_module_*` exports after every task. `lipo -info` reports
  arm64 only.
- [ ] Existing tests pass: no test suite exists (`docs/testing.md`).
- Tests added: none, not requested.

**Evidence**: from `develop/evidence/<task>/`. Each folder's `.md` report quotes the key
results. Screenshots are git-ignored; attach them if wanted.

| What it shows | Source |
|---|---|
| Race fixed: the detector saw 3 overlaps in 60 s on the spike build; on the final build, 0 in 22,450 core calls over a 5-min screenshot stress run | `P2-002/P2-002-race-fix.md` |
| An off-air screenshot is no longer black | `P2-002-final-2-hidden-12s.png` |
| Swarm draws opaque: min alpha 0 before, 255 after; 0 magenta pixels show through | `P2-003/P2-003-opaque-draw.md`, `P2-003-after-swarm-nest.png` |
| Audio reaction: levels up on the beat; 0 in silence, muted, None and after removal; switching detaches before attaching | `P2-004/P2-004-audio-source.md` |
| Whole mix: reacts on the fixture's track, reads exactly 0 on others | `P2-005/P2-005-whole-mix.md` |
| Preset switching: 30 switches skipped 16 frames before, 0 after (also 0 with about 400 ms prepares); broken and empty folders handled | `P2-006/P2-006-presets.md` |
| Auto-cycle timing, random order with no repeat, live settings changes, mid-blend screenshots | `P2-007/P2-007-cycle.md` |
| Hotkeys through `TriggerHotkeyByName`; pause defers requests; two instances act independently | `P2-008/P2-008-hotkeys.md` |
| Resize live; probe orientation correct at every scale; other sources bit-identical after 65 resizes | `P2-009/P2-009-resolution.md` |
| All defaults present; strings audit; group layout | `P2-010/P2-010-settings.md` |
| Package inspection: arm64 distribution, payload, ad-hoc signature, `spctl` rejects as expected | `P2-013/P2-013-package-inspection.md` |

Each OBS session in P2-002 to P2-009 ended with 0 GL error lines and `Number of memory leaks: 0`.
The P2-010 session ended in a forced stop (see Risks), so it has no leak count.

**Manual QA**: from `plan/output-plan-test-cases.md`

| Check | Status |
|---|---|
| Renders and other sources stay correct (GL state) | Verified per task. The combined run on the final build is deferred (P2-011) |
| Reacts to a chosen source and to the whole mix | Verified with a Media Source WAV fixture. Real mic and desktop audio need your macOS permissions |
| Preset and transition changes apply without restart | Verified (P2-006, P2-007) |
| Removing the source or quitting leaks nothing | Verified per task up to P2-009. The final-build quit is deferred (P2-011) |
| **1080p60 without stalling OBS** | **Deferred (P2-012)**, in the approved separate `synesthesia-1080p60` profile |
| Dialog no longer saves a preset on open (P2-010b) | **Deferred in-OBS check.** The code was reviewed and the build is clean |
| Install from the `.pkg` / zip into OBS | **Not run.** The package was inspected, not installed |

## Review

- [x] Reviewed by the `code-reviewer` pass. Decision: **Approve**. 0 blockers, 2 should-fix
  (README), 9 nits.
- [x] Must-fix findings addressed: S1 (the dSYM is now in the README's install and uninstall
  text) and S2 (exact `pkgutil --forget` command for the quoted receipt id), plus N4 (README
  blend wording). N1–N3 and N5–N9 are recorded in `docs/concerns.md`.
- **Areas to scrutinize:**
  - the cycle-latch and pause bookkeeping (`take_preset_job`, `clear_cycle_request`,
    `preset_requested`, `toggle_auto_cycle_pause`);
  - `previous_from_history`;
  - the audio attach/detach under `audio_mutex` and the resampler lifetime;
  - the worker teardown order before `sv_destroy`;
  - the `distribution.in` override running after the template's `configure_file`.

## Documentation

- [x] `docs/**` updated: architecture, structure, integrations, conventions, testing, stack,
  concerns and the README listing.
- [x] New gotchas and flows recorded in `COMMON-ISSUES` (plugin-build, plugin-runtime,
  verification-tooling) and `HOW-X-WORKS` (three plugin flows).
- Root `README.md`: not touched, not requested. `plugin/README.md` is new (P2-014).

## Risks and follow-ups

- **Deferred checks**, in the reviewer's priority order:
  1. **P2-012, 1080p60.** A heavy preset's commit takes up to about 18 ms on the graphics thread,
     over a 60 fps frame.
  2. **P2-011, GL-state check on the integrated build.** `sv_commit_preset`, `sv_resize` and
     `sv_set_transition_config` were never in the spike's audit.
  3. **P2-011, final-build stress and combined teardown.**
  4. **P2-011, final-build leak check.**
  5. **Pkg install into OBS.**
  6. **P2-010b dialog check.**
- **OBS was force-stopped once:** a test removed a source while its properties dialog was open,
  and the agent stopped OBS with SIGINT. The crash report has no frames from the plugin. OBS then
  stuck at its safe-mode prompt, which is why the checks above are deferred. Rule added: never
  remove an input or switch collection with its properties dialog open.
- **Shared audio buffer (C-006):** the core's PCM buffer is process-global, so several
  visualizers on different audio inputs react to all of them mixed. Upstream ask #4 is drafted
  in `docs/concerns.md`.
- **Stale audio:** the core never expires old PCM, and the plugin works around it with a 0.5 s
  silence push. Upstream ask #6 is drafted.
- **Random textures:** the core searches only the bundle's `textures/` folder, which is empty in
  a release, so presets that ask for a random texture get none. Upstream ask #5 is in `PLAN.md`.
- **Notarization:** not done (no Developer ID on this Mac). The `.pkg` is unsigned and the
  `.plugin` ad-hoc signed, so Gatekeeper steps are needed. Commands with placeholders are in
  `docs/integrations.md`.
- **Template packaging quirks:** the pkg receipt id contains literal quotes, and the pkg installs
  a `.dSYM`. Both are documented, not fixed.
- **Smaller behaviours:**
  - creating a visualizer skips about 2 frames, because `sv_create` runs on the graphics thread;
  - removing a source can wait up to about 0.4 s for a prepare in flight;
  - a resize resets the image;
  - render scale only lowers GetStats time when the GPU is saturated;
  - the Preset list doesn't follow auto-cycle (a "now playing" line is a follow-up).
- **Not run on this Mac:** real mic and desktop audio (OBS's macOS microphone permission is off),
  a real Metal renderer, and OBS versions other than 33.0.3.
- **Your cleanup:**
  - turn obs-websocket authentication back on with a new password;
  - get OBS past its startup prompt (Run in Normal Mode);
  - delete the `synesthesia-spike` scene collection when done.

  The test plugin was already removed from your plugins folder.

## Checklist

- [x] No secrets, tokens or credential files in the diff. Grepped for the websocket password,
  credential and scratchpad paths, `/Users/` paths, local folder names, signing identity names
  and team IDs. The reviewer ran the same scan.
- [x] No AI-session-only files staged unintentionally. Session artifacts are committed per
  `HARNESS.md`; bulk evidence (PNG, logs, scripts) is git-ignored.
- [x] New files carry the standard header (`audio-ring.*`, `distribution.in` as an XML comment).
  README and JSON are exempt; the verbatim template files are unchanged (`git diff master -- plugin/cmake` is empty).
- [x] No unrequested code comments added. The reviewer confirmed this.
- [x] Style matches the sibling components. The reviewer confirmed this.
- [ ] Shared components reused or extended: N/A. Sibling libobs patterns were followed
  (compressor sidechain, slideshow folder and hotkeys).

---
🤖 Built with an AI vibe-flow run — session `20261123091415-plugin-phase-2-macos`

🤖 Generated with [Claude Code](https://claude.com/claude-code)
