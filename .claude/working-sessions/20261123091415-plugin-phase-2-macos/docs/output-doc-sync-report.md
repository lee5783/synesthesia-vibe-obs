# Documentation sync report

## Summary

The doc corpus now describes the shipped, reviewed Phase 2 plugin (P2-001 … P2-010b, P2-013,
P2-014) instead of the spike: the build guard; the tick-atomics / render-consumes threading model,
with `update` deferred to the graphics thread for video sources; the audio path (source capture or
whole mix → resampler → SPSC ring → `sv_push_pcm_float`, plus the 0.5 s silence push); the preset
worker's prepare/commit, generations, the cycle-latch mirror and pause deferral; hotkeys;
resize and render scale; the opaque draw; teardown order; packaging and notarization.

Wherever a page would otherwise claim verification, it now says the user deferred P2-011,
P2-012 and the P2-010b OBS check: `docs/concerns.md` *Deferred Checks* (with the reviewer's
priority ranking), `docs/testing.md` *Planned, not yet run*, the `PLAN.md` *Done-when state*, and
the wedge entry in `.claude/common-issues/verification-tooling.md` ("pending OBS confirmation").

Target: `docs/` (default), plus `.claude/how-x-works/`, `.claude/common-issues/` and their indexes.
The packet authorised two `PLAN.md` edits, reported as user-approved in
`plan/output-plan-gate-1-decisions.md`, *Post-review decisions* ("Phase 2 status and task states
only, plus upstream ask #5"). Only those were made. The root `README.md`, `plugin/README.md` and
all source code were not touched. CodeGraph was configured and reachable.

## Files updated

- `docs/architecture.md`: rewrote the plugin sections. Covers source creation; the frame flow
  (apply transition config → commit → resize → texrender at the scaled size → audio push →
  `sv_render_frame` → opaque sprite at the source size); the audio path, preset path, auto-cycle,
  hotkeys, resize/render scale and teardown; a new *Threading* section (thread table, deferred
  `update`, lock rules); and the new abstractions (`audio_ring`, the preset worker and ready slot,
  properties). Line citations are refreshed.
- `docs/structure.md`: added `plugin/src/audio-ring.*`, `plugin/packaging/macos/` (purpose and
  placement rule), `plugin/README.md` as the user page, `build_macos_arm64/package-staging/`,
  `Release/` and the stub `plugin/build_macos/`. Updated the "where to add" guidance for settings,
  helper files and packaging overrides.
- `docs/integrations.md`: replaced the spike-era install that copied presets into the bundle.
  - New dev install: `ditto` plus verify, with why `cmake --install` must always get `--prefix`.
  - New dev-only preset folder outside the repo, with the textures beside the presets.
  - Build-guard behaviour and its message.
  - New *Packaging* section: commands with `--prefix`, `ditto --norsrc --noextattr`, the
    `grep hostArchitectures build_macos_arm64/distribution` check (N9), payload, quoted receipt id,
    inspection commands and expected output.
  - New *Signing and notarization* section with placeholders only.
  - Updated the log lines, the libobs APIs, storage (no bundled presets) and the env vars
    (`CODESIGN_IDENT` / `CODESIGN_TEAM`). Links `plugin/README.md` for user install.
- `docs/conventions.md`: refreshed the constant-naming examples and import order. New
  *Strings and properties* section: everything through `obs_module_text`; the `Group.*`,
  `Unit.*`, `MixTrack.Track<n>`, `<Label>.<Item>` and `Hotkey.*` key conventions; stable setting
  keys and hotkey names; group order; defaults; the `""` list item. New *Concurrency* section.
  Updated the logging and error-handling patterns.
- `docs/testing.md`: rewrote *How the plugin is verified*:
  - build and `sv_*` reachability checks;
  - OBS rules: never remove an input or switch collection while its properties dialog is open,
    and run removal tests in the program scene;
  - the websocket requests used, pixel and log-timing measurement, and `GetStats` on OpenGL;
  - scratch-only instrumentation (the re-entrancy detector and the audio-levels patch, applied to
    copies and absent from the reviewed source);
  - the deferred P2-011, P2-012 and P2-010b procedures.

  Also covers the fixtures (the Media Source WAV and the preset scratch folders) and the plugin
  user checklist. Removed the synthetic-tone stand-in.
- `docs/stack.md`: no version changed. Updated the plugin's status, languages (`audio-ring.cpp`,
  `distribution.in`), libobs API surface, packaging tools, the build-guard note and the
  production install line.
- `docs/concerns.md`: rewritten.
  - New *Fixed in Phase 2* list: the `video_tick` race, the black off-air frame, alpha 0,
    unenforced macOS-only, the spike stand-ins.
  - Tech debt: template packaging defects (quoted receipt id, dSYM, names), the `distribution.in`
    configure-order dependency (N9), file size.
  - *Known Limitations*: C-006 shared PCM; stale PCM and the silence push, plus N7; random
    textures; the list not following auto-cycle; the folder not watched and N5; N4; transition
    timing; dialog rewrites on open (N2, N3); resize resetting the image; texsize fixed at create;
    mix routing; C-024; the N8 step cap; N6; the N1 teardown window.
  - Performance: commit cost up to 17.8 ms against the 16.7 ms frame at 60 fps; frames skipped on
    create (`sv_create` on the graphics thread); `destroy` waiting up to about 0.4 s; `GetStats` on
    OpenGL tracking CPU time; 8192² too heavy.
  - Fragile areas: `sv_resize` added to the unguarded-state note; new cycle-latch entry.
  - New *Deferred Checks* section with the reviewer's ranking.
  - New *Upstream Ask Drafts*: #4 (per-context PCM buffer plus a sample-rate parameter) and #6
    (public reset or expiry of stale audio).
- `docs/README.md`: refreshed the page descriptions and the plugin status, and listed
  `plugin/README.md` as a user page.
- `docs/decisions.md`: unchanged (no decision locked beyond the C-decisions).
- `.claude/how-x-works/plugin-render-frame.md`: rewritten for the current frame flow with all line
  citations refreshed (setup, tick, render, render_core_frame steps, guard, draw, teardown).
- `.claude/how-x-works/plugin-audio-path.md` (new): choosing a producer, lazy resolve, both
  producers, ring ordering, consumer and silence push, teardown, edge cases.
- `.claude/how-x-works/plugin-preset-loading.md` (new): the three triggers (setting, auto-cycle
  request, hotkey), worker job selection and coalescing, file resolution, ready slot, latch
  bookkeeping, commit, teardown.
- `.claude/common-issues/plugin-build.md`: new entries for the build-guard message, the
  `release`/`Release` case-insensitive collision, the quoted pkg receipt id and the `._`
  provenance sidecars in `--payload-files`.
- `.claude/common-issues/plugin-runtime.md`: new entry, "`sv_resize` writes the same unguarded
  state as `sv_render_frame`".
- `.claude/common-issues/verification-tooling.md`: updated the wedge entry.
  - A folder-only source no longer dirties the dialog after P2-010b. This is marked as pending OBS
    confirmation and still treated as unsafe until then.
  - Triggers that still dirty it: a deleted or renamed saved preset (N3), an off-step render
    scale (N2), an out-of-range mix track.
  - New "How to verify" steps.

  New entry: `RemoveInput` in a non-rendering scene leaves the source alive.
- `PLAN.md` (user-approved scope only):
  1. Status line: Phase 2 built and reviewed, tasks 1–6 done, task 7 partly done, Phase 2 not
     done.
  2. Task states: **Done.** on tasks 1–6. Task 7 marked **Partly done** (ad-hoc pkg and zip,
     notarization waits on a Developer ID, pkg install unverified).
  3. A *Done-when state* paragraph listing P2-012, P2-011 and the P2-010b check as deferred.
  4. Upstream ask #5 (host-settable random-texture folder), drafted and not filed.

  No Risks edits.

## Index updates

- `.claude/HOW-X-WORKS.md`: refreshed the render-frame row. Added rows for
  `plugin-audio-path.md` and `plugin-preset-loading.md`.
- `.claude/COMMON-ISSUES.md`: widened the descriptions of the plugin-build, plugin-runtime and
  verification-tooling rows.
- `docs/README.md`: page descriptions refreshed. No page was added or removed.
- Root `CLAUDE.md` routing table: N/A, because no `docs/` page was added (see Follow-ups for a
  stale sentence there).

## Follow-ups / gaps

- **`PLAN.md` pointer, flag for the orchestrator:** the ask #5 item ends with one sentence
  pointing to asks #4 and #6 in `docs/concerns.md`. Without it the numbering jumps from #3 to #5.
  The asks themselves are not in `PLAN.md`. If even that pointer is outside the approved scope,
  delete the last two lines of item 5.
- **Root `CLAUDE.md` is stale.** It says "The `docs/` pages do not exist yet", "Nothing builds yet"
  and "No test suite yet. Plugin evidence is …". `CLAUDE.md` is outside this role's allowed
  writes, so the user or orchestrator should update *Setup*, *Quick facts* and the routing-table
  note.
- **The P2-010b wedge wording** in `verification-tooling.md` must lose "pending OBS confirmation"
  once the deferred OBS check passes. The same applies to `docs/concerns.md` *Deferred Checks*
  and the `PLAN.md` *Done-when state* after P2-011 and P2-012 run.
- **Unverified claims are carried forward and marked:** stapling a `.plugin` bundle;
  `pkgutil --pkgs` / `--forget` against a real receipt, including whether `--volume ~` is needed;
  "OBS 31 or later" rests on the build target, and only 33.0.3 was checked.
- **Some PLAN.md risk rows describe the spike** (the race, alpha 0, the synchronous load) and are
  now out of date. They were left alone because Risks edits were excluded.
- **`.claude/DICTIONARY.md`:** no entries added. Candidate terms, if wanted: "cycle latch",
  "ready slot", "render scale".
- **Line length:** a few new lines run 101–116 characters (long command lines and code spans),
  similar to the existing corpus.
