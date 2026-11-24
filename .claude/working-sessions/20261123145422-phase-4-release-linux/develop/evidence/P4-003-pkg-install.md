# P4-003 — TC-M04: the release pkg installed into OBS

Run on 2026-11-24 against the published `v0.1.0` release. It also closes Phase 2's "pkg install not
verified" follow-up (P2-013).

**Result: pass.** Both install paths from `plugin/README.md` load, render and quit cleanly. One
README bug was found: the receipt command needs `--volume ~` (below).

## Setup

- **OBS:** 33.0.3 (mac), the user's `Untitled` profile and collection. No stream, recording or
  virtual camera was running. The plugins folder was empty before the run.
- **Files:** `synesthesia-vibe-obs-0.1.0-macos-arm64.pkg` and `.zip` downloaded from the public
  release URLs. `shasum -a 256 -c SHA256SUMS.txt` passed for both. A browser's quarantine flag
  was added to both files to mimic a browser download.
- **Driver:** obs-websocket only, in the test collection `synesthesia-spike`. OBS was quit with
  `osascript` and relaunched with `open -a OBS`, as the user asked for this run.
- **Fixtures:** the `RT Beat` Media Source on `beat.wav` and the core's tested presets, outside the
  repo.

## Part A: Option A, the installer

| Check | Result |
|---|---|
| Gatekeeper on the quarantined pkg | `spctl --assess --type install`: `accepted`, `source=Notarized Developer ID` |
| `installer -pkg … -target CurrentUserHomeDirectory` | `Installing at base path ~`, `The install was successful.` No admin password. |
| Installed files | `synesthesia-vibe-obs.plugin` and `synesthesia-vibe-obs.plugin.dSYM` in `~/Library/Application Support/obs-studio/plugins/`. The bundle passes `codesign --verify --deep --strict`, carries no quarantine flag and has the hardened runtime. |
| Receipt | `'io.github.lee5783.synesthesia-vibe-obs'`, version `'0.1.0'`, **on volume `~`**. `pkgutil --pkgs` and `pkgutil --pkg-info` without `--volume ~` do not find it. |
| Load (TC-M03) | `renderer check passed (OpenGL, device type 1)`, `plugin loaded successfully (version 0.1.0)`, `sv_init_gl and sv_create succeeded (1920x1080)` |
| Render | `P4-003-pkg-install-a-scene.png`: the preset draws next to the image, colour and text sources. Two visualizer frames 1 s apart differ by 11.9 on average (0–255), so it animates. 30 fps, 4.7 ms average render time. |
| Audio and preset | `audio source attached: RT Beat`, then the first preset prepared (224 ms) and committed |
| Quit | `audio source detached`, `preset worker stopped`, `plugin unloaded`, `Number of memory leaks: 0`. No crash report. |
| GL errors | 0 lines |

The installed bundle has no stapled ticket. The pkg's payload is built before CI staples, so
only the pkg is stapled. This does not matter, because files installed by a pkg are not
quarantined.

## Uninstall, as `plugin/README.md` wrote it

- Steps 1–2 (`rm -rf` the bundle and the `.dSYM`) worked.
- Step 3, `pkgutil --forget "'io.github.lee5783.synesthesia-vibe-obs'"`, **failed**:
  `No receipt for ''io.github.lee5783.synesthesia-vibe-obs'' found at '/'.`, exit 1.
- `pkgutil --volume ~ --pkgs` lists it, and `pkgutil --volume ~ --forget
  "'io.github.lee5783.synesthesia-vibe-obs'"` removed it: `Forgot package … on '~'`.
- Fixed in `plugin/README.md` (*Uninstall*) in the same change.

## Part B: Option B, the zip, still quarantined

- The zip was extracted, and the quarantine flag was put on all 11 files of the bundle, as a
  Finder double-click leaves them. The bundle was then moved into the plugins folder.
- `stapler validate` passed on the bundle. `spctl --assess --type execute` reports "not an app",
  which is expected for a plugin bundle.

| Check | Result |
|---|---|
| Load | `renderer check passed`, `plugin loaded successfully (version 0.1.0)`, `sv_init_gl and sv_create succeeded`. No prompt appeared. |
| Render | `P4-003-pkg-install-b-scene.png`; frames 1 s apart differ by 11.8 on average |
| Quit | `plugin unloaded`, `Number of memory leaks: 0`, 0 GL error lines, no crash report |

So the README's "OBS should load it as downloaded" holds: removing the quarantine flag is only a
fallback.

## Not a regression

`Failed to load 'en-GB' text for module: 'synesthesia-vibe-obs'` appears at load. OBS runs in the
`en-GB` locale and the plugin ships only `en-US.ini`, so OBS falls back to `en-US`. The same line
is in 7 of the 10 earlier OBS logs on this Mac.

## Cleanup

- Both test runs restored the visualizer's settings to `{"audio_source":"","preset_path":""}`,
  removed `RT Beat` and switched back to `Untitled`.
- Afterwards the plugin and `.dSYM` were removed and the receipt forgotten, leaving the plugins
  folder empty as before. OBS was relaunched on `Untitled` / `Untitled`.

## Files

- **Plugin log lines:** `P4-003-pkg-install-a-plugin-log.txt` and
  `P4-003-pkg-install-b-plugin-log.txt`, with paths replaced by `<presets>` and `~`.
- **Screenshots:** `P4-003-pkg-install-{a,b}-scene.png` and `-viz-1.png` / `-viz-2.png`.
- **Script:** `P4-003-pkg-install.mjs`, phases `check` and `cleanup`.
  - It imports `../P2-002/P2-002-ws-session.mjs` from the Phase 2 session's evidence and reads the
    websocket credentials from the file named by `OBS_WS_CREDS`.
  - Run it as `node P4-003-pkg-install.mjs check <shots> <presets> <beat.wav> <a|b>`.
