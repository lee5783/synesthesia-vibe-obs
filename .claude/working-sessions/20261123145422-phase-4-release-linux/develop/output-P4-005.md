# P4-005

## Subtasks executed
- Linux job `Linux x86_64` in `.github/workflows/build-plugin.yaml` on `ubuntu-24.04`.
- Packages: `ppa:obsproject/obs-studio` then `obs-studio`, `build-essential`, `libsimde-dev`,
  `libgl-dev`, `libegl-dev`, `libopengl-dev`, `libgles2-mesa-dev` (as the template), `ninja-build`,
  `pkg-config`, and the vcpkg `glfw3` port's list (`xorg-dev`, `libxinerama-dev`, `libxcursor-dev`,
  `libglu1-mesa-dev`). The port is built without its `wayland` feature, so no Wayland dev packages.
- vcpkg as on macOS (same pinned commit, binary cache keyed
  `vcpkg-linux-x64-linux-<vcpkg commit>-<core gitlink>`, saved even after a failed build).
- Configure `cmake --preset ubuntu-ci-x86_64 -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=$CONFIG`,
  build `cmake --build --preset ubuntu-ci-x86_64`.
- *Record libobs version*: the `obs-studio` package version and `LIBOBS_API_*_VER` from
  `/usr/include/obs/obs-config.h`, in the log and the run summary (C-009).
- *Check plugin binary* (fails the job): exports, `TEXTREL`, `ldd`, `dlopen` probe.
- *Package* `cmake --build build_x86_64 --target package`; *Inspect package* (fails the job):
  control fields, contents, packaged exports.
- Upload `synesthesia-vibe-obs-<ver>-x86_64-linux-gnu.deb` as workflow artifact
  `<name>-<ver>-x86_64-linux-gnu` only (C-006: not a release asset). No `.ddeb`, no source tarball
  (C-007).

## Files changed
- `.github/workflows/build-plugin.yaml` (the `linux` job)
- `.claude/working-sessions/20261123145422-phase-4-release-linux/develop/evidence/P4-005-linux-check-dry-run.md`

## What changed and why
Checks (each prints an `::error::` and exits 1):
- `nm -D --defined-only`: the `obs_module_*` names must equal the 7 in `exported-symbols.map`
  (diff and count); no other defined dynamic symbol except the linker-reserved `_init`, `_fini`,
  `_edata`, `_end`, `__bss_start`; nothing matching `glad`, `^sv_`, `projectm`, `nseel`, `^_Z`.
- `readelf -d` has no `TEXTREL` (catches non-PIC objects, C-015).
- `ldd` shows no `not found`.
- `dlopen` probe: inline `python3 -I` with ctypes, `RTLD_NOW | RTLD_LOCAL` (stricter than the
  plan's `RTLD_LAZY`: every function import must bind at load), resolves `obs_module_load`,
  `obs_module_unload`, `obs_module_ver`, `obs_module_set_pointer`. Needs no display.
- `dpkg-deb -I` printed; `Maintainer` non-empty; `Depends` names the OBS package.
- `dpkg-deb -c` has `./usr/lib/x86_64-linux-gnu/obs-plugins/synesthesia-vibe-obs.so` and
  `./usr/share/obs/obs-plugins/synesthesia-vibe-obs/locale/en-US.ini`; no presets or textures.
- The `.so` extracted from the `.deb` still exports the 7 names (CPack strips it for the `.ddeb`).

No probe program, test target or Xvfb is committed (C-027); the checks live inline in the workflow.

## Verification
- `actionlint` with shellcheck: 0 errors.
- Dry run (no Linux here): the *Check plugin binary* and *Inspect package* scripts, extracted
  verbatim, pass `bash -n`; the probe's Python parses; with stand-in `nm`/`readelf`/`ldd`/`dpkg-deb`
  the export check passes clean output, tolerates linker-reserved names, and fails on a leaked
  `gladLoadGL` (exit 1); the package check passes on a real-format `dpkg-deb -c` listing.

## Evidence
- `evidence/P4-005-linux-check-dry-run.md`. **This is not a Linux build.** CI is the first real
  configure, compile, link and package of the Linux port. Run id, check excerpts, `dpkg-deb -c`
  listing, timings and the libobs version come back with the first CI logs.

## Known limitations / follow-ups
- **Stop rule (C-013, C-015):** if the core does not compile or link on Linux without editing
  `external/synesthesia-vibe/`, I stop and draft the upstream ask. Nothing in the core was edited or
  found to need editing so far.
- If `dpkg-shlibdeps` can't map `libobs.so` to a package, packaging fails. The template uses the
  same setup, so this is not expected.
- P4-008 (real OBS on Ubuntu X11 and Wayland) stays with the user; the Linux `.deb` is not shippable
  until then.
- Doc gaps (P4-012): `docs/integrations.md` (Linux job, packages, checks, libobs floor),
  `docs/stack.md` (Ubuntu 24.04 image, GCC 13, GLVND/EGL, PPA libobs), `docs/testing.md` (CI checks),
  `docs/concerns.md` (Linux untested in OBS, Flatpak unsupported, PPA libobs floor).

## Handoff for downstream review
- Acceptance criteria addressed: ubuntu-24.04 job, PPA libobs, GL/EGL/X11 dev packages, vcpkg,
  build, failing export/ldd/dlopen checks, libobs version recorded, `.deb` contents checked, `.deb`
  artifact only.
- Areas to scrutinize: the linker-reserved allow-list; `Depends` must contain `obs`.
- How to run: push the branch; watch workflow **CI**, job **Build / Linux x86_64**.

## CI iteration 1 (run 45139712419, PR #3, commit f191650)

**Result:** `Build / macOS arm64` passed, with every inspection check green. `Build / Linux x86_64`
failed inside the core's ExternalProject configure, during `vcpkg install`, at package 17/27,
`pthread-stubs:x64-linux`:
`pthread-stubs currently requires the following programs from the system package manager: autoconf autoconf-archive automake libtoolize`.
The `CMAKE_C_COMPILER not set` lines after it are knock-on errors from the aborted inner
`project()`. Our sources and the core sources were never reached. The cause is CI tooling, so the
C-013 stop rule does not apply.

**Analysis of the 27-package plan** (portfiles in a local vcpkg checkout at the pinned `5f96cd15`;
the run used the same git-tree SHAs):
- Already built before the failure: `vcpkg-cmake*`, `vcpkg-make`, `xorg-macros`, `xtrans`,
  `vcpkg-tool-meson`, `zlib`, `libiconv`, `libxml2`, `libxslt`, `xproto`, `xcb-util-m4`,
  `xcb-proto`, `pthreads`, `pthread`.
- `libxau`, `libxdmcp`, `libxcb`, `libx11`, `xproto`, `xtrans`, `xorg-macros` and `xcb-proto` are
  **empty packages on Linux**. Their portfiles skip the build unless
  `X_VCPKG_FORCE_VCPKG_X_LIBRARIES` is set, and expect the system X11 dev packages, which
  `xorg-dev` covers. `libiconv` is empty too (glibc iconv).
- `pthread-stubs` is the one autotools build: `vcpkg_make_configure(AUTORECONF)`. The vcpkg-make
  check needs `autoreconf`, `aclocal`, `libtoolize` and the autoconf-archive macros. It also stops
  on missing `libltdl` for configure scripts that use ltdl, and warns about `autopoint`/gettext.
- The rest are CMake ports: `bzip2`, `glfw3` (against system X11; the `wayland` feature is off),
  `glm`, `gtest`, `projectm-eval`.

**Fix (one round trip, covering the whole chain):** the Linux apt list in `build-plugin.yaml` now
adds:
- `autoconf autoconf-archive automake libtool libltdl-dev gettext make`, for `pthread-stubs`'
  autoreconf and the vcpkg-make checks;
- `bison flex gperf python3 libxml2-utils xsltproc`, which the xcb/X11 ports name (python3,
  xmllint, xsltproc) and which cost little if the empty ports never need them;
- `curl zip unzip tar`, vcpkg's own requirements (already on the image; listed explicitly);
- `libxkbcommon-dev`, cheap insurance for glfw.

All of these are standard Ubuntu 24.04 packages. The packages from run 1 installed fine.

**Cache fix (both jobs):** the save step already ran on failure (`!cancelled()`). Run 1 saved 16
archives (6.6 MB) under key `vcpkg-linux-x64-linux-<vcpkg>-<core>`. Because that key was fixed,
run 2 would have restored it as an **exact hit**, `cache-hit == 'true'` would have skipped the
save, and the packages built after the failure point would never be cached. Now:
- The save key ends in `-<run_id>-<run_attempt>`, so every save gets its own entry and a partial
  cache can't block later saves.
- Restore uses the prefixes `vcpkg-<os>-<triplet>-<vcpkg>-<core>-`, then
  `vcpkg-<os>-<triplet>-<vcpkg>-`, so it picks the newest matching entry. Run 1's entry matches
  the second prefix, so run 2 starts with its 16 archives.
- The new step *Count restored vcpkg archives* records the restored count. *Check vcpkg binary
  cache* runs whenever the run was not cancelled and saves only if the archive count went up, so
  a warm run with nothing new adds no cache entry. It treats a missing cache dir as 0.

Local check of the save decision (scripts extracted verbatim, stand-in archive dir with 3 zips):
restored 3 → `save=false`; restored 2 → `save=true`; nothing restored → `save=true`; setup never
ran → `save=false`.

**Verification:** `actionlint -shellcheck=$(command -v shellcheck) .github/workflows/*.yaml`:
exit 0.

**Files changed in this iteration:** `.github/workflows/build-plugin.yaml` (Linux apt list; vcpkg
cache restore, count, check and save in both jobs). This handoff file and `session.log`.

**Still unverified:** whatever comes after `vcpkg install` on Linux: the core compile under GCC 13,
PIC, the plugin compile under `-Werror`, the link and the packaging checks.

## CI iteration 2 (run 45141112632, commit 995831c)

- Iteration 1 fixed the vcpkg step. All 27 packages installed, the core compiled, and all four
  plugin sources (including `opengl-loader-linux.cpp`) compiled under GCC 13 with `-Werror`. This
  is the first compile proof of the Linux loader.
- The link failed on `NSEEL_HOSTSTUB_EnterMutex`/`LeaveMutex`: a cyclic dependency between the core
  and the ns-eel2 archives. The fix is a CMake `LINK_GROUP:RESCAN` in `plugin/CMakeLists.txt`;
  details are in `output-P4-004.md` *CI iteration 2*. The workflow did not change in this iteration.
- **Linux libobs (C-009), from the job's *Record libobs version* step:** `obs-studio
  32.2.0-0obsproject1~noble` from the PPA, libobs API version `32.2.0`, linked as
  `/usr/lib/x86_64-linux-gnu/libobs.so.30`. `obs_module_ver` returns the API version the plugin was
  built against, so **the Linux `.deb` needs OBS 32.2.0 or newer**, not the OBS 31 floor that macOS
  has (D6). This feeds P4-007 (README) and `docs/concerns.md`.
- Next unverified steps: the link itself, then the export, `TEXTREL`, `ldd`, `dlopen`, `.deb` and
  packaged-export checks.

## CI iteration 3 (run 45142685335, commit 64c5010)

**Result:** macOS passed. On Linux, *Record libobs version* failed:
`awk: fatal: cannot open file '/usr/include/obs/obs-config.h'`.

**Cause (from comparing run 2 and run 3):** the PPA was down, not moved. In run 3,
`apt-get update` got `503 Service Unavailable` from
`ppa.launchpadcontent.net/obsproject/obs-studio/ubuntu/dists/noble/InRelease`, twice, once from
`add-apt-repository` and once from our own update. apt only warns about that ("Some index files
failed to download. They have been ignored"), so the install quietly took **Ubuntu's own
`obs-studio 30.0.2+dfsg-3build1` plus `libobs0t64`** from noble/universe. Ubuntu ships the libobs
headers in a separate `libobs-dev`, so there was no `obs-config.h`. Run 2 had fetched the PPA's
`obs-studio 32.2.0-0obsproject1~noble`, which bundles the headers and CMake config under
`/usr/include/obs`. The PPA package itself did not change between the runs.

If the version check hadn't failed first, the build would have hit the same problem at
`find_package(libobs)`, or worse, built against distro OBS 30.0.2 and produced a `.deb` with a
different OBS floor.

**Fix (`.github/workflows/build-plugin.yaml`, Linux job):**
- *Install build dependencies*:
  - `add-apt-repository --yes --no-update`, then `apt-get update --error-on=any`, retried up to 5
    times with growing waits (20, 40, 60, 80 s), so a temporary 503 is retried and a lasting one
    fails the job.
  - Before installing, `apt-cache policy obs-studio` is printed and the candidate must contain
    `obsproject`. Otherwise the job fails with "refusing to build against the distro OBS".
- *Record libobs version*:
  - The installed `obs-studio` version must contain `obsproject`.
  - The header is found through the package database (`dpkg-query -S '*/obs-config.h'`) instead
    of a fixed path. If no package provides it, the job fails, naming the problem and listing the
    installed OBS packages.
  - It fails if the `LIBOBS_API_*_VER` lines are missing.
  - It logs the header path next to the package and API version.
- I did not fall back to Ubuntu's `libobs-dev`. C-009 picked the PPA as the Linux libobs source,
  and a silent fallback would quietly change the Linux OBS floor. `plugin/cmake` is unchanged; the
  template's `find_package(libobs)` path is the PPA package's, as in run 2.

**Local checks:**
- actionlint with shellcheck: exit 0. The install script parses (`bash -n`).
- With stand-in `dpkg-query` and `apt-cache`:
  - the PPA package gives `libobs API version: 32.2.0`, exit 0;
  - the distro package fails with `installed obs-studio 30.0.2+dfsg-3build1 is not from ppa:obsproject/obs-studio`;
  - a missing header fails with `no installed package provides obs-config.h`;
  - a distro candidate fails the pre-install guard.

**C-009 note:** the Linux OBS floor is whatever the PPA has when the job runs. It was 32.2.0 in
run 2 (libobs API 32.2.0, `libobs.so.30`). Because the PPA can move between runs, the job records it
on every run (log and run summary). The README and release notes should name the floor from the run
that built the shipped `.deb`. The PPA only keeps its current version, so a rebuild later may raise
the floor.

**Files changed in this iteration:** `.github/workflows/build-plugin.yaml` (Linux *Install build
dependencies* and *Record libobs version*). This handoff file and `session.log`.

**Still unverified:** everything from the link onward (the iteration 2 `LINK_GROUP` fix has not had
a CI run yet).

## CI iteration 4 (run 45143470392, commit 61107e0)

**Result:** macOS passed. On Linux, the iteration 3 update retry worked: 503 on attempts 1 and 2,
the PPA `InRelease` fetched on attempt 3, and the candidate passed the PPA-origin check. Then
`apt-get install` failed fetching the package itself:
`obs-studio_32.2.0-0obsproject1~noble_amd64.deb 503 Service Unavailable`, exit 100. Launchpad's
PPA CDN is returning 503s on and off, for both index and pool files.

**Fix (`.github/workflows/build-plugin.yaml`, Linux *Install build dependencies*, smallest
change):**
- `Acquire::Retries "5"` is written to `/etc/apt/apt.conf.d/80-retries` first, so every apt fetch
  in the job (update and install) retries a failed file up to 5 times with apt's own backoff.
- The package list moved into a bash array, and `apt-get install` sits in a bounded loop of 3
  attempts with 30 s and 60 s waits. After the third failure it stops with
  `::error::apt-get install failed 3 times; …`.
- The update loop (5 attempts, `--error-on=any`), the PPA-candidate check before install, and the
  PPA-origin and header checks in *Record libobs version* are unchanged.

**Local checks:**
- actionlint with shellcheck: exit 0.
- The install loop, extracted verbatim, with a stand-in `sudo` that fails the first N calls (and a
  no-op `sleep`): N=2 installs on attempt 3 with all 35 arguments passed (exit 0); N=5 stops after 3
  attempts with the error line (exit 1).

**Files changed in this iteration:** `.github/workflows/build-plugin.yaml`. This handoff file and
`session.log`.

**Still unverified:** everything from the link onward (the iteration 2 `LINK_GROUP` fix and all
binary and package checks). A long Launchpad outage can still fail the job. It now fails with a
clear message, and the fix is to re-run.
