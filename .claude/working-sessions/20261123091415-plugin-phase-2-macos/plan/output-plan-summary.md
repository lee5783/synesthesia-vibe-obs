# Implementation plan summary

## Goal

Turn the Phase 2 spike in `plugin/` into a usable macOS (Apple Silicon) OBS source, "SynesthesiaVibe
Visualizer". It renders the core inside OBS's OpenGL context without breaking other sources, and
reacts to a chosen OBS audio source or to the whole mix. It loads presets off the render thread
from a user-chosen folder, with auto-cycle, transitions, hotkeys and a configurable resolution. It
ships as a locally built package with install steps. The spike's threading race, transparent
alpha-0 presets and off-air time jump are fixed first, because every later feature builds on that
render path.

## Scope

**In scope**
- `PLAN.md` Phase 2, *Tasks after the spike*, items 1–7:
  - build guards (P2-001);
  - lifecycle and threading (P2-002);
  - audio from a chosen source and the whole mix (P2-004, P2-005);
  - presets, auto-cycle and transitions (P2-006, P2-007);
  - hotkeys (P2-008);
  - resolution and render scale (P2-009);
  - packaging and install steps (P2-013, P2-014).
- Spike follow-ups:
  - the `video_tick`/`video_render` race and the `dt` reset/cap (P2-002);
  - opaque draw (P2-003);
  - 1080p60 measurement (P2-012, needs approval);
  - the export-list catch (P2-010);
  - removing the stand-ins (P2-004, P2-006);
  - how presets ship (C-003).
- Verification against *Done when* (P2-011, P2-012), review (P2-015), doc pass (P2-016).

**Out of scope**
- Phase 3 (Windows) and Phase 4 (CI, release, Linux, version script).
- The root `README.md`, and any edit to the core submodule (core asks become upstream drafts).
- Unit tests (C-023 optional), an in-app renderer notice (C-015 default) and a universal binary
  (C-016 default).
- Notarization until Developer ID credentials exist (C-002).

## Key decisions

- **Threading contract:**
  - Every GL `sv_*` call runs in `video_render` (or in create/destroy under `obs_enter_graphics`),
    always inside `with_core_gl_state`.
  - The tick, UI, audio and hotkey threads only write atomics, a guarded slot or the SPSC ring.
  - `sv_prepare_preset` runs on a per-source worker that is drained before `sv_destroy`.
- **Audio:**
  - Convert to 44.1 kHz interleaved stereo on the audio side. The core assumes 44.1 kHz. libobs
    converts the mix path through `audio_output_connect`'s `audio_convert_info`, and
    `audio_resampler_*` handles a single source.
  - Push only the newest ≤ 2304 frames per render; the core keeps no more than that.
- **Audio source selection** follows libobs's compressor-sidechain pattern: a weak reference with
  lazy resolution in `video_tick`, stored by UUID (C-005).
- **Presets** use the "next preset" request callback (`sv_set_preset_request_callback`).
  `sv_set_transition_config` drives the timer, and the commit happens in `video_render`.
- **No presets or textures in the package** (licensing). The user picks a folder (C-003, C-004).
- **Packaging** uses the template's own `pkgbuild`/`productbuild` install step. It installs into
  the user domain, which is the only plugin path OBS scans on Apple Silicon. The plugin is ad-hoc
  signed and the pkg unsigned until Developer ID credentials exist (C-002).
- **Opaque draw** with `OBS_EFFECT_OPAQUE`. The delta resets on show/activate and is capped at
  0.1 s (C-011).

## Risks

- **Process-global audio buffer in the core** (`SetAudioBuf`): two instances with different
  inputs share analysis. Not fixable here; it is documented and drafted as upstream ask #4 (C-006).
- **Verifying the race fix:** TSan can't be injected into release OBS (hardened runtime). The
  plan relies on a temporary re-entrancy detector plus a screenshot stress loop (C-009).
- **Notarization blocked:** only Apple Development and Distribution identities exist. Gatekeeper
  warnings are expected for downloaded builds (C-002).
- **1080p60 unmeasured** until the user approves a dedicated profile (C-001). If it fails, render
  scale is the mitigation.
- **The template pkg declares `arm64,x86_64`** while the bundle is arm64 only (C-016).
- **Preset licensing:** the release can't bundle the BeatDrop presets. First-run UX depends on the
  user supplying a folder (C-003, C-004).
- **GL state:** new core calls (`sv_commit_preset`, `sv_resize`, `sv_set_transition_config`) must
  sit inside the guard. `sv_commit_preset` and `sv_resize` were not part of the spike's GL audit,
  so P2-011's GL-state check covers them.

## Impact

- **Code:**
  - `plugin/src/visualizer-source.cpp` / `.hpp` (most changes);
  - possible new `plugin/src/audio-ring.*` (and similar small files);
  - `plugin/CMakeLists.txt` (guards, new sources, packaging hook);
  - `plugin/data/locale/en-US.ini`;
  - `plugin/exported-symbols.txt` (recheck only).
- **New user doc:** `plugin/README.md`.
- **Docs:** `docs/architecture.md`, `docs/structure.md`, `docs/integrations.md`,
  `docs/concerns.md`, `docs/testing.md`, `.claude/how-x-works/plugin-render-frame.md`, common
  issues, and `PLAN.md` (with approval).
- **UX:**
  - the OBS properties panel for the source (Audio, Presets and Output groups);
  - four source hotkeys in OBS Settings → Hotkeys;
  - a `.pkg` / `.zip` install.

## References

- Task breakdown: [output-plan-tasks.md](output-plan-tasks.md)
- Test scenarios (high level): [output-plan-test-cases.md](output-plan-test-cases.md)
- Open questions and edge cases: [output-plan-concerns.md](output-plan-concerns.md)
- Inputs: `../inputs/requirements.md`. No analyzer artifacts this run.
