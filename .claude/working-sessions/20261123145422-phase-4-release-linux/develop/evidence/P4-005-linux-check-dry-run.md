# P4-005 evidence: Linux job checks, dry run with stand-in tools

**This is not a Linux build.** There is no Linux host or container here, so the CI `Linux x86_64`
job is the first real compile, link and package of the Linux port. What was checked locally is that
the job's check scripts parse, and that their pass/fail logic does what it should against
representative tool output.

## Scripts extracted verbatim

```
$ python3 P4-002-workflow-step.py .github/workflows/build-plugin.yaml linux "Check plugin binary" > check-linux.sh
$ bash -n check-linux.sh
bash -n ok
$ (python heredoc between <<'PROBE' and PROBE, parsed with ast.parse)
probe python parses
```

## *Check plugin binary* with stand-in `nm`, `readelf`, `ldd`, `file`, `python3`

The stand-in `nm -D --defined-only` printed the 7 `obs_module_*` names plus, per mode, nothing
(clean), the linker-reserved `_init` and `__bss_start` (reserved), or a leaked `gladLoadGL` (leak).

```
mode=clean    exit=0
mode=reserved exit=0
mode=leak     exit=1   ::error::the plugin exports symbols beyond the 7 obs_module_* entry points
```

## *Inspect package* with a stand-in `dpkg-deb`

The stand-in printed a `dpkg-deb -c` listing in the real format
(`-rw-r--r-- root/root 2345678 2026-11-23 10:00 ./usr/lib/x86_64-linux-gnu/obs-plugins/synesthesia-vibe-obs.so`
and `./usr/share/obs/obs-plugins/synesthesia-vibe-obs/locale/en-US.ini`), a Maintainer and a
Depends line:

```
exit=0
Maintainer: lee5783 <lee5783@users.noreply.github.com>
Depends: libc6 (>= 2.34), libegl1, libopengl0, libstdc++6 (>= 13.1), obs-studio (>= 32.0.0)
```

(The Depends value is invented for the dry run; the real one comes from `dpkg-shlibdeps`.)

## What the real job checks (fails the job on any miss)

- exactly the 7 `obs_module_*` names of `plugin/exported-symbols.map` in `nm -D --defined-only`, no
  other defined dynamic symbol except the linker-reserved `_init`, `_fini`, `_edata`, `_end`,
  `__bss_start`, and no `glad`, `sv_`, projectM, ns-eel or `_Z` (C++) names;
- no `TEXTREL` in `readelf -d` (non-PIC code linked in);
- no `not found` in `ldd`;
- a `dlopen` probe in Python ctypes with `RTLD_NOW | RTLD_LOCAL` that resolves `obs_module_load`
  and three other entry points (stricter than the plan's `RTLD_LAZY`: every function import must
  bind at load);
- `cmake --build build_x86_64 --target package` produces
  `release/synesthesia-vibe-obs-<ver>-x86_64-linux-gnu.deb`;
- `dpkg-deb -I` printed; Maintainer non-empty; Depends names the OBS package;
- `dpkg-deb -c` has the `.so` under `/usr/lib/x86_64-linux-gnu/obs-plugins/` and `en-US.ini` under
  `/usr/share/obs/obs-plugins/synesthesia-vibe-obs/locale/`, and no presets or textures;
- the `.so` extracted from the `.deb` still exports the 7 names;
- the link itself uses `-Wl,--no-undefined`, so a missing library fails at link time.

The libobs version the job built against goes to the job log and the run summary (*Record libobs
version*, C-009).

## Unverified until CI runs

Everything Linux: the core compiling with GCC 13 (C-013), PIC on the core objects (C-015), the
vcpkg `glfw3` system packages (C-014), the PPA libobs headers and CMake config, the
`dpkg-shlibdeps` mapping of libobs to a package, and the GLVND link.

## CI result (run 45144284376, commit ec49eeb)

After the CI iterations in `output-P4-005.md`, run 5 was green on both jobs, and every Linux check
ran for real (log excerpts):

```
obs-studio package: obs-studio 32.2.0-0obsproject1~noble
libobs headers: /usr/include/obs/obs-config.h
libobs API version: 32.2.0
build_x86_64/synesthesia-vibe-obs.so: ELF 64-bit LSB shared object, x86-64, version 1 (SYSV), dynamically linked, …, with debug_info, not stripped
##[group]Exports
obs_module_free_locale
obs_module_get_string
obs_module_load
obs_module_set_locale
obs_module_set_pointer
obs_module_unload
obs_module_ver
 (NEEDED) Shared library: [libobs.so.30]
 (NEEDED) Shared library: [libOpenGL.so.0]
 (NEEDED) Shared library: [libEGL.so.1]
 (NEEDED) Shared library: [libstdc++.so.6] [libm.so.6] [libgcc_s.so.1] [libc.so.6] [ld-linux-x86-64.so.2]
	libobs.so.30 => /lib/x86_64-linux-gnu/libobs.so.30
	libOpenGL.so.0 => /lib/x86_64-linux-gnu/libOpenGL.so.0
	libEGL.so.1 => /lib/x86_64-linux-gnu/libEGL.so.1
	(… libobs's own dependencies, none "not found")
dlopen probe: RTLD_NOW | RTLD_LOCAL load succeeded and obs_module_load resolved
CPack: - package: …/plugin/release/synesthesia-vibe-obs-0.1.0-x86_64-linux-gnu.deb generated.
 Package: synesthesia-vibe-obs
 Version: 0.1.0
 Architecture: amd64
 Maintainer: lee5783 <lee5783@users.noreply.github.com>
 Depends: libc6 (>= 2.38), libegl1, libgcc-s1 (>= 3.0), libopengl0, libstdc++6 (>= 13.1), obs-studio
-rw-r--r-- root/root   1470288 … ./usr/lib/x86_64-linux-gnu/obs-plugins/synesthesia-vibe-obs.so
-rw-r--r-- root/root      1090 … ./usr/share/obs/obs-plugins/synesthesia-vibe-obs/locale/en-US.ini
##[group]Packaged binary exports
(the same 7 obs_module_* names)
```

No `TEXTREL`, and no defined dynamic symbol beyond the 7. Artifact
`synesthesia-vibe-obs-0.1.0-x86_64-linux-gnu`, 447203 B. The dry-run logic above matched the real
output; the linker-reserved allow-list was not needed, since none of those names appeared.
