# Verification tooling (headless Chrome, OBS scripting)

> Part of the known-issues corpus — see [`../COMMON-ISSUES.md`](../COMMON-ISSUES.md) for the
> index. **Analysis date:** 2026-11-23 (Chrome 157 on macOS, OBS 33.0.3). How verification is done
> overall: `docs/testing.md`.

## Chrome's fake audio file delivers silence on macOS

- **Symptom:** headless Chrome started with `--use-fake-device-for-media-stream
  --use-file-for-fake-audio-capture=<wav>` gives the page a running audio input, but the PCM is all
  zeros and the engine's audio levels stay at 0. Without the file option the default fake beep
  arrives, so it looks like the WAV is the problem.
- **Root cause:** the sandboxed audio service cannot read the WAV file.
- **Fix / workaround:** add `--disable-features=AudioServiceSandbox` to the Chrome flags. Also pass
  `--use-fake-ui-for-media-stream` (no permission prompt) and
  `--autoplay-policy=no-user-gesture-required` (no click needed).
- **How to verify this is still current:** run once with and once without the feature flag and
  compare the PCM peak the page receives.

## `osascript -e 'tell application "OBS" to quit'` returns `User canceled (-128)`

- **Symptom:** the quit command exits non-zero with error -128, so a script treats the quit as
  failed.
- **Root cause:** not investigated. The error is cosmetic: the quit itself proceeds. OBS exited
  within 2 s each time, and its log ended with a normal shutdown and `Number of memory leaks: 0`.
- **Fix / workaround:** ignore the exit status; instead wait until the OBS process is gone (e.g.
  poll `pgrep -x OBS`) before copying a new plugin build or relaunching.
- **How to verify this is still current:** run the quit and check the exit status, the process list
  and the end of the OBS log.
