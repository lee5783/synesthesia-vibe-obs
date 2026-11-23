# Browser Source in OBS

> Part of the known-issues corpus — see [`../COMMON-ISSUES.md`](../COMMON-ISSUES.md) for the
> index. **Analysis date:** 2026-11-23 (OBS 33.0.3, obs-browser 2.27.6 / CEF 150, macOS 26).

## Page reports `audio unavailable (NotFoundError: Requested device not found)` on macOS

- **Symptom:** visuals run but never react; the page console (and the OBS log as
  `[obs-browser: '<source>'] Error: …`) shows
  `obs-page: audio unavailable (NotFoundError: Requested device not found), running without audio`.
  `enumerateDevices()` returns no audio inputs.
- **Root cause:** macOS microphone access for OBS is off. The OBS log starts with
  `[macOS] Permission for audio device access denied.` Once the permission has been decided, macOS
  never prompts again, and Chromium reports `NotFoundError`, not `NotAllowedError`.
- **Fix / workaround:** System Settings → Privacy & Security → Microphone → turn on OBS, then quit
  and restart OBS with `--enable-media-stream`. Only the user can change this; agents must not.
- **How to verify this is still current:** with access off, start OBS with the flag and check the
  log line and the page error. The error name when only the flag is missing (permission on) is
  still unconfirmed; the user checklist covers it.

## First-ever Browser Source in a profile stays blank until OBS restarts

- **Symptom:** the first Browser Source created in a fresh OBS profile (seen with
  `--remote-debugging-port=9222` on the command line) never loads: transparent frame, and the
  debugging port does not answer.
- **Root cause:** not isolated (first CEF start in the profile vs the debugging flag). Seen once.
- **Fix / workaround:** quit and restart OBS. Every later load worked.
- **How to verify this is still current:** create a new OBS profile, add the source on the first
  start, with and without the debugging flag, and see which case reproduces it.
