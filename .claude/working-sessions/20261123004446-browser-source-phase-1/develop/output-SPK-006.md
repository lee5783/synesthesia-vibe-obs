# SPK-006

## Subtasks executed
- Read the libobs 31.1.1 GL device to see what it caches and when it skips a GL call:
  - `libobs-opengl/gl-subsystem.c`/`.h`, `gl-helpers.c`/`.h`, `gl-vertexbuffer.c`,
    `gl-texture2d.c`/`gl-texture3d.c`, `gl-stagesurf.c`;
  - `libobs/graphics/graphics.c` (the blend-state cache) and `texture-render.c`.
- Read the core's GL call sites under `external/synesthesia-vibe/src/render/gl/`, read only. In
  particular `PresetShaderInputs::Apply`/`Restore`, `blend_state.cpp` and `framebuffer.cpp`.
- Temporary instrumentation, spike only and now removed:
  - Snapshotted 250 GL items before and after every core call: `sv_init_gl` + `sv_create`,
    `sv_load_preset`, `sv_render_frame`, `sv_destroy`.
  - Logged a raw diff (what the core changes) and a guarded diff (what OBS sees afterwards).
  - The snapshot drained `glGetError` before and after itself, so it never left an error for
    OBS's `gl_success` checks.
- Run 1, with SPK-005's program-only guard: recorded the raw diff.
- Wrote `gl_state_guard` from the audit and replaced `gl_program_guard`.
- Run 2, instrumented with the full guard, about 4 minutes:
  - 14 program-scene switches;
  - 4 visualizer hide/show cycles;
  - text, image and colour toggles;
  - a second visualizer instance created, scaled into view, then removed;
  - screenshots.
  - The guarded diff was checked on every core call.
- Run 3, final build with the instrumentation removed: the same script, plus `GetStats`
  sampling, two instances, one instance, and a baseline with the visualizer hidden.
- Run 4, instrumented: switched presets (glowsticks v2 02 with 3D noise textures, Jester's
  Calling 2, swarm, then back to Fractopia) and timed an empty guard save+restore.
- Rebuilt and reinstalled the final build. Reset the visualizer's `preset_path` to empty and
  restored the `Untitled` scene collection.
- Wrote the "GL state" answer, the Risks row and the upstream ask #2 draft (not filed) into
  `PLAN.md`.

## Files changed
- `plugin/src/visualizer-source.cpp`:
  - `gl_program_guard` is replaced by `gl_state_guard`, plus a small `with_core_gl_state`
    helper.
  - Each core call is wrapped on its own. The preset load no longer sits inside a guard that
    also spanned `gs_texrender_begin`/`end`.
  - The debug callback setup moved out of the guarded block. It makes no GL calls.
- `PLAN.md`:
  - *GL state* answer, with the state table, decision, proof and cost.
  - The *Rendering into OBS* note now points to it.
  - Risks row for GL state.
  - Upstream ask #2 draft.
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md`: SPK-006 section appended.
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-SPK-006.md`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-006/`: see *Evidence*.
- Outside the repo:
  - The installed `synesthesia-vibe-obs.plugin` holds the final build, re-signed ad hoc, with
    presets and textures copied (reuses `scratchpad/spk005/reload.sh`).
  - In the `synesthesia-spike` collection, the second instance `SPK-006 Visualizer 2` was
    created and removed.
  - The visualizer's `preset_path` was set during run 4 and reset to empty.
- Not touched: `external/synesthesia-vibe/` (clean), OBS config files, the OBS profile, canvas
  and fps settings, `docs/`.

## What changed and why

The audit answers the question with **save/restore in OBS's own context**. Beyond the program
bug found in SPK-005, the core leaves several more pieces of state that OBS's GL device caches:

- **`sv_create`** runs on the UI thread. It leaves the **draw framebuffer at 0** and the
  **viewport at 1920×1080**, while `gs_device.cur_fbo` and `cur_viewport` still hold OBS's values.
  OBS skips `glBindFramebuffer` when its cache matches.
- **`sv_render_frame`** leaves the following changed:
  - **blending disabled, with preset-dependent factors**. `graphics->cur_blend_state` still says
    enabled, so `gs_reset_blend_state` skips re-enabling it. SPK-005 only escaped this because
    the plugin's own `gs_blend_state_pop` re-issues the blend state.
  - **the active unit at `GL_TEXTURE0`**.
  - **its own texture on unit 0**, and it zeroes the 2D/3D bindings on the other units it used.
    `cur_textures[8]` then skips rebinding.
  - its program, VAO, `GL_ARRAY_BUFFER` and read framebuffer.
- `sv_load_preset` and `sv_destroy` change nothing.

`gl_state_guard` saves and restores:

- program
- VAO
- `GL_ARRAY_BUFFER`
- draw and read framebuffers
- viewport
- blend enable, factors and equation
- active texture unit
- the 2D and 3D binding on units 0–7 (`GS_MAX_TEXTURES`)

That is every item the core changes that OBS caches or relies on, plus two cheap uncached ones
(`GL_ARRAY_BUFFER`, read framebuffer), so the diff comes out empty. Left out on purpose:

- **Samplers and pixel store**: the core resets them to OBS's assumed defaults itself.
- **Scissor, depth, cull, stencil and colour mask**: the core only sets values OBS already has.
- **Units 8 and up**: OBS never uses them.
- **Clear colour**: OBS sets it before every clear.

The full table is in `evidence/SPK-006/SPK-006-state-diff.md` and in `PLAN.md`.

The VAO restore also protects the core. OBS's index-buffer creation binds into whatever VAO is
current, so without it OBS could overwrite the core's VAO element binding.

## Verification
- Build: `cd plugin && VCPKG_ROOT=$HOME/vcpkg cmake --build --preset macos-arm64` gave
  `** BUILD SUCCEEDED **` for the instrumented builds and twice for the final build. The
  template uses `-Werror`, and there were no warnings in `plugin/src`.
- `grep -c "SPK006\|spk006\|snapshot" plugin/src/visualizer-source.cpp` returns 0: the
  instrumentation is gone. The repo file is byte-identical to the build that was installed and
  tested last.
- Run 2 (instrumented, full guard):
  - `SPK006 guarded checks 7200, with any change 1`. The one change is `COLOR_CLEAR_VALUE` after
    the second instance's `sv_create`.
  - Every `sv_render_frame` guarded diff read 0 of 250.
  - 0 GL error lines; `plugin unloaded`; `Number of memory leaks: 0`.
- Run 3 (final build):
  - 0 GL error lines (`grep -ciE 'glUniform|\(GL\) failed|GL_INVALID|glActiveTexture failed'`).
  - Every 10 s debug line read `gl_errors 0, last_error ""` (31 lines).
  - No memory leaks on quit.
- Run 4: every preset load and every logged frame read `guarded: 0 of 250 items changed`, with
  0 GL error lines.
- Guard cost: `empty guard save+restore: 19.6–20.3 us average over 300–1200 calls`.
- GetStats (render time; the user's profile is 1080×1920 at 30 fps and was not changed):

  | Build | Visualizer on | Baseline (hidden) |
  |---|---|---|
  | Final build | one instance 6.9–10.3 ms; two instances 7.6–10.9 ms | 2.8 ms |
  | SPK-005 | 5.9–9.4 ms | 2.0–2.1 ms |

  The per-frame delta is the same within GetStats noise, and the guard itself accounts for
  about 0.02 ms of it.
- There is no existing test suite for this area.

## Evidence
All in `evidence/SPK-006/`:
- `SPK-006-state-diff.md` holds the report:
  - the raw diff per core call;
  - the table of what the core changes against what OBS caches, with `libobs` source
    references;
  - the guarded-diff results, the decision and the cost.
- `SPK-006-obs-log-instrumented.txt`: `SPK006` log lines from run 1 (program-only guard: the
  guarded diff still shows the leaks) and run 2 (full guard: empty guarded diffs, the
  7200-check counter, 0 GL errors). Paths are scrubbed.
- `SPK-006-obs-log-final.txt`: the plugin lines from the final-build run, the GL error count,
  the preset-run `SPK006` lines with guard timing, and the shutdown/leak lines from all three
  logs. Paths are scrubbed.
- `SPK-006-stats.txt`: GetStats for the final and instrumented builds, the guard micro-timing,
  and the SPK-005 reference. It is a text file, not `SPK-006-stats.png`: GetStats is a
  websocket text response, and GUI capture of the Stats dock is not allowed.
- Screenshots, all `GetSourceScreenshot` (720 px wide, portrait canvas, transparent areas
  white). Every image was opened and checked before writing this:
  - `SPK-006-obs-scene.png`: final build. Visualizer (Fractopia), image, colour and text all
    draw correctly.
  - `SPK-006-two-instances.png`: final build. Two visualizer instances render independently
    alongside the other sources.
  - `SPK-006-visualizer-hidden.png`: final build, visualizer item hidden. The other sources
    are unaffected.
  - `SPK-006-after-remove.png`: final build after the second instance was removed
    (`sv_destroy` under the guard). The scene is intact.
  - `SPK-006-text-source.png`: the text source on its own, alpha-blended edges intact (blend
    state not leaked).
  - `SPK-006-preset-glowsticks.png`: instrumented build, glowsticks v2 02 (3D noise samplers)
    rendering with the other sources.
  - `SPK-006-preset-swarm-alpha.png`: instrumented build, the swarm preset. The visualizer area
    reads RGB content with **alpha 0**, so it shows transparent in the PNG. This is a
    colour/alpha finding, not GL state; see the follow-ups.

## Known limitations / follow-ups
- **Alpha (new finding, not in SPK-006 scope):**
  - Some presets (swarm) write alpha 0. The plugin draws its texture with blending off, so the
    zero alpha reaches the canvas. Screenshots, and any downstream blend such as a filter or a
    nested scene, see the visualizer as transparent. A plain RGB stream output would still show
    it.
  - Phase 2 should draw with an opaque technique (e.g. `OBS_EFFECT_OPAQUE`) or force alpha to 1.
- The guard covers units 0–7 because that is OBS's `GS_MAX_TEXTURES`. A future libobs that
  raises the limit would need the constant revisited. It uses `GS_MAX_TEXTURES` directly, so it
  follows a recompile.
- The audit holds for core `v0.7.0`. A core bump should re-run it. The instrumented source is
  kept outside the repo in the session scratchpad (`spk006/visualizer-source-instrumented.cpp`).
  The scratchpad can be cleared, so if it is wanted durably it should be turned into a
  debug-only option. Upstream ask #2 would remove the need.
- The viewport is unchanged in `sv_render_frame` today only because the texrender and core are
  both 1920×1080. A configurable resolution would make the guard's viewport restore matter there
  too; it is already covered.
- Reverse direction: the core sets every piece of state it needs on each call. It binds its
  program unconditionally and queries `GL_CURRENT_PROGRAM` only once, in `blend_state.cpp`. Four
  presets rendered correctly with OBS state changing in between. This is not a documented core
  guarantee, so it is in the upstream ask.
- 1080p60 was still not measured; the user's profile stays at 30 fps (see SPK-005). The guard
  adds about 20 µs a frame.
- S-012: the diff logging is removed; the guard is kept. The debug-report line every 10 s from
  SPK-005 is still `LOG_INFO` and is spike instrumentation for SPK-009 to settle.
- Doc gaps for document-writer:
  - `docs/architecture.md`: the GL state contract. Every core call goes through
    `with_core_gl_state`, and the guard list must be kept in step with the core.
  - `docs/concerns.md`: the OBS caches (program, FBO, viewport, blend, textures per unit), the
    UI-thread `sv_create` hazard, and re-auditing on core bumps.
  - `.claude/common-issues/`: "Other sources lose alpha blending or sample the wrong texture
    after the visualizer = GL state leak; check `gl_state_guard` covers it".

## Handoff for downstream review
- Acceptance criteria addressed:
  - The SPK-005 scene draws every source correctly with the chosen approach, including a second
    instance.
  - The before/after diff of OBS-tracked state is empty (7200+ checks).
  - Render time matches SPK-005; 60 fps was not measured (profile unchanged).
  - The `PLAN.md` answer, table and decision are written.
  - The upstream ask #2 draft is in `PLAN.md`, not filed.
- Areas to scrutinize:
  - The restore order in `~gl_state_guard`: texture units first, then the active unit, then the
    rest.
  - That restoring the draw framebuffer inside the texrender (`sv_render_frame`) puts back the
    texrender FBO that OBS's `cur_fbo` names.
  - `with_core_gl_state` copies the lambda by value. Every lambda captures by reference.
  - `sv_load_preset` now runs under its own guard, outside `gs_texrender_begin`.
- How to run: as in SPK-005 (build, quit OBS, `ditto` the bundle, copy the presets and textures,
  `codesign --force --sign -`, relaunch).
