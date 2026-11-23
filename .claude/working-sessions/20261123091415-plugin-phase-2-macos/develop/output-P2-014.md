# P2-014

## Subtasks executed
- Read the gate-1 decisions, the P2-014 task row and section, the concerns, and every develop
  handoff P2-002 … P2-010b for README doc-gap items.
- Checked every setting label, default, range, list entry, hotkey name and quoted log line against
  `plugin/src/visualizer-source.cpp`, `plugin/src/plugin-main.cpp` and
  `plugin/data/locale/en-US.ini`.
- Wrote `plugin/README.md` in the shape of `browser-source/README.md`.
- Cross-checked labels, links, anchors and paths mechanically; wrote the claim-check evidence.

## Files changed
- `plugin/README.md` (new)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md` (P2-014
  section appended)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-014/P2-014-claim-check.md` (new)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-P2-014.md` (this file)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/session.log` (log lines)

## What changed and why
`plugin/README.md` is the user-facing install and usage page (PLAN item 7, C-017). Sections:
intro and known limits; contents; what you need; Step 1 install (pkg with the unnotarized
Gatekeeper workaround for macOS 15+ and 14-, a `installer -target CurrentUserHomeDirectory`
alternative, zip with `xattr -dr com.apple.quarantine`, upgrading); Step 2 OpenGL renderer check;
Step 3 add the source; Settings by group (Presets, Transitions with the cycle + blend timing,
Audio with single source / Whole mix / "(missing)" / muted = silence, Output with portrait sizing,
128–8192 limits and render scale); Hotkeys (the four labels, history of 50, pause semantics,
hidden-source behaviour); Presets (where to get them, textures next to presets, random-texture
limit, CC-BY-NC-SA terms, C-003); hidden sources, shared audio (C-006) and performance; reading the
OBS log; troubleshooting (renderer refusal line verbatim, plugin not loaded, empty folder lines,
failed presets, missing textures, no audio reaction, shared audio, stutter, list vs screen);
uninstall (bundle and pkg receipt); a "For developers" link to `docs/integrations.md`; credits and
licences. No file header (README convention). Root `README.md` untouched. Pkg and zip file names
are left to the release, as instructed; the bundle name comes from the build output.

## Verification
- `python3 -I` check (inline, in the evidence report): every `en-US.ini` label appears in the
  README (Track 2–5 via "Track 1–Track 6"); no absolute paths; every in-page anchor resolves.
- `ls docs/integrations.md LICENSE browser-source/README.md`: all relative link targets exist.
- No build: documentation only. No OBS run, per delegation.

## Evidence
- `evidence/P2-014/P2-014-claim-check.md` — each README claim mapped to code, locale, build output
  or the handoff that observed it in OBS, plus the list of unverified claims.

## Known limitations / follow-ups
- Unverified (see the evidence report): pkg/zip artifact names and real installer behaviour
  (P2-013 concurrent), the pkg receipt identifier, Gatekeeper dialog wording, whether OBS refuses a
  quarantined bundle, OBS UI paths not exercised this run (Renderer setting and the Metal option
  name, Hotkeys listed under the source name, Add Existing, Advanced Audio Properties), and the
  refusal on a real Metal renderer (`docs/concerns.md` already notes it was never observed).
- "OBS 31 or later" rests on the libobs 31.1.1 build target; only 33.0.3 was checked.
- If P2-013 changes the bundle name or the install location, Step 1, Troubleshooting and Uninstall
  need the same change.
- Doc gaps for document-writer (P2-016): `docs/integrations.md` should hold the developer build,
  dev-only preset copy, packaging and notarization commands this README links to; its spike-era
  bundled-presets install snippet is stale (P2-006/P2-010 handoffs). `docs/README.md` /
  `docs/structure.md` can list `plugin/README.md` as the user page.
- No test needed.

## Handoff for downstream review
- Acceptance criteria addressed: requirements; install from pkg or zip with the Gatekeeper note;
  adding "SynesthesiaVibe Visualizer"; every setting; hotkeys (Settings → Hotkeys); presets and
  licence terms (C-003); multi-instance audio limit (C-006); troubleshooting (renderer refusal line,
  empty folder, no audio reaction); uninstall; no root README change; no machine paths beyond
  `~/Library/...`.
- Areas to scrutinize: the Gatekeeper steps per macOS version; the Blend time row (first preset of
  a new source is a hard cut, everything else blends, per `take_preset_job`); the Whole mix
  exclusions; reconcile Step 1 with P2-013's actual artifacts.
- How to check: follow the README on the packaged build from P2-013.
