# Review findings — Gate 2 (BS-009 + SPK-009)

Scope: BS-001…BS-005, BS-007, BS-008, LIC-001, SPK-001…SPK-008. The change set is uncommitted on
`master`.

## Decision

**Request changes.** Two items block a commit, and both are small. Neither blocks the design. The
Phase 1 page and the spike plugin are correct and well evidenced. Neither needs rework.

## Tooling

- There is no linter or formatter (CLAUDE.md), so the review compared the code with the
  structurally closest files: the dist JS for `obs-page.js`, and the template's C style (tabs) for
  `plugin/src`.
- **Template copies.** I downloaded `obsproject/obs-plugintemplate@3e7d7ac3` into the scratchpad
  and ran `cmp` on all 27 files: 25 under `plugin/cmake/**`, plus `src/plugin-support.c.in` and
  `src/plugin-support.h`. **All 27 are byte-identical**, and the file sets match. The authored
  diffs against the template are expected:
  - `CMakeLists.txt`: header, build-number guard, ExternalProject, imported target, export list.
  - `CMakePresets.json`: an added `macos-arm64` preset.
  - `buildspec.json`: the S-007 identity, with an empty email.
- **LICENSE.** sha256 is `edaef632…33f6`, matching LIC-001. I tried a gnu.org re-fetch from here
  and it **timed out again**, so the LIC-001 caveat still stands: byte identity rests on one curl
  write plus a match with two GNU package copies.
- **Submodule.** The staged gitlink is `887628f` (v0.7.0). `.gitmodules` has one HTTPS entry. The
  submodule tree is clean, including ignored files.
- **libobs facts checked against `plugin/.deps/obs-studio-31.1.1`:**
  - `tick_sources` runs outside `gs_enter_context` (`libobs/obs-video.c:1107-1112`).
  - `obs_init_module` handles a `false` return (`libobs/obs-module.c:148-165,345`).
  - `GS_RGBA` maps to `GL_SRGB8_ALPHA8` (`libobs-opengl/gl-subsystem.h:96`).
- **Hygiene grep.** I grepped every file that would be committed (the `git ls-files --others
  --exclude-standard` list plus `PLAN.md` and `.gitmodules`) for:
  - `/Users/`, `/private/`, `scratchpad`, the user name, the user's local folder names
  - `password`, `credential`, `token`, `4455`, `ws://`, `OBS_ENV`
  - LAN IPs and host or device names

  Results are under Security. I could not grep for the literal websocket password, because it is
  not in my context. No file has a `password=`-style value or a long random string near the
  credential wording.

## Code quality / common issues

`.claude/COMMON-ISSUES.md` holds only the example area, so none of these is a known catalogued
issue.

### Blocker

**B1. Absolute local path with the user name in a session artifact.**
- Where: `develop/output-SPK-006.md:176`.
- It quotes
  a machine-specific absolute path into the session scratchpad (redacted by the orchestrator).
- That string exposes the macOS user name and the user's local folder layout. CLAUDE.md
  rules out both (no absolute machine paths, no private references), and the
  `.gitignore` evidence rule makes this file trackable.
- Fix: replace it with "the session scratchpad", or drop the sentence. The file is ephemeral
  anyway.

**B2. Spike-only instrumentation is still in the plugin, against the S-003 decision.**
- Where: `plugin/src/visualizer-source.cpp:34,146-150,230-233`.
- The core debug callback logs `frame …, fps …, gl_errors …` at `LOG_INFO` every 10 s, for every
  instance, forever. That is about 360 lines an hour per source in the user's OBS log.
- The SPK-005, SPK-006 and SPK-008 handoffs each call it spike instrumentation "for SPK-009 to
  settle". S-003 says it goes before review.
- Fix: remove it, or drop it to `LOG_DEBUG` before the docs pass, so the docs don't describe a line
  that is about to go. `PLAN.md:367-368` lists it as a "stand-in", which then also needs editing.

### Should-fix

**S1. A data race between `video_tick` and an off-thread `video_render`. The same hazard is in the
PLAN's Phase 2 audio design.**
- Where: `plugin/src/visualizer-source.cpp:272-278` and `280-302`, and `PLAN.md:405`.
- `video_tick` runs on the graphics thread **without** the graphics mutex
  (`obs-video.c:1107-1112`). It writes `pending_seconds` and `frame_pending` and calls
  `sv_push_pcm_float(ctx)` directly.
- `video_render` can also run on a non-graphics thread that holds only the graphics mutex.
  obs-websocket `GetSourceScreenshot` does this, and this spike used it for all its evidence. That
  path calls `sv_render_frame(ctx)` and reads and resets the same fields.
- So one `sv_context` can be touched from two threads at once. That breaks the core's
  single-threaded rule, and the CLAUDE.md checklist says "never a direct `sv_push_pcm_*` call".
  Nothing crashed in the spike, but it is a real race.
- The synthetic tone is an acceptable stand-in by itself (see the classification below). The
  problem is that PLAN task 3 keeps the same shape: "`video_tick` drains the ring into
  `sv_push_pcm_float` on the graphics thread".
- Recommendation:
  - Drain the ring, and consume the accumulated `dt`, inside `video_render` / `render_core_frame`,
    under the graphics context, right before `sv_render_frame`.
  - Make the tick side write only atomics or the lock-free ring.
  - Record this in `PLAN.md` task 3 before Phase 2 starts.
  - Verify with a debugger, or with TSan on a screenshot-while-rendering repro, before closing.

**S2. ExternalProject never rebuilds the core when its sources change.**
- Where: `plugin/CMakeLists.txt:60-83`.
- With `SOURCE_DIR` set and no `BUILD_ALWAYS`, the build and install stamps survive a submodule
  bump. After `git submodule update` to a new tag, `cmake --build` silently links the **old**
  `libSynesthesiaVibeCore.a`. D4's model is "pinned and bumped", so this trap will be hit.
- Fix: add `BUILD_ALWAYS TRUE` (the inner Ninja no-op build is cheap), or add a configure-time
  dependency on the submodule HEAD.

**S3. The macOS / arm64 / Xcode assumptions are unguarded.**
- Where: `plugin/CMakeLists.txt:69-71` (`arm64-osx`, `arm64`), `94-105` (Apple frameworks) and
  `110-113` (`XCODE_ATTRIBUTE_EXPORTED_SYMBOLS_FILE`).
- The template presets left in `plugin/CMakePresets.json` now fail in confusing ways:
  - `macos` and `macos-ci` are universal, so they fail at link, because the core is arm64 only.
  - `windows-*` and `ubuntu-*` fail at the triplet and the frameworks.
- A non-Xcode generator would silently drop the export list, which brings back the weak-symbol
  coalescing that SPK-007 fixed.
- Handoffs and `PLAN.md:363` record the gap, but nothing in the build tells the user.
- Fix, one of:
  - a `FATAL_ERROR` unless `OS_MACOS AND CMAKE_OSX_ARCHITECTURES STREQUAL "arm64"` and the
    generator is Xcode;
  - `if(OS_MACOS)` guards plus a clear message;
  - at least an entry in `docs/` saying only `macos-arm64` works.

**S4. `PLAN.md` links into the agent session directory.**
- Where: `PLAN.md:477-478`.
- It points to `.claude/working-sessions/…/spk-008-upstream-asks.md`. `PLAN.md` is the public plan
  of record, and a session folder is not a stable place for it to depend on.
- If the session dir is not committed, the link is dead. The three drafts are already inline
  (`PLAN.md:482-514`), so the pointer adds nothing.
- Fix: drop the pointer, or move the issue-ready text to a durable place (for example
  `docs/upstream-asks.md`, through the document-writer).

### Nit

- **N1. The guard leaves the active texture unit at `GL_TEXTURE0 + 7` while the core runs.**
  - Where: `visualizer-source.cpp:64-69`.
  - The save loop leaves unit 7 active, and the constructor does not switch back before `call()`.
    The core sets its own unit, so the output is correct, and the destructor restores the unit.
  - It does skew the record: the SPK-006 table and the upstream-ask text describe the core as
    "unit 7 to unit 0" (`PLAN.md:232`). That 7 comes from the guard, not from OBS.
  - Fix: restore `active_texture` at the end of the constructor.
- **N2. Scissor and colour mask are not guarded.**
  - Where: `visualizer-source.cpp:47-116`.
  - The core disables scissor and sets the colour mask to all true. This is "no net change" only
    because OBS had scissor off and all channels on in the scenes tested. OBS does not cache
    either, so the cost of a miss is a later OBS draw that is unscissored or writes alpha.
  - Projector, multiview and filter paths were not exercised. Guarding both costs two `glGet`s.
- **N3. Possible null in the module data path.**
  - Where: `visualizer-source.cpp:212`.
  - `std::string resource_path = obs_get_module_data_path(...)` is undefined behaviour if libobs
    returns `NULL`. That is unlikely for a bundle, but a null check is cheap.
- **N4. Null passed to `%s` on the pass path.**
  - Where: `plugin/src/plugin-main.cpp:33`.
  - The pass log uses `device_name` without the null fallback that the refusal branch has
    (line 29).
- **N5. The build-number guard overrides CI build numbers.**
  - Where: `plugin/CMakeLists.txt:10-12`.
  - It seeds `cmake/.CMakeBuildNumber` with `0` before the template runs. On CI that bypasses the
    template's `GITHUB_RUN_ID` branch (`cmake/common/buildnumber.cmake:19-27`).
  - It is fine locally. Revisit it in Phase 4. Its purpose (the template's first-run empty-number
    bug) should go into `.claude/common-issues/`, since comments are not allowed.
- **N6. `fetch-dist.sh` has no curl timeout or retry.**
  - Where: `browser-source/fetch-dist.sh:64`.
  - A stalled connection hangs the script forever. Consider `--connect-timeout` and `--retry 2`.
- **N7. The test-hook overrides are silent.**
  - Where: `browser-source/fetch-dist.sh:176-177`.
  - `FETCH_DIST_BASE_URL` / `FETCH_DIST_ARCHIVE_URL` accept any scheme. The checksum comes from the
    same overridden base, so an override turns integrity into self-consistency.
  - It is documented as a test hook, but a one-line stderr notice when either is set would make
    the override visible.
- **N8. Misleading 404 message for the tarball.**
  - Where: `browser-source/fetch-dist.sh:67`.
  - The 404 text says "check that release v… exists", even when the missing file is the source
    tarball. The developer already noted this.
- **N9. Interrupted runs can leave untracked folders.**
  - `browser-source/.dist-stage.*` and `.dist-old.*` are left behind after a `SIGKILL`, and they
    are not git-ignored. Adding `browser-source/.dist-*/` to `.gitignore` would cover them.
- **N10. A bare `+` matches every name.**
  - Where: `browser-source/obs-page.js:120-136`.
  - A `preset` or `audio` item made only of `+` or spaces becomes the empty key, which substring-
    matches every candidate and picks the first, with a warning. This is harmless, and treating an
    empty key as "no match" is a one-line fix.
- **N11. The `index.html` header comes before `<!doctype html>`.**
  - Where: `browser-source/index.html:1-7`.
  - The plan asked for it after the doctype. The spec allows comments before the doctype, and the
    probe confirmed standards mode, so keeping it as is is fine. The deviation is recorded here only.
- **N12. One unverified README claim.**
  - Where: `browser-source/README.md:313`.
  - It says "Shutdown source when not visible … reopens the audio device". That follows from
    reasoning but was not checked in OBS. Mark it untested, or leave it to the user checklist.
- **N13. Tracked evidence file names break the kebab-case rule.**
  - The tracked evidence `.md` files use uppercase names: `SPK-002-report.md`, `SPK-003-report.md`,
    `SPK-006-state-diff.md`, `SPK-007-report.md` and `SPK-008-plan-diff.md`. CLAUDE.md wants
    lowercase kebab-case.
  - The plan prescribed `BS-00N-…` names, so this is plan-induced. Fix it in the harness template
    rather than by renaming now.

## Spike instrumentation (S-003)

| Item | Where | Classification |
|---|---|---|
| 10 s `LOG_INFO` frame/fps/gl_errors line | `visualizer-source.cpp:34,146-150,230-233` | **Must remove** (or `LOG_DEBUG`). Diagnostic only, ships log spam, and the handoffs call it instrumentation. See B2. |
| Synthetic tone (`push_synthetic_tone`) | `visualizer-source.cpp:152-175,277` | **Acceptable stand-in** without which the source is static. Documented in `PLAN.md:366`; Phase 2 task 3 replaces it. It also carries S1's race, and it keeps pushing about 44k frames/s per instance while off-air. |
| `preset_path` setting with no properties UI, default to a hand-copied bundle preset | `visualizer-source.cpp:29-30,138-144` | **Acceptable stand-in**, documented (`PLAN.md:366-367`). A fresh build plus `ditto` has no presets in the bundle, so the source shows nothing until the presets are copied by hand (also documented). |
| SPK-006 GL-state diff logging | — | Removed. Checked: no `SPK006`/snapshot code in the file. |
| SPK-003 `core linked` line | — | Removed in SPK-005 (checked). |
| `renderer check passed` info line | `plugin-main.cpp:33` | Keep. It is one line per OBS start and useful. |

## Architecture

- **Phase 1 page.** It stays on top of `SynesthesiaCanvas` and uses the engine timer, as planned.
  - Preset names never reach the DOM. User text only selects among manifest paths
    (`obs-page.js:139-162`), and nothing raw goes to `loadPreset`.
  - Failure handling is bounded: one full pass in `createPlaylist`.
  - Audio failures are isolated from rendering. Audio starts after the first preset, and errors
    go to `main().catch`.
  - The deferred-start autoplay design is sound and avoids monkeypatching. It is verified in
    Chrome, and OBS CEF starts `running`.
- **`fetch-dist.sh`.** I traced every exit path:
  - Checksum verification cannot be skipped. A missing line, a mismatch or a partial download each
    exit 1 before anything is staged.
  - The tarball is downloaded before staging. Members are extracted by explicit path under a
    validated root, and symlinks and dotfiles are filtered.
  - The trap restores the old `dist/` if the second `mv` fails. A signal between that `mv` and
    `stage_dir=""` is harmless, because `rm -rf` of the renamed path is a no-op.
  - "Atomic" really means two renames on one filesystem. The handoff documents this.
  - Leftover risks:
    - `unzip` would create symlink entries (bounded by the release checksum).
    - The tarball has no checksum (accepted at C-010).
    - GNU tar and `sha256sum` on Linux are unverified.
- **Plugin render path.**
  - `create` and `destroy` run under `obs_enter_graphics()`. Every core GL call is wrapped by
    `with_core_gl_state`, and the core renders once per tick into the texrender.
  - `GS_FLIP_V` is correct for GL's bottom-left origin.
  - The sRGB handling is right. `GS_RGBA` is `GL_SRGB8_ALPHA8`, so framebuffer sRGB is turned off
    while the core writes. The draw then uses the `_srgb` setter with sRGB on, as
    `obs_source_draw` does. The evidence (50% reads 128) confirms the round trip is identity.
- **The guard matches the SPK-006 table item for item:** program, VAO (and so the element buffer),
  `ARRAY_BUFFER`, draw and read FBO, viewport, blend enable/func/equation, active unit, and 2D/3D
  bindings on units 0–7. The restore order is right: units, then the active unit, then the rest.
  N1 and N2 are the only gaps I would close.
- **Lifetime.** `create`/`destroy` are symmetric, and destroy is safe when `sv_create` failed.
  Destruction is deferred in libobs 31 after the source leaves the tick list, so it does not race
  tick. Evidence: "Number of memory leaks: 0" on every quit, and no errors on collection switches.
- **Renderer check return value (SPK-004 asked).** **Keep `true`.**
  - In libobs 31.1.1, `false` only logs "Failed to initialize module" and frees the module
    (`obs-module.c:161,345`). It shows no failure dialog.
  - `true` keeps one clear, specific warning and leaves the module listed. Neither changes what
    the user sees in Add Source.
- **Exports.** The 7 names match what `load_module_exports` looks up and what the module defines.
  The trap (an optional entry point added later and silently hidden) is documented in `PLAN.md`.
- **dt.** Accumulating while off-air and passing the whole gap in one call is a known follow-up
  (`PLAN.md:373-375`). It fits naturally with S1's fix: consume and clamp `dt` in `render`.
- **Alpha-0 presets.** These are drawn opaque with blending off, so their zero alpha reaches the
  canvas. This is a known follow-up. `OBS_EFFECT_OPAQUE`, or forcing alpha to 1, is the right fix
  for Phase 2.

## Security

- **High (public-repo hygiene): B1.** An absolute path with the user name and the user's
  local folder layout, in `develop/output-SPK-006.md:176`. To verify the fix, re-run the grep
  above over every file to be committed.
- **Medium:** `.vscode/settings.json` (untracked, not part of this run) contains
  `"cmake.sourceDirectory"` set to an absolute machine path (`~/<path>/synesthesia-vibe-obs/plugin`).
  **Do not commit it.** Adding `.vscode/` to `.gitignore` is the user's call.
- **Low:**
  - Ignored evidence logs (`develop/evidence/SPK-003/SPK-003-build.log:36,55-56`) contain
    home-relative paths (`~/<path>/…`). They are git-ignored by the evidence rule and will
    not be committed. Scrub them only if that rule changes.
  - `bs-008-obs-ws.mjs` reads credentials from a file named in `OBS_ENV`. The script is ignored and
    names no path. Handoffs refer only to "local obs-websocket credentials".
- **Credentials.** No password, credential-file path or websocket URL secret was found in any file
  to be committed, by keyword and pattern grep. The literal password could not be grepped (see
  Tooling). The user should still:
  - turn obs-websocket authentication back on;
  - rotate the password shared in chat (gate note).
- **Supply chain.** The zip is checksum-verified against the release's `SHA256SUMS.txt`. The
  tarball has no checksum (C-010, accepted, documented). `curl` uses HTTPS by default.
- **Local serving.** `--bind 127.0.0.1` in the README keeps the local-only preset terms. The README
  correctly warns that `--remote-debugging-port` exposes OBS's browser to local programs.
- **Not applicable:** injection (preset names reach only an FS path chosen from the manifest and
  `console`), auth, and secrets in code.

## Major issues (must fix before docs)

1. **B1.** Scrub the absolute scratchpad path from `develop/output-SPK-006.md:176`.
2. **B2.** Remove the 10 s `LOG_INFO` debug line, or drop it to `LOG_DEBUG`
   (`visualizer-source.cpp:34,146-150,230-233`), and adjust `PLAN.md:367-368`.
3. **S4.** Drop or relocate the `PLAN.md:477-478` pointer into `.claude/working-sessions/`, so the
   docs pass doesn't spread it.
4. **S1** (`PLAN.md` part only). Change Phase 2 task 3 (`PLAN.md:405`) so the ring is drained, and
   `dt` consumed, in `video_render` under the graphics context, not in `video_tick`. The code part
   can wait for Phase 2, because the tone is a stand-in.

S2 and S3 should be fixed or recorded in `docs/concerns.md` by the docs pass. The nits are
optional.

## Minor issues (nice-to-have)

N1–N13 above.

## Tests

N/A: tests are not expected (CLAUDE.md), and none were added. The evidence `.mjs` / `.js` / `.py`
helpers are not tests and are git-ignored, apart from `bs-005-make-beat-wav.py`, which the evidence
rule tracks. One recommendation: the S1 race should get a targeted TSan or debugger repro
(screenshot requests while the source renders) before Phase 2 audio lands.

## Plan alignment notes

- **Scope.** Every planned task was delivered, and BS-006 was removed as decided. LIC-002 (the
  `docs/decisions.md` page) has not run yet, so `PLAN.md:48` points at a page that does not exist
  until the docs pass. Expected.
- **Evidence (Gate 1.5).** I opened BS-005 `fake-device`, BS-008 `obs-a`/`obs-b` and SPK-006
  `two-instances`, and they match their claims. Evidence is sufficient for every task.
  - Phase 1 audio-in-OBS and loopback are pending the user checklist **by decision** (C-003), plus
    the macOS microphone permission for OBS, which only the user can turn on. Not a defect.
  - SPK-004's refusal branch was proven with an env-forced scratch build (S-008). Acceptable.
  - Screenshots come from `GetSourceScreenshot` because GUI automation was disallowed. Acceptable.
  - 1080p60 is unmeasured (the user's profile is unchanged). Documented.
- **Decisions.**
  - The engine meaning of `cycle` (cycle + blend) is implemented and explained in the README.
  - C-005 to C-009 match the code.
  - S-006 template handling is verified byte-identical.
  - The S-007 identity has no personal email.
  - S-012 cleanup was done (plugin bundle removed). The `synesthesia-spike` collection is left for
    the user.
  - S-003 is violated only by B2.
- **For the orchestrator, not adjudicated here:**
  - **D4 "Locked"** (`PLAN.md:51`) was set by the developer, not the user.
  - **`PLAN.md:3`** still says "proposed, no code yet". BS-010 owns that line.
  - `PLAN.md:63` and Phase 1 task 5 still mention the Pages workflow. D5 is open, so leaving them
    is defensible. BS-010 can mark them deferred.
- **Public-plan tone (nit).** `PLAN.md:358` ("while the user fixed the obs-websocket login") and
  `PLAN.md:376` ("The user's OBS profile is 1080×1920 at 30 fps") are session narrative in a public
  plan. Wording such as "a test profile at 1080×1920, 30 fps" would read better.
- **Process.** SPK-003's macOS accessibility GUI fallback went outside S-010. It was already raised
  with the user, and later tasks complied (websocket only). No further action.
- **`.vscode/settings.json`:** do not commit (see Security).

## Round 2 — fix verification (2026-11-23)

Inputs: `develop/output-REVIEW-FIX.md`, `develop/evidence/REVIEW-FIX/review-fix-report.md`,
`develop/output-SPK-006.md`, and the current `plugin/src/visualizer-source.cpp`,
`plugin/src/plugin-main.cpp`, `plugin/CMakeLists.txt` and `PLAN.md`. I reviewed the frozen code
only. A separate OBS runtime re-check of the same build runs in parallel, and the orchestrator
combines the results.

### Decision

**Approve. Gate 2 is satisfied** on code review. The two blockers and the two must-fix
should-fix items are closed. Nothing in the fix pass introduces a new blocking problem. The
remaining items are documented concerns for the docs pass (listed below).

### Item-by-item

| Item | Status | Verified how |
|---|---|---|
| **B1** absolute path in `output-SPK-006.md:176` | **Closed** | The line now reads "in the session scratchpad (`spk006/visualizer-source-instrumented.cpp`)". I re-ran the hygiene grep (absolute home paths, `/private/`, the temp-dir user tag, the user name, the local folder name, the credentials-file name, the websocket scratch folder) over all 87 to-be-committed files (untracked, unstaged and staged, without `.vscode/`). There are no real paths and no user name. The remaining hits are pattern lists and prose, see *Hygiene* below. |
| **B2** 10 s `LOG_INFO` debug callback | **Closed** | `visualizer-source.cpp` no longer has `debug_interval_seconds`, `visualizer_log_debug`, `sv_set_debug_callback` or `sv_set_debug_interval`. The developer's `strings` check shows no `frame %u`. The only `gl_errors` string left belongs to the core's own destroy log. `PLAN.md:366-368` no longer lists it as a stand-in. |
| **S4** `PLAN.md` pointer into `.claude/working-sessions/` | **Closed** | `PLAN.md` has no `working-sessions` hits left. The *Upstream asks* intro now says the drafts are inline. |
| **S1** (PLAN part) | **Closed** | Task 3 (`PLAN.md:414-416`) drains the ring and consumes `dt` in `video_render` under the graphics context, and `video_tick` writes only atomics. Task 4 (`:422-423`) commits in `video_render`. The new follow-up (`:379-387`) describes the race and the TSan / debugger screenshot-loop repro. The risk rows (`:474`, `:478`) match. These extra edits to task 4 and the Risks table are correct and consistent with the CLAUDE.md threading rule (only `sv_prepare_preset*` / `sv_validate_preset*` off the render thread). As agreed, the code side stays open until Phase 2. |
| **S2** `BUILD_ALWAYS` | **Closed** | `plugin/CMakeLists.txt:81` sets `BUILD_ALWAYS TRUE`. Inner Ninja rebuilds changed sources and re-runs its own configure when the core's CMake files change, so a submodule bump now rebuilds the `.a`. The build evidence shows a no-op inner build plus `Up-to-date` installs. |
| **N3** null module data path | **Closed** | `visualizer-source.cpp:206-207`: `data_path ? data_path : ""`. This matches the existing `module_file_path` idiom (`:133`). |
| **N4** null device name | **Closed** | `plugin-main.cpp:33`: same `"unavailable"` fallback as the refusal branch (`:29`). |
| **N1** active texture unit | **Closed, correct** | See the next section. |

### `gl_state_guard` active-unit change (N1)

The constructor at `visualizer-source.cpp:63-69` saves `GL_ACTIVE_TEXTURE`, walks units 0–7 to save
the 2D/3D bindings, then calls `glActiveTexture(active_texture)`.

- **The destructor's restore is still correct.** It does not depend on what is active at entry.
  It sets each unit 0–7 explicitly before rebinding (`:74-78`), then restores the saved active
  unit (`:79`). The member it restores is the value read before the loop, and the fix does not
  change that member. The order is unchanged: units, then the active unit, then the rest.
- **The core now starts on OBS's unit, not unit 7.** The only GL effect of this change is on core
  binds made before its first `glActiveTexture` in a call, such as texture uploads in
  `sv_create` / `sv_load_preset`. Those binds now land on OBS's active unit instead of unit 7.
- **No new state leak.**
  - libobs-opengl only ever activates units below `GS_MAX_TEXTURES` = 8
    (`gl-subsystem.c:164,564,622`, with `cur_textures[GS_MAX_TEXTURES]` as the bound). So OBS's
    active unit is always one the guard saves and restores.
  - OBS does not cache the active unit (`gl_active_texture` is a plain wrapper,
    `gl-helpers.h:186-189`). The restored value is therefore the only thing that matters, and it
    is exact.
  - Theoretical edge: if some other code left a unit at 8 or above active, a core bind before its
    first `glActiveTexture` would land on an unguarded unit. libobs never does this, so I am not
    reporting it as a finding.
- **Cost:** one extra `glActiveTexture` per guarded call. Negligible.
- **The `PLAN.md` "unit 7 → 0" correction is accurate.**
  - `PLAN.md:232` now says `sv_render_frame` "always leaves `GL_TEXTURE0` active". I checked
    this against the core at the pinned `v0.7.0`. Every `glActiveTexture(GL_TEXTURE0 + n)` path
    ends on unit 0:
    - `preset_shader_inputs.cpp:469,564`, `blend_state.cpp:95` and `blur_chain.cpp:203` reset
      to unit 0;
    - `luminance_limiter.cpp:280-284` and `motion_interpolator.cpp:312-316` (`ReleaseTextureUnits`)
      loop down to unit 0.
  - This matches upstream ask #2 (`PLAN.md:508`) and the "texture create/unmap bind on the active
    unit" column, which is what still justifies guarding it.
- **Runtime.** I agree with the developer's reasoning that rendering should not change. The
  orchestrator's OBS re-check is the evidence that settles it, and this review does not depend on
  it.

### Style, comments and siblings

- **No new code comments.** The only `//` lines in `plugin/src/*` are the standard headers
  (lines 1-6). The only `#` lines in `plugin/CMakeLists.txt` are the header.
- **Sibling style holds:** tabs, the existing ternary null-fallback idiom, `static_cast<GLenum>`
  as in the destructor, and `BUILD_ALWAYS` placed with the other ExternalProject keywords.
- **No orphaned code** after the B2 removal. `<cmath>`, `<array>` and `util/platform.h` are still
  used, and nothing references the removed symbols.

### Hygiene

- **My round-1 file.** I rewrote the local folder-name mentions (the old lines 34, 53, 252, 255
  and 259) to neutral wording (`the user's local folder layout`, `~/<path>/…`). Line 52 keeps
  the orchestrator's redaction. No other round-1 text changed.
- **New hits in the developer's REVIEW-FIX artifacts.** They name the local folder while reporting
  on my file:
  - `develop/evidence/REVIEW-FIX/review-fix-report.md:58,60` (line 60 quotes the
    home-relative fragment in full);
  - `develop/output-REVIEW-FIX.md:109`.

  None is an absolute path or names the user, but they repeat the same folder name that this
  hygiene pass removed. **The orchestrator or developer should neutralise those three lines
  before commit.** They are outside my write list.
- `develop/output-SPK-008.md:164`, `review/input-code-review.md:59`, `review-fix-report.md:48,56`
  and `output-REVIEW-FIX.md:75-80` are pattern lists or placeholders only. They are fine.
- `.vscode/settings.json` is still untracked: do not commit it (unchanged from round 1).

### New nits from round 2 (non-blocking)

- **R2-N1. `PLAN.md:405`, task 2, still says "`video_tick` sends the frame delta to
  `sv_render_frame`".** That wording predates this run (`HEAD`). Read literally, it now conflicts
  with task 3 ("`video_render` … consumes the accumulated `dt`"). BS-010 or the docs pass could
  change it to "`video_tick` accumulates the frame delta; `video_render` passes it to
  `sv_render_frame`."
- **R2-N2. `PLAN.md:368` says the debug line "was removed before review."** It was removed in
  the review-fix pass, after round 1. "Removed before merge" would be exact.
- **R2-N3. Phase 2 design notes for task 3** (planning input, not a defect):
  - With the drain in `video_render`, an off-air source fills the ring. Say whether it drops the
    oldest or the newest samples on overflow.
  - `video_render` can run more than once per frame (projectors, multiview, screenshots). The
    tick-to-render handshake should `exchange` the pending `dt` and flag atomically, so two
    renders don't both consume it. This fits the existing "clamp `dt`" follow-up.
- **R2-N4. Unverified after a real core bump: does the plugin relink?** `BUILD_ALWAYS` rebuilds
  `libSynesthesiaVibeCore.a`, but under the Xcode generator the relink of an IMPORTED static
  library is not proven by this run, because the core was unchanged. After the first real
  submodule bump, check that the plugin binary's timestamp moves. This belongs in
  `docs/concerns.md`, next to the BUILD_ALWAYS note.

### Remaining concerns for the docs pass (documented, not blocking)

1. **S3:** the macOS / arm64 / Xcode assumptions in `plugin/CMakeLists.txt` are unguarded.
   Only `macos-arm64` works, and a non-Xcode generator silently drops the export list. Record
   this in `docs/concerns.md` and `docs/integrations.md`, or add a guard in Phase 2 task 1.
2. **S1 code side:** the spike's `video_tick` still calls `sv_push_pcm_float` and writes
   `pending_seconds` / `frame_pending` without the graphics lock
   (`visualizer-source.cpp:263-269`). It races an off-thread `video_render` (`GetSourceScreenshot`)
   until Phase 2 task 3 lands. Put it in `docs/concerns.md` with the TSan repro.
3. **BUILD_ALWAYS visibility:** the core build and install script phases run on every build
   (Xcode says so). This is cheap but visible. Pair it with R2-N4 (relink after a bump is
   unverified).
4. **SPK-006 evidence note:** `develop/evidence/SPK-006/SPK-006-state-diff.md:39,64` records
   `GL_TEXTURE7 → GL_TEXTURE0` as measured. The 7 came from the old guard, not from OBS. The file
   is tracked by the evidence rule. Any doc that cites the table should add a one-line note
   (the core always ends on `GL_TEXTURE0`, and the starting unit is whatever the host had
   active). Or leave the file as a historical record and note it in `docs/concerns.md`.
5. **Round-1 nits still open:** N2 (scissor and colour mask are not guarded) and N5–N13, plus
   R2-N1 to R2-N3 above. The plan-alignment items from round 1 (D4 "Locked" set by the developer,
   `PLAN.md:3` status line, Pages workflow mentions, public-plan tone at `PLAN.md:358,376`) are
   still for the orchestrator and BS-010.
6. **Hygiene:** neutralise `review-fix-report.md:58,60` and `output-REVIEW-FIX.md:109` (see
   above). Do not commit `.vscode/settings.json`.
