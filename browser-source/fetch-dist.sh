#!/usr/bin/env bash
#
#  fetch-dist.sh
#  SynesthesiaVibeOBS
#
#  Created by lee5783 on 2026-11-23.
#

set -euo pipefail

readonly PINNED_VERSION="0.7.0"
readonly DEFAULT_BASE_URL="https://github.com/lee5783/synesthesia-vibe/releases/download"
readonly DEFAULT_ARCHIVE_URL="https://github.com/lee5783/synesthesia-vibe/archive/refs/tags"
readonly SUMS_FILE="SHA256SUMS.txt"
readonly PRESETS_SOURCE="resources/mvp-presets"
readonly TEXTURES_SOURCE="resources/beatdrop/textures"

die() {
    echo "fetch-dist: $*" >&2
    exit 1
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
dist_dir="$script_dir/dist"
work_dir=""
stage_dir=""
old_dir=""

cleanup() {
    if [ -n "$work_dir" ]; then rm -rf "$work_dir"; fi
    if [ -n "$stage_dir" ]; then rm -rf "$stage_dir"; fi
    if [ -n "$old_dir" ]; then
        if [ ! -e "$dist_dir" ] && [ -d "$old_dir/dist" ]; then
            mv "$old_dir/dist" "$dist_dir"
        fi
        rm -rf "$old_dir"
    fi
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM

normalise_version() {
    local version="${1#v}"
    case "$version" in
        "" | *[!0-9A-Za-z.-]* | .* | -*) die "invalid version '$1' (expected e.g. 0.7.0 or v0.7.0)" ;;
    esac
    echo "$version"
}

require_tools() {
    local tool
    for tool in curl unzip tar python3; do
        command -v "$tool" >/dev/null 2>&1 || die "missing required tool: $tool"
    done
    if ! command -v shasum >/dev/null 2>&1 && ! command -v sha256sum >/dev/null 2>&1; then
        die "missing required tool: shasum or sha256sum"
    fi
}

download() {
    local url="$1"
    local output="$2"
    local status
    if ! status="$(curl -fsL -o "$output" -w '%{http_code}' "$url")"; then
        case "$status" in
            "" | 000) die "download failed: $url" ;;
            404) die "not found (HTTP 404), check that release v$version exists: $url" ;;
            *) die "download failed (HTTP $status): $url" ;;
        esac
    fi
}

verify_checksum() {
    local asset="$1"
    local line_file="$work_dir/$asset.sha256"
    awk -v name="$asset" '$2 == name || $2 == "*" name { print $1 "  " name; exit }' \
        "$work_dir/$SUMS_FILE" >"$line_file"
    [ -s "$line_file" ] || die "$SUMS_FILE has no entry for $asset"
    if command -v shasum >/dev/null 2>&1; then
        (cd "$work_dir" && shasum -a 256 -c "$line_file" >/dev/null 2>&1) \
            || die "checksum mismatch for $asset"
    else
        (cd "$work_dir" && sha256sum -c "$line_file" >/dev/null 2>&1) \
            || die "checksum mismatch for $asset"
    fi
}

stage_dist() {
    local archive="$1"
    stage_dir="$(mktemp -d "$script_dir/.dist-stage.XXXXXX")"
    unzip -q "$archive" -d "$stage_dir" || die "could not unpack $(basename "$archive")"
    [ -n "$(ls -A "$stage_dir")" ] || die "$(basename "$archive") is empty"
    chmod 755 "$stage_dir"
}

copy_files() {
    local source="$1"
    local suffix="$2"
    local target="$3"
    local file
    mkdir -p "$target"
    for file in "$source"/*"$suffix"; do
        if [ -f "$file" ] && [ ! -L "$file" ]; then
            cp "$file" "$target/"
        fi
    done
}

stage_resources() {
    local archive="$1"
    local name
    local root
    local notice
    local source_dir="$work_dir/source"
    name="$(basename "$archive")"
    tar -tzf "$archive" >"$work_dir/source.list" 2>/dev/null || die "could not read $name"
    root="$(sed -n '1p' "$work_dir/source.list")"
    root="${root%%/*}"
    case "$root" in
        "" | *[!0-9A-Za-z._-]* | .*) die "unexpected layout in $name" ;;
    esac
    mkdir "$source_dir"
    tar -xzf "$archive" -C "$source_dir" \
        "$root/LICENSE" "$root/NOTICE" "$root/$PRESETS_SOURCE" "$root/$TEXTURES_SOURCE" 2>/dev/null \
        || die "$name is missing LICENSE, NOTICE, $PRESETS_SOURCE or $TEXTURES_SOURCE"
    for notice in LICENSE NOTICE; do
        [ -f "$source_dir/$root/$notice" ] && [ ! -L "$source_dir/$root/$notice" ] \
            || die "$name has no regular $notice file"
        cp "$source_dir/$root/$notice" "$stage_dir/$notice"
    done
    copy_files "$source_dir/$root/$PRESETS_SOURCE" ".milk" "$stage_dir/resources/presets"
    copy_files "$source_dir/$root/$TEXTURES_SOURCE" "" "$stage_dir/resources/textures"
    [ -n "$(ls -A "$stage_dir/resources/presets")" ] || die "$name has no presets in $PRESETS_SOURCE"
}

write_manifest() {
    python3 -I - "$stage_dir/resources" <<'PYTHON' || die "could not write resources/manifest.json"
import json
import os
import sys

root = sys.argv[1]

def listing(category, suffix):
    directory = os.path.join(root, category)
    names = sorted(
        name for name in os.listdir(directory)
        if name.endswith(suffix) and not name.startswith(".")
        and os.path.isfile(os.path.join(directory, name))
    )
    return [category + "/" + name for name in names]

manifest = {
    "presets": listing("presets", ".milk"),
    "textures": listing("textures", ""),
    "audio": [],
}
with open(os.path.join(root, "manifest.json"), "w", encoding="utf-8") as output:
    json.dump(manifest, output, indent=2, ensure_ascii=False)
    output.write("\n")
PYTHON
}

swap_dist() {
    if [ -e "$dist_dir" ]; then
        old_dir="$(mktemp -d "$script_dir/.dist-old.XXXXXX")"
        mv "$dist_dir" "$old_dir/dist"
    fi
    mv "$stage_dir" "$dist_dir"
    stage_dir=""
}

[ "$#" -le 1 ] || die "usage: fetch-dist.sh [version]"

version="$(normalise_version "${1:-$PINNED_VERSION}")"
base_url="${FETCH_DIST_BASE_URL:-$DEFAULT_BASE_URL}"
archive_url="${FETCH_DIST_ARCHIVE_URL:-$DEFAULT_ARCHIVE_URL}"
asset="synesthesia-vibe-web-$version.zip"
source_archive="v$version.tar.gz"
release_url="$base_url/v$version"

require_tools
work_dir="$(mktemp -d "${TMPDIR:-/tmp}/fetch-dist.XXXXXX")"

download "$release_url/$SUMS_FILE" "$work_dir/$SUMS_FILE"
download "$release_url/$asset" "$work_dir/$asset"
verify_checksum "$asset"
download "$archive_url/$source_archive" "$work_dir/$source_archive"
stage_dist "$work_dir/$asset"
file_count="$(find "$stage_dir" -type f | wc -l | tr -d ' ')"
stage_resources "$work_dir/$source_archive"
write_manifest
preset_count="$(find "$stage_dir/resources/presets" -type f | wc -l | tr -d ' ')"
texture_count="$(find "$stage_dir/resources/textures" -type f | wc -l | tr -d ' ')"
swap_dist

echo "Installed synesthesia-vibe web dist $version ($file_count files, $preset_count presets," \
    "$texture_count textures) into browser-source/dist"
