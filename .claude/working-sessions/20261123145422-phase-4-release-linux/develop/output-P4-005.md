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
