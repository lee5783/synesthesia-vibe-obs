# BS-002 evidence: presets, textures, notices and manifest

Captured 2026-11-23 on macOS (stock `/bin/bash` 3.2.57, BSD tar). The raw log is
`BS-002-resources.log` next to this file (git-ignored); this report quotes it in full. Machine paths
are replaced with `<mirror>` (a scratch `file://` mirror outside the repo). The bad tarballs were
built in that scratch directory and served through the two test hooks `FETCH_DIST_BASE_URL` (real
v0.7.0 `SHA256SUMS.txt` + zip) and `FETCH_DIST_ARCHIVE_URL` (the case under test).

Cases covered:

| Case | Result |
|---|---|
| Fresh run from the repo root | exit 0: 15 dist files, 10 presets, 23 textures, `LICENSE` + `NOTICE` |
| `manifest.json` valid JSON (`python3 -I -m json.tool`) | exit 0 |
| Re-run from inside `browser-source/` with `v0.7.0` | exit 0, no leftover stage/old dirs |
| Staged files byte-identical to the tag tarball | presets, textures, `LICENSE`, `NOTICE` all identical |
| Random bytes as the tarball | `could not read`, exit 1, `dist/` unchanged |
| Tarball without `NOTICE` | `is missing LICENSE, NOTICE, …`, exit 1, `dist/` unchanged |
| Tarball whose members start with `../` | `unexpected layout`, exit 1, `dist/` unchanged, nothing written outside |
| Tarball with no `.milk` in `mvp-presets` | `has no presets`, exit 1, `dist/` unchanged |
| Tarball URL missing (`file://` 404) | `download failed`, exit 1, `dist/` unchanged |
| Tarball host unreachable | `download failed`, exit 1, `dist/` unchanged |
| SIGTERM 1 s into a run | exit 1, `dist/` unchanged, no temp dirs left |
| Symlinked `.milk`/`.png`, dotfiles, nested texture dir, a name with `"` and `\` | symlinks, dotfiles and the nested dir skipped; the name is JSON-escaped and round-trips |
| Dist `ResourceLoader.load()` on the generated manifest (Node 24, stub FS) | 10 presets, 23 textures, 34 HTTP 200s, 0 errors |
| `git status` / `git ls-files` / `git check-ignore` | only `fetch-dist.sh` untracked; 0 tracked files under `dist/`; ignored by `.gitignore:10` |
| `shellcheck` | exit 0, no findings |

"`dist/` unchanged" means the same inode and the same digest over every file's sha256 before and
after each failure run.

## Log

```text
$ /bin/bash --version | head -1
GNU bash, version 3.2.57(1)-release (arm64-apple-darwin25)
$ shellcheck browser-source/fetch-dist.sh
exit 0
$ bash -n browser-source/fetch-dist.sh
exit 0
$ /bin/bash browser-source/fetch-dist.sh
Installed synesthesia-vibe web dist 0.7.0 (15 files, 10 presets, 23 textures) into browser-source/dist
exit 0
$ ls browser-source/dist
LICENSE
NOTICE
VERSION
audio-input-worklet.js
audio-input.js
capture-session.js
resource-loader.js
resources/
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
$ ls browser-source/dist/resources/presets | wc -l
      10
$ ls browser-source/dist/resources/textures | wc -l
      23
$ ls -la browser-source/dist/LICENSE browser-source/dist/NOTICE
-rw-r--r--@ 1071 browser-source/dist/LICENSE
-rw-r--r--@ 32430 browser-source/dist/NOTICE
$ head -1 browser-source/dist/LICENSE; head -3 browser-source/dist/NOTICE
MIT License
SynesthesiaVibe
Third-party notices

$ cat browser-source/dist/resources/manifest.json
{
  "presets": [
    "presets/Eo.S. + Geiss - glowsticks v2 02 (Relief Mix).milk",
    "presets/Eo.S. + Phat - CAT Scan (Nirvana flux).milk",
    "presets/Eo.S. - dark side of the moon (plus a few more hits and a pill).milk",
    "presets/Esotic & Rozzer - Now And Later.milk",
    "presets/Goody - Need - Transcendance remix.milk",
    "presets/Rovastar - Fractopia (Upspoken Mix).milk",
    "presets/Rovastar - Jester's Calling 2.milk",
    "presets/Stahlregen & Flexi + Geiss - Tiger No.5 (Random texture flow).milk",
    "presets/Unchained - Fuzzy Sciences.milk",
    "presets/shifter - swarm.milk"
  ],
  "textures": [
    "textures/BeatDropLogo.png",
    "textures/Image415.jpg",
    "textures/cells.jpg",
    "textures/clouds.jpg",
    "textures/clouds2.jpg",
    "textures/heart.jpg",
    "textures/kaite.jpg",
    "textures/lavatex.png",
    "textures/lichen.jpg",
    "textures/manyfish.jpg",
    "textures/moss1.jpg",
    "textures/onefish.jpg",
    "textures/paper.jpg",
    "textures/prayerwheel.jpg",
    "textures/ruin.jpg",
    "textures/seaweed.jpg",
    "textures/sky.jpg",
    "textures/smalltiled_colors3.jpg",
    "textures/smalltiled_electric_nebula.jpg",
    "textures/smalltiled_ensign_meat.jpg",
    "textures/smalltiled_lizard_scales.jpg",
    "textures/sunrise.jpg",
    "textures/wrenches.jpg"
  ],
  "audio": []
}
$ python3 -I -m json.tool browser-source/dist/resources/manifest.json >/dev/null
exit 0
$ ls -a browser-source
.
..
dist
fetch-dist.sh
--- re-run from inside browser-source/ with v0.7.0
$ (cd browser-source && ./fetch-dist.sh v0.7.0)
Installed synesthesia-vibe web dist 0.7.0 (15 files, 10 presets, 23 textures) into browser-source/dist
exit 0
$ ls -a browser-source
.
..
dist
fetch-dist.sh
$ cmp presets/textures/notices against the tag tarball
presets identical to tarball
textures identical to tarball
LICENSE and NOTICE identical to tarball
--- failure paths (FETCH_DIST_BASE_URL=file://<mirror>/release, FETCH_DIST_ARCHIVE_URL=file://<mirror>/<case>)
before: dist inode 41820944 digest 589e5da9848a5ee2
$ FETCH_DIST_ARCHIVE_URL=file://<mirror>/bad-garbage browser-source/fetch-dist.sh
fetch-dist: could not read v0.7.0.tar.gz
exit 1
after:  dist inode 41820944 digest 589e5da9848a5ee2
$ FETCH_DIST_ARCHIVE_URL=file://<mirror>/bad-nonotice browser-source/fetch-dist.sh
fetch-dist: v0.7.0.tar.gz is missing LICENSE, NOTICE, resources/mvp-presets or resources/beatdrop/textures
exit 1
after:  dist inode 41820944 digest 589e5da9848a5ee2
$ FETCH_DIST_ARCHIVE_URL=file://<mirror>/bad-traversal browser-source/fetch-dist.sh
fetch-dist: unexpected layout in v0.7.0.tar.gz
exit 1
after:  dist inode 41820944 digest 589e5da9848a5ee2
$ FETCH_DIST_ARCHIVE_URL=file://<mirror>/empty browser-source/fetch-dist.sh
fetch-dist: v0.7.0.tar.gz has no presets in resources/mvp-presets
exit 1
after:  dist inode 41820944 digest 589e5da9848a5ee2
$ FETCH_DIST_ARCHIVE_URL=file://<mirror>/missing-404 browser-source/fetch-dist.sh
fetch-dist: download failed: file://<mirror>/missing-404/v0.7.0.tar.gz
exit 1
after:  dist inode 41820944 digest 589e5da9848a5ee2
$ FETCH_DIST_ARCHIVE_URL=https://127.0.0.1:9/nothing browser-source/fetch-dist.sh
fetch-dist: download failed: https://127.0.0.1:9/nothing/v0.7.0.tar.gz
exit 1
after:  dist inode 41820944 digest 589e5da9848a5ee2
$ SIGTERM right after start (background run, killed after 1 s)
exit 1
after:  dist inode 41820944 digest 589e5da9848a5ee2
$ ls -a browser-source
.
..
dist
fetch-dist.sh
leftover fetch-dist temp dirs: 0
no ../escape directory written anywhere
--- hostile-but-valid tarball: symlinked .milk/.png, dotfiles, nested texture dir, a preset name with " and \
$ FETCH_DIST_ARCHIVE_URL=file://<mirror>/hostile browser-source/fetch-dist.sh
Installed synesthesia-vibe web dist 0.7.0 (15 files, 1 presets, 1 textures) into browser-source/dist
exit 0
$ ls -la browser-source/dist/resources/presets browser-source/dist/resources/textures
-rw-r--r--@ back\slash.milk
-rw-r--r--@ real.png
$ find browser-source/dist -type l | wc -l
0
$ cat browser-source/dist/resources/manifest.json
{
  "presets": [
    "presets/odd \"quote\" back\\slash.milk"
  ],
  "textures": [
    "textures/real.png"
  ],
  "audio": []
}
$ python3 -I -c json.load(...) presets[0]
presets/odd "quote" back\slash.milk
--- restore the real dist
$ browser-source/fetch-dist.sh
Installed synesthesia-vibe web dist 0.7.0 (15 files, 10 presets, 23 textures) into browser-source/dist
exit 0
--- optional: dist ResourceLoader.load() against the generated manifest (Node 24, stub FS, served by python3 -m http.server)
$ node loader-check.mjs browser-source/dist http://127.0.0.1:8765
ResourceLoader wrote 10 presets, 23 textures, 0 audio files (2040516 bytes) in 179 ms
presets 10 textures 23 audio 0 bytes 2040516
first preset /synesthesiavibe/presets/Eo.S. + Geiss - glowsticks v2 02 (Relief Mix).milk
last texture /synesthesiavibe/textures/wrenches.jpg
exit 0
HTTP requests served: 34, non-200: 0
--- git
$ git status --short --untracked-files=all browser-source
?? browser-source/fetch-dist.sh
$ git ls-files browser-source/dist | wc -l
0
$ git check-ignore -v browser-source/dist/resources/manifest.json browser-source/dist/LICENSE
.gitignore:10:browser-source/dist/	browser-source/dist/resources/manifest.json
.gitignore:10:browser-source/dist/	browser-source/dist/LICENSE
$ shellcheck browser-source/fetch-dist.sh
exit 0
```

## Loader check script (scratch, not committed)

```js
const [distDir, origin] = process.argv.slice(2);
globalThis.document = { baseURI: origin + "/" };
const { ResourceLoader } = await import(distDir + "/resource-loader.js");
const files = new Map();
const fakeModule = {
    FS: {
        stat(path) { if (!files.has(path) && !path.startsWith("/synesthesiavibe")) throw new Error("ENOENT"); return {}; },
        mkdirTree() {},
        writeFile(path, data) { files.set(path, data.length); },
    },
};
const result = await new ResourceLoader(fakeModule).load("./dist/resources/manifest.json");
console.log("presets", result.presets.length, "textures", result.textures.length, "audio", result.audio.length, "bytes", result.bytes);
console.log("first preset", result.presets[0]);
console.log("last texture", result.textures[result.textures.length - 1]);
```
