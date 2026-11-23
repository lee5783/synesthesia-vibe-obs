# REVIEW-FIX evidence: README S1, S2, N4

All checks ran against the P2-013 artifacts in `plugin/build_macos_arm64/package-staging/`
(git-ignored). The pkg was expanded into the session scratchpad. Nothing was installed,
`pkgutil --forget` was not run, and OBS was not touched.

## The pkg is the P2-013 build

```
shasum -a 256 synesthesia-vibe-obs.pkg
1dbf3ab24811451344351a0d31c9ffb274510f1c6fd6c15c6a5a2da2df187889  synesthesia-vibe-obs.pkg
```

That matches the SHA-256 recorded in `output-P2-013.md`.

## S1: the pkg installs the dSYM next to the plugin

`pkgutil --payload-files synesthesia-vibe-obs.pkg`, without the `._*` AppleDouble entries:

```
./Library/Application Support/obs-studio/plugins
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/...
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/...
```

`PackageInfo` lists both bundles: `synesthesia-vibe-obs.plugin` (`io.github.lee5783.synesthesia-vibe-obs`)
and `synesthesia-vibe-obs.plugin.dSYM` (`com.apple.xcode.dsym.io.github.lee5783.synesthesia-vibe-obs`).

The release zip holds only `synesthesia-vibe-obs.plugin` (`unzip -l`, top-level entries), so the
README's Option B text stays correct and the dSYM line in Uninstall is a no-op after a zip install.

## S2: the receipt id includes literal single quotes

`pkgutil --expand synesthesia-vibe-obs.pkg <scratch>/review-fix-expand`, then `PackageInfo`:

```
<pkg-info ... identifier="'io.github.lee5783.synesthesia-vibe-obs'" ... version="'0.1.0'" ...>
```

`Distribution`:

```
<pkg-ref id="io.github.lee5783.synesthesia-vibe-obs" version="'0.1.0'" packageIdentifier="'io.github.lee5783.synesthesia-vibe-obs'" ...>
```

The `packageIdentifier` (the receipt id) carries the single quotes; the `pkg-ref id` does not.

### Shell quoting

The argument `pkgutil` receives, with `pkgutil` replaced by a function that prints its argv.

Pasting the `pkgutil --pkgs` line as it prints (`pkgutil --forget 'io.github...'`):

```
argv: [--forget]
argv: [io.github.lee5783.synesthesia-vibe-obs]        <- quotes stripped, no receipt matches
```

The README line, extracted verbatim from `plugin/README.md` and sourced in zsh and in bash:

```
pkgutil --forget "'io.github.lee5783.synesthesia-vibe-obs'"
argv: [--forget]
argv: ['io.github.lee5783.synesthesia-vibe-obs']      <- zsh
argv: [--forget]
argv: ['io.github.lee5783.synesthesia-vibe-obs']      <- bash
```

Plain `printf` check (bash): `printf '%s\n' "'io.github.lee5783.synesthesia-vibe-obs'"` prints
`'io.github.lee5783.synesthesia-vibe-obs'`.

Read-only `pkgutil --pkgs | grep -i synesthesia` on this machine: no match (the pkg was never
installed here), so the live `pkgutil --pkgs` output itself is not shown.

## Uninstall `rm -rf` lines, dry run in a fake HOME

The two `rm -rf` lines were extracted verbatim from `plugin/README.md` and run with `HOME` set to a
scratch folder that held `synesthesia-vibe-obs.plugin`, `synesthesia-vibe-obs.plugin.dSYM` and an
unrelated `other.plugin`:

```
--- before:
other.plugin
synesthesia-vibe-obs.plugin
synesthesia-vibe-obs.plugin.dSYM
rm exit=0
--- after:
other.plugin
--- rm lines again on zip-style install (no dSYM):
exit=0
```

## N4

The Blend time row now says the blend is skipped for the first preset of a new source and for the
next change after a `preset failed to commit` error, per the review's reading of
`visualizer-source.cpp:340-342` and `:887-888` (a failed commit clears `preset_target`). Not
re-derived from source in this pass; the code was not changed.
