# BS-002

## Subtasks executed

- Confirmed `browser-source/fetch-dist.sh` was BS-001's version before editing (it is untracked,
  so `git diff --stat` is empty; content matched `output-BS-001.md` and was snapshotted to the
  scratchpad for the diff).
- Split BS-001's `install_dist` into `stage_dist` (unzip into `.dist-stage.*`) and `swap_dist` (the
  unchanged rename-aside / rename-in), so the resources go into the same stage and `dist/` is
  swapped exactly once.
- Added the source tarball download: `https://github.com/lee5783/synesthesia-vibe/archive/refs/tags/v<ver>.tar.gz`,
  overridable through `FETCH_DIST_ARCHIVE_URL` (counterpart of BS-001's `FETCH_DIST_BASE_URL`).
  It is downloaded after the zip checksum passes and before anything is staged.
- `stage_resources`: lists the tarball (`tar -tzf`), takes the top-level directory from the first
  entry and rejects anything outside `[0-9A-Za-z._-]` or starting with `.` (so `../` roots fail),
  extracts only `<root>/LICENSE`, `<root>/NOTICE`, `<root>/resources/mvp-presets` and
  `<root>/resources/beatdrop/textures` into the temp work dir, then copies regular, non-symlink,
  non-dot top-level files into the stage:
  - `*.milk` → `dist/resources/presets/`
  - `*` → `dist/resources/textures/`
  - `LICENSE`, `NOTICE` → `dist/`
- `write_manifest`: `python3 -I` (script on stdin, staged directory as the argument) writes
  `dist/resources/manifest.json` as `{"presets":["presets/…"],"textures":["textures/…"],"audio":[]}`,
  sorted, 2-space indented, via `json.dump`, so `"` and `\` are escaped correctly.
- `require_tools` now also checks `tar` and `python3`.
- Success line: `Installed synesthesia-vibe web dist 0.7.0 (15 files, 10 presets, 23 textures) into browser-source/dist`.

## Files changed

- `browser-source/fetch-dist.sh` (edited; still untracked, not committed)
- `browser-source/dist/**` (regenerated, git-ignored)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md` (BS-002 section added)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-BS-002.md` (new)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/BS-002/bs-002-resources-report.md` (new)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/BS-002/BS-002-resources.log` (new, git-ignored)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/session.log` (appended)

## What changed and why

PLAN Phase 1 item 1, second half (C-001/C-002/C-010 resolved at Gate 1: tag source tarball, local
serving only). The web dist has no presets, textures or notices. BS-003's page loads
`./dist/resources/manifest.json` through `ResourceLoader.load()`, which resolves each entry
relative to the manifest and keeps only the basename, so the manifest lists flat
`presets/<name>.milk` and `textures/<name>` paths.

BS-001's guarantees carry over: everything happens in the `mktemp -d` work dir and the
`.dist-stage.*` directory, and the old `dist/` moves only after the stage is complete (web dist +
notices + resources + manifest). Any failure leaves the old `dist/` in place, and the trap removes
the temp and stage directories. Archive contents are only listed, extracted by explicit member
path and copied; nothing from the tarball is executed or sourced.

Placement: the delegation summary said LICENSE/NOTICE go "into dist/resources/", but the plan's
BS-002 row and acceptance criteria say `dist/LICENSE` and `dist/NOTICE`. I followed the plan.

## Verification

- `bash -n browser-source/fetch-dist.sh` → exit 0.
- `shellcheck browser-source/fetch-dist.sh` → exit 0, no findings.
- Ran under `/bin/bash` 3.2.57 with BSD tar and `/usr/bin/python3`. No bash-4 features.
- `python3 -I -m json.tool browser-source/dist/resources/manifest.json` → exit 0.
- No existing test suite covers this area.

## Evidence

- `evidence/BS-002/bs-002-resources-report.md` quotes every run (raw log
  `evidence/BS-002/BS-002-resources.log`, git-ignored). Key output:
  - fresh run: `Installed synesthesia-vibe web dist 0.7.0 (15 files, 10 presets, 23 textures) into browser-source/dist`, exit 0
  - `ls dist/resources/presets | wc -l` → 10, `ls dist/resources/textures | wc -l` → 23,
    `dist/LICENSE` (1071 B, "MIT License") and `dist/NOTICE` (32430 B) present
  - manifest excerpt:
    ```json
    {
      "presets": [
        "presets/Eo.S. + Geiss - glowsticks v2 02 (Relief Mix).milk",
        "presets/Eo.S. + Phat - CAT Scan (Nirvana flux).milk",
        …
        "presets/shifter - swarm.milk"
      ],
      "textures": [
        "textures/BeatDropLogo.png",
        …
        "textures/wrenches.jpg"
      ],
      "audio": []
    }
    ```
  - re-run from `browser-source/` with `v0.7.0`: exit 0, no `.dist-*` leftovers; staged files are
    byte-identical to the tag tarball
  - bad tarballs through `FETCH_DIST_ARCHIVE_URL=file://<mirror>/<case>`, each exit 1 with `dist/`
    inode and content digest unchanged:
    - random bytes → `fetch-dist: could not read v0.7.0.tar.gz`
    - no `NOTICE` → `fetch-dist: v0.7.0.tar.gz is missing LICENSE, NOTICE, resources/mvp-presets or resources/beatdrop/textures`
    - `../` members → `fetch-dist: unexpected layout in v0.7.0.tar.gz` (nothing written outside)
    - no `.milk` → `fetch-dist: v0.7.0.tar.gz has no presets in resources/mvp-presets`
    - missing URL, unreachable host → `fetch-dist: download failed: …`
    - SIGTERM 1 s in → exit 1, no temp dirs left
  - hostile-but-valid tarball (symlinked `.milk`/`.png`, dotfiles, nested texture dir, a name with
    `"` and `\`): symlinks, dotfiles and the nested dir are skipped; the odd name is escaped
    (`"presets/odd \"quote\" back\\slash.milk"`) and round-trips through `json.load`
  - the dist's own `ResourceLoader.load("./dist/resources/manifest.json")` under Node 24 with a stub
    FS against `python3 -m http.server`: `ResourceLoader wrote 10 presets, 23 textures, 0 audio files (2040516 bytes)`,
    34 HTTP 200s, no errors
  - `git status --short --untracked-files=all browser-source` → only `?? browser-source/fetch-dist.sh`;
    `git ls-files browser-source/dist` → 0; `git check-ignore` → `.gitignore:10:browser-source/dist/`

## Known limitations / follow-ups

- The source tarball has no published checksum (C-010, accepted). It is fetched over HTTPS from the
  same tag but not verified; GitHub archive bytes are not guaranteed stable, so a pinned hash was
  not added. An upstream ask (presets + checksum as a release asset) would close this.
- The 404 message for the tarball reuses BS-001's text ("check that release v<ver> exists"); for
  the tarball the thing missing is the tag. Minor wording, left alone to keep `download` shared.
- Not run with GNU tar / on `ubuntu-latest` (no Docker, no `gtar` here). The script uses only
  `tar -tzf` and `tar -xzf … -C dir <member paths>` with exact directory names (no wildcards), which
  GNU tar accepts the same way, but that is unverified.
- The run now downloads about 4.2 MB of tarball on top of the 1.1 MB zip on every invocation,
  even when only the dist is wanted. No caching or skip flag; the plan did not ask for one.
- Only top-level files in `textures/` are taken (subdirectories skipped), matching
  `ResourceLoader`'s flat basename layout. v0.7.0 has no subdirectories there.
- Two test hooks are now undocumented outside the script: `FETCH_DIST_BASE_URL` and
  `FETCH_DIST_ARCHIVE_URL`. Document-writer: mention both, the `tar`/`python3` requirement, the
  `dist/resources/` layout and the local-serving-only preset terms in `docs/integrations.md`; BS-007's
  README should credit the presets (BeatDrop notice in `dist/NOTICE`).
- The `.dist-stage.*` / `.dist-old.*` SIGKILL leftover note from BS-001 still applies.
- No test was added (project default).

## Handoff for downstream review

- Acceptance criteria addressed: manifest lists 10 presets and 23 textures and passes
  `python3 -I -m json.tool`; `dist/LICENSE` and `dist/NOTICE` exist; re-runs replace everything
  cleanly (single stage + swap); nothing under `dist/` is tracked.
- Areas to scrutinise: the root-directory derivation from the first `tar -tzf` line and its
  character check; `copy_files`' glob (`"$source"/*"$suffix"`, dotfiles excluded by default
  `dotglob` off, `-f && ! -L` filter); that `stage_resources` runs before `swap_dist`; the
  `python3 -I -` heredoc (no cwd on `sys.path`, staged path passed as `argv[1]`).
- How to run: `browser-source/fetch-dist.sh [version]`. To reproduce a bad-tarball case, mirror
  `v0.7.0/SHA256SUMS.txt` and `v0.7.0/synesthesia-vibe-web-0.7.0.zip` into `<dir>/release/`, put a
  bad `v0.7.0.tar.gz` into `<dir>/<case>/`, and run with
  `FETCH_DIST_BASE_URL=file://<dir>/release FETCH_DIST_ARCHIVE_URL=file://<dir>/<case>`.
