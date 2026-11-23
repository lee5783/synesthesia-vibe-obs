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
