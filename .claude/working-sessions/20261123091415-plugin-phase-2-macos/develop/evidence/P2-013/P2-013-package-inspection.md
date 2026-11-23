# P2-013 package inspection

Run from `plugin/build_macos_arm64/package-staging` after the Release build and the staged `cmake --install`. Paths sanitized (`<repo>`, `<scratch>`, `<user>`).

```
$ ls -l (package-staging)
total 2512
-rw-r--r--@ 1 <user> staff 496298 Nov 23 12:36 synesthesia-vibe-obs-0.1.0-macos-arm64.zip
-rw-r--r--@ 1 <user> staff 785522 Nov 23 12:35 synesthesia-vibe-obs.pkg
drwxr-xr-x@ 3 <user> staff 96 Nov 23 12:35 synesthesia-vibe-obs.plugin
drwxr-xr-x@ 3 <user> staff 96 Nov 23 12:35 synesthesia-vibe-obs.plugin.dsym

$ shasum -a 256 *.pkg *.zip
1dbf3ab24811451344351a0d31c9ffb274510f1c6fd6c15c6a5a2da2df187889  synesthesia-vibe-obs.pkg
10344f23c9ca252ef4aa2f78a960c4968c2437db3aa253a28795ab863f9a88d3  synesthesia-vibe-obs-0.1.0-macos-arm64.zip

$ pkgutil --check-signature synesthesia-vibe-obs.pkg
Package "synesthesia-vibe-obs.pkg":
   Status: no signature
(exit 1)

$ pkgutil --payload-files synesthesia-vibe-obs.pkg
.
./Library
./Library/Application Support
./Library/Application Support/obs-studio
./Library/Application Support/obs-studio/plugins
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/Relocations
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/Relocations/aarch64
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/Relocations/aarch64/synesthesia-vibe-obs.yml
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/Relocations/aarch64/._synesthesia-vibe-obs.yml
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/Relocations/._aarch64
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/._Relocations
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/DWARF
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/DWARF/synesthesia-vibe-obs
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/DWARF/._synesthesia-vibe-obs
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/._DWARF
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/._Resources
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Info.plist
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/._Info.plist
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/._Contents
./Library/Application Support/obs-studio/plugins/._synesthesia-vibe-obs.plugin.dSYM
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/_CodeSignature
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/_CodeSignature/CodeResources
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/_CodeSignature/._CodeResources
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/.__CodeSignature
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/MacOS
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/MacOS/._synesthesia-vibe-obs
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/._MacOS
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/Resources
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/Resources/locale
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/Resources/locale/en-US.ini
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/Resources/locale/._en-US.ini
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/Resources/._locale
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/._Resources
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/Info.plist
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/._Info.plist
./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/._Contents
./Library/Application Support/obs-studio/plugins/._synesthesia-vibe-obs.plugin
./Library/Application Support/obs-studio/._plugins
./Library/Application Support/._obs-studio
./Library/._Application Support
./._Library

$ pkgutil --payload-files synesthesia-vibe-obs.pkg | grep -i -E "presets|textures|\.milk" || echo none
none

$ pkgutil --expand synesthesia-vibe-obs.pkg <scratch>/pkg-expanded; ls
Distribution
synesthesia-vibe-obs.pkg

$ cat <scratch>/pkg-expanded/Distribution
<?xml version="1.0" encoding="utf-8"?>
<!--
    distribution.in
    SynesthesiaVibeOBS

    Created by lee5783 on 2026-11-23.
-->
<installer-gui-script minSpecVersion="1">
    <options rootVolumeOnly="true" hostArchitectures="arm64" customize="never" allow-external-scripts="no"/>
    <domains enable_currentUserHome="true" enable_anywhere="false" enable_localSystem="false"/>
    <title>synesthesia-vibe-obs</title>
    <choices-outline>
        <line choice="obs-plugin"/>
    </choices-outline>
    <choice id="obs-plugin" title="synesthesia-vibe-obs" description="">
        <pkg-ref id="io.github.lee5783.synesthesia-vibe-obs"/>
    </choice>
    <pkg-ref id="io.github.lee5783.synesthesia-vibe-obs" version="'0.1.0'" packageIdentifier="'io.github.lee5783.synesthesia-vibe-obs'" installKBytes="3334" updateKBytes="0">#synesthesia-vibe-obs.pkg</pkg-ref>
    <installation-check script="installCheck();"/>
    <script>
        function installCheck() {
            var macOSVersion = system.version.ProductVersion

            if (system.compareVersions(macOSVersion, '12.0') == -1) {
                my.result.title = system.localizedStandardStringWithFormat(
                                    'InstallationCheckError',
                                    system.localizedString('DISTRIBUTION_TITLE')
                                  );
                my.result.message = ' ';
                my.result.type = 'Fatal';
                return false;
            }
        }
    </script>
    <pkg-ref id="io.github.lee5783.synesthesia-vibe-obs">
        <bundle-version>
            <bundle CFBundleShortVersionString="0.1.0" CFBundleVersion="4" id="io.github.lee5783.synesthesia-vibe-obs" path="Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin"/>
            <bundle CFBundleShortVersionString="0.1.0" CFBundleVersion="4" id="com.apple.xcode.dsym.io.github.lee5783.synesthesia-vibe-obs" path="Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM"/>
        </bundle-version>
    </pkg-ref>
</installer-gui-script>
$ cat <scratch>/pkg-expanded/synesthesia-vibe-obs.pkg/PackageInfo
<?xml version="1.0" encoding="utf-8"?>
<pkg-info overwrite-permissions="true" relocatable="false" identifier="'io.github.lee5783.synesthesia-vibe-obs'" postinstall-action="none" version="'0.1.0'" format-version="2" generator-version="InstallCmds-864.12 (25G332)" auth="root">
    <payload numberOfFiles="47" installKBytes="3334"/>
    <bundle path="./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin" id="io.github.lee5783.synesthesia-vibe-obs" CFBundleShortVersionString="0.1.0" CFBundleVersion="4"/>
    <bundle path="./Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM" id="com.apple.xcode.dsym.io.github.lee5783.synesthesia-vibe-obs" CFBundleShortVersionString="0.1.0" CFBundleVersion="4"/>
    <bundle-version>
        <bundle id="io.github.lee5783.synesthesia-vibe-obs"/>
        <bundle id="com.apple.xcode.dsym.io.github.lee5783.synesthesia-vibe-obs"/>
    </bundle-version>
    <upgrade-bundle>
        <bundle id="io.github.lee5783.synesthesia-vibe-obs"/>
        <bundle id="com.apple.xcode.dsym.io.github.lee5783.synesthesia-vibe-obs"/>
    </upgrade-bundle>
    <update-bundle/>
    <atomic-update-bundle/>
    <strict-identifier/>
    <relocate/>
</pkg-info>
$ plutil -extract (Info.plist, staged .plugin)
CFBundleIdentifier = io.github.lee5783.synesthesia-vibe-obs
CFBundleShortVersionString = 0.1.0
CFBundleVersion = 4
CFBundleExecutable = synesthesia-vibe-obs
CFBundleDisplayName = synesthesia-vibe-obs

$ lipo -info synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
Non-fat file: synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs is architecture: arm64

$ codesign -dvv synesthesia-vibe-obs.plugin
Executable=<repo>/plugin/build_macos_arm64/package-staging/synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
Identifier=io.github.lee5783.synesthesia-vibe-obs
Format=bundle with Mach-O thin (arm64)
CodeDirectory v=20500 size=4103 flags=0x10002(adhoc,runtime) hashes=121+3 location=embedded
Signature=adhoc
Info.plist entries=21
TeamIdentifier=not set
Runtime Version=27.2.0
Sealed Resources version=2 rules=13 files=1
Internal requirements count=0 size=12

$ codesign --verify --deep --strict --verbose=2 synesthesia-vibe-obs.plugin
synesthesia-vibe-obs.plugin: valid on disk
synesthesia-vibe-obs.plugin: satisfies its Designated Requirement
(exit 0)

$ codesign -d --entitlements - synesthesia-vibe-obs.plugin
Executable=<repo>/plugin/build_macos_arm64/package-staging/synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs

$ nm -gU synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
0000000000003b38 T _obs_module_free_locale
0000000000003adc T _obs_module_get_string
0000000000003b60 T _obs_module_load
0000000000003af0 T _obs_module_set_locale
0000000000003a84 T _obs_module_set_pointer
0000000000003c18 T _obs_module_unload
0000000000003a9c T _obs_module_ver
count: 7

$ diff <(nm -gU ... | awk "{print \$3}" | sort) <(sort exported-symbols.txt w/o header)
identical

$ find synesthesia-vibe-obs.plugin
synesthesia-vibe-obs.plugin
synesthesia-vibe-obs.plugin/Contents
synesthesia-vibe-obs.plugin/Contents/_CodeSignature
synesthesia-vibe-obs.plugin/Contents/MacOS
synesthesia-vibe-obs.plugin/Contents/Resources
synesthesia-vibe-obs.plugin/Contents/Info.plist
synesthesia-vibe-obs.plugin/Contents/_CodeSignature/CodeResources
synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
synesthesia-vibe-obs.plugin/Contents/Resources/locale
synesthesia-vibe-obs.plugin/Contents/Resources/locale/en-US.ini

$ spctl --assess -vv --type install synesthesia-vibe-obs.pkg
synesthesia-vibe-obs.pkg: rejected
source=no usable signature
(exit 3)

$ spctl --assess -vv --type execute synesthesia-vibe-obs.plugin
synesthesia-vibe-obs.plugin: rejected
(exit 3)

$ spctl --assess -vv --type open --context context:primary-signature synesthesia-vibe-obs-0.1.0-macos-arm64.zip
synesthesia-vibe-obs-0.1.0-macos-arm64.zip: rejected
source=no usable signature
(exit 3)

$ installer -pkginfo -verbose -pkg synesthesia-vibe-obs.pkg

Package information:

Package           : synesthesia-vibe-obs
Status            : MetaPackage
Will Restart      : NO
Size              : 3334 KB
Must Authenticate : NO
Title             : synesthesia-vibe-obs
Description       : 
Message           : MetaPackage information only, it is not installed

Package           : synesthesia-vibe-obs
Status            : Install
Will Restart      : NO
Size              : 3334 KB
Must Authenticate : NO
Title             : synesthesia-vibe-obs
Description       : 
Message           : 

$ installer -dominfo -pkg synesthesia-vibe-obs.pkg
CurrentUserHomeDirectory

$ pkgutil --expand-full synesthesia-vibe-obs.pkg <scratch>/full; find <Payload>/Library -print (extracted as Installer would; AppleDouble entries merge back into xattrs)
Library
Library/Application Support
Library/Application Support/obs-studio
Library/Application Support/obs-studio/plugins
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Info.plist
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/_CodeSignature
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/MacOS
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/Resources
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/Info.plist
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/Relocations
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/DWARF
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/_CodeSignature/CodeResources
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/Resources/locale
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/Relocations/aarch64
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/DWARF/synesthesia-vibe-obs
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin/Contents/Resources/locale/en-US.ini
Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM/Contents/Resources/Relocations/aarch64/synesthesia-vibe-obs.yml

$ codesign --verify --deep --strict --verbose=2 <extracted payload .plugin>
synesthesia-vibe-obs.plugin: valid on disk
synesthesia-vibe-obs.plugin: satisfies its Designated Requirement
Non-fat file: synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs is architecture: arm64

$ unzip -q synesthesia-vibe-obs-0.1.0-macos-arm64.zip -d <scratch>/unzip-test; codesign --verify --deep --strict --verbose=2
synesthesia-vibe-obs.plugin: valid on disk
synesthesia-vibe-obs.plugin: satisfies its Designated Requirement
```
