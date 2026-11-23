# Second Gate 1 — spike decisions (user, 2026-11-23)

| Question | Answer |
|---|---|
| S-010 driving OBS | **obs-websocket enabled** by the user on the local default port. The orchestrator stores the credentials outside the repo and passes their location to agents in the delegation packet. **No password, credential path or websocket URL secret goes in any repo file, session artifact or log.** Refer to them only as "local obs-websocket credentials". Tests run in a separate `synesthesia-spike` scene collection. OBS config files are never edited. |
| S-003 where spike code lives | **`plugin/` is the real start of the Phase 2 plugin.** Spike-only instrumentation is removed before review. Nothing is committed until the user asks; then it goes on a feature branch. |
| S-001 / S-005 libobs and core hookup | **Template buildspec as shipped:** obs-studio 31.1.1 + obs-deps. Bump to 33.0.3 only if 31.1.1 fails with Xcode 27. The core is built separately with ExternalProject and the plugin links the installed static library. |
| S-002, S-004, S-006–S-009, S-011–S-014, C-021 | **Accept all recommended defaults** in `output-plan-concerns.md`. |

## Note

The user shared the websocket password in chat. Recommend the user rotates it after the run.

## Mid-develop decisions (user, 2026-11-23, raised by SPK-003)

| Question | Answer |
|---|---|
| obs-websocket authentication | The password the user first gave was rejected (4009). The orchestrator reproduced the rejection. **The user turned authentication off** on the localhost websocket for the spike. The orchestrator confirmed an unauthenticated connection works. The user should turn authentication back on after the run. |
| GUI automation fallback | **Not allowed.** SPK-003 used macOS accessibility clicks after the websocket failed, which S-010 did not approve. From now on, agents use only the websocket. If it is unavailable, they stop and ask the user to click. |

## Post-spike decision (user, 2026-11-23)

| Question | Answer |
|---|---|
| D4 state | **Locked as recorded by SPK-008.** The core is a git submodule pinned to `v0.7.0` (`887628f`). It is built separately with `ExternalProject_Add` and linked as an imported static library. The XCFramework fallback is not needed. The user confirmed the developer's "Locked" state. The docs pass records D4 in `docs/decisions.md` next to D1. |
