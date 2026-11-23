# BS-001 evidence: fetch-dist.sh

Run on macOS (stock `/bin/bash` 3.2.57, BSD `mktemp`, `/usr/bin/shasum`) on 2026-11-23, from the
repo root unless stated. The raw log sits next to this file as `BS-001-fetch-dist.log`
(git-ignored). Scratch paths are shown as `<scratch>`.

## 1. Fresh install (no `dist/` before)

```
$ ls browser-source
fetch-dist.sh
exit=0

$ /bin/bash browser-source/fetch-dist.sh 0.7.0
Installed synesthesia-vibe web dist 0.7.0 (15 files) into browser-source/dist
exit=0

$ ls browser-source/dist
VERSION
audio-input-worklet.js
audio-input.js
capture-session.js
resource-loader.js
synesthesia-canvas.js
synesthesia-context.js
synesthesia-error.js
synesthesia-module.js
synesthesia-sync-group.js
synesthesia-vibe-wasm.js
synesthesia-vibe-wasm.wasm
synesthesia-vibe.js
wav-feeder.js
webgl-context.js
exit=0

$ cat browser-source/dist/VERSION
0.7.0
exit=0
```

## 2. Re-run from inside `browser-source/` (default version, then `v` prefix)

```
$ cd browser-source && ./fetch-dist.sh
Installed synesthesia-vibe web dist 0.7.0 (15 files) into browser-source/dist
exit=0

$ cd browser-source && ./fetch-dist.sh v0.7.0
Installed synesthesia-vibe web dist 0.7.0 (15 files) into browser-source/dist
exit=0

$ ls -A browser-source
dist
fetch-dist.sh
```

No `.dist-stage.*` or `.dist-old.*` leftovers after replacement.

## 3. Failure paths with an existing `dist/` in place

Fingerprint before:

```
dist inode 41819574
content digest 0114a95e37c939e30bd37b42d31fde8cadb77ea022e607fa91298ba32a523bd0
file count 15
```

```
# 3a. bogus version (release does not exist)
$ browser-source/fetch-dist.sh 9.9.9
fetch-dist: not found (HTTP 404), check that release v9.9.9 exists: https://github.com/lee5783/synesthesia-vibe/releases/download/v9.9.9/SHA256SUMS.txt
exit=1

# 3b. invalid version string
$ browser-source/fetch-dist.sh ../../etc
fetch-dist: invalid version '../../etc' (expected e.g. 0.7.0 or v0.7.0)
exit=1

# 3c. checksum mismatch: local mirror with one byte appended to the real zip
$ FETCH_DIST_BASE_URL=file://<scratch>/mirror-tampered browser-source/fetch-dist.sh 0.7.0
fetch-dist: checksum mismatch for synesthesia-vibe-web-0.7.0.zip
exit=1

# 3d. SHA256SUMS.txt has no line for the zip
$ FETCH_DIST_BASE_URL=file://<scratch>/mirror-no-entry browser-source/fetch-dist.sh 0.7.0
fetch-dist: SHA256SUMS.txt has no entry for synesthesia-vibe-web-0.7.0.zip
exit=1

# 3e. unreachable host
$ FETCH_DIST_BASE_URL=https://invalid.invalid browser-source/fetch-dist.sh
fetch-dist: download failed: https://invalid.invalid/v0.7.0/SHA256SUMS.txt
exit=1

# 3f. too many arguments
$ browser-source/fetch-dist.sh 0.7.0 extra
fetch-dist: usage: fetch-dist.sh [version]
exit=1
```

Fingerprint after all six failures: identical inode, digest and count, so `dist/` was not touched.

```
dist inode 41819574
content digest 0114a95e37c939e30bd37b42d31fde8cadb77ea022e607fa91298ba32a523bd0
file count 15
```

## 4. `dist/` is git-ignored

```
$ git status --short --untracked-files=all browser-source
?? browser-source/fetch-dist.sh

$ git check-ignore -v browser-source/dist/VERSION
.gitignore:10:browser-source/dist/	browser-source/dist/VERSION
```

## 5. Static checks

```
$ bash -n browser-source/fetch-dist.sh
exit=0

$ shellcheck browser-source/fetch-dist.sh
exit=0
```

## 6. `sha256sum` fallback (PATH with no `shasum`)

```
$ PATH=<scratch>/bin-no-shasum command -v shasum sha256sum
<scratch>/bin-no-shasum/sha256sum

$ PATH=<scratch>/bin-no-shasum browser-source/fetch-dist.sh 0.7.0
Installed synesthesia-vibe web dist 0.7.0 (15 files) into browser-source/dist
exit=0

$ PATH=<scratch>/bin-no-shasum FETCH_DIST_BASE_URL=file://<scratch>/mirror-tampered browser-source/fetch-dist.sh 0.7.0
fetch-dist: checksum mismatch for synesthesia-vibe-web-0.7.0.zip
exit=1
```

This used macOS's `/sbin/sha256sum`, not GNU coreutils; `ubuntu-latest` was not run.

## 7. SIGTERM mid-run

```
$ fetch-dist.sh 0.7.0 & kill -TERM after 0.4s -> exit=1
$ ls -A <scratch>/tmp-kill   (TMPDIR of the killed run; nothing listed = cleaned)
$ ls -A browser-source
dist
fetch-dist.sh
dist content digest unchanged
```
