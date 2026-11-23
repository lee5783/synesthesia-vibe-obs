# Gate 1 decisions (user, 2026-11-23)

| Concern | Answer |
|---|---|
| C-001 1080p60 | **Separate profile.** The agent creates a `synesthesia-1080p60` profile through obs-websocket (1920×1080, 60 fps), measures for 5 minutes, then switches back to the user's profile. The user's own profile settings are never touched. Never while streaming or recording. |
| C-002 signing / notarization | **Ad-hoc now, notarize later.** Ship an ad-hoc signed `.plugin`, an unsigned `.pkg` and a zip. Notarization commands are documented and run only when the user supplies Developer ID credentials out of band. PLAN item 7 stays partly open. |
| C-003 presets in the release | **None bundled.** Users pick a preset folder. Copying the core's presets into the installed bundle is a dev-only step, never packaged. |
| C-004 … C-029 | **Accept all recommended defaults** in `output-plan-concerns.md`. That includes C-006 (process-global PCM buffer: document it and draft upstream ask #4), C-019 (Media Source WAV as the audio fixture), C-026 (remove the plugin at the end), C-027 (osascript quit is allowed) and C-028 (PLAN.md edits only with approval). |

## Standing rules carried over from the previous run

- obs-websocket only, never GUI or accessibility automation. If the websocket is unavailable, stop and ask the user.
- Never edit OBS config files.
- Work in a test scene collection and restore the user's collection afterwards.
- No secrets, credential paths or machine paths in repo files.

## Mid-develop decisions (user, 2026-11-23)

| Question | Answer |
|---|---|
| Dialog saves `preset` when opened (found in P2-010) | **Fix it now** as P2-010b, with a "(first preset)" entry whose value is `""`. The fix is built, but its OBS check is deferred (see below). |
| OBS stuck at its safe-mode prompt after the P2-010 wedge | **Skip the OBS tasks for now.** The P2-010b OBS check, P2-011 (Done-when run) and P2-012 (1080p60) are deferred follow-ups. The run goes on to review and docs, and the PR must list these as not done. The P2-010 leak check is deferred along with P2-011. |

## Post-review decisions (user, 2026-11-23)

| Question | Answer |
|---|---|
| Random textures (the core searches only the bundle's `textures/` folder, which is empty in a release) | **Document now, ask upstream.** The README documents the limit. The docs pass adds upstream ask #5 to `PLAN.md` (let the host set the random-texture folder), drafted and not filed. No code change in this run. |
| Installed test plugin (C-026) | **Remove it now.** The orchestrator removed only `~/Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin`, after checking that its bundle id is `io.github.lee5783.synesthesia-vibe-obs`. OBS had not loaded it, because OBS was stuck at the startup prompt. |

| PLAN.md scope for the docs pass | **Phase 2 status and task states only**, plus upstream ask #5, which was approved in the textures answer above. Asks #4 and #6, risks and follow-ups go in `docs/concerns.md`, not `PLAN.md`. |
