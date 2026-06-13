#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later

set -euo pipefail

readonly domain="kiriview"
readonly desktop_id="org.hnjae.kiriview"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root

prefix="${1:-/app}"
build_dir="${2:-$repo_root/target/i18n}"
desktop_output="$build_dir/$desktop_id.desktop"

mkdir -p "$build_dir"
cp "$repo_root/$desktop_id.desktop" "$desktop_output"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

po_with_default_header() {
    local po_file="$1"
    local output_file="$2"
    local language="$3"

    if grep -qx 'msgid ""' "$po_file"; then
        cp "$po_file" "$output_file"
        return
    fi

    {
        printf 'msgid ""\n'
        printf 'msgstr ""\n'
        printf '"Project-Id-Version: KiriView 0.1.0\\n"\n'
        printf '"Report-Msgid-Bugs-To: https://github.com/hnjae/kiriview/issues\\n"\n'
        printf '"PO-Revision-Date: 2026-01-01 00:00+0000\\n"\n'
        printf '"Last-Translator: KiriView contributors\\n"\n'
        printf '"Language-Team: \\n"\n'
        printf '"Language: %s\\n"\n' "$language"
        printf '"MIME-Version: 1.0\\n"\n'
        printf '"Content-Type: text/plain; charset=UTF-8\\n"\n'
        printf '"Content-Transfer-Encoding: 8bit\\n"\n'
        printf '\n'
        cat "$po_file"
    } >"$output_file"
}

shopt -s nullglob
for po_file in "$repo_root"/po/*/"$domain.po"; do
    lang="$(basename "$(dirname "$po_file")")"
    locale_dir="$prefix/share/locale/$lang/LC_MESSAGES"
    compile_po="$tmp_dir/$lang.po"

    po_with_default_header "$po_file" "$compile_po" "$lang"
    install -d "$locale_dir"
    msgfmt --output-file="$locale_dir/$domain.mo" "$compile_po"

    translated_desktop="$build_dir/$desktop_id.$lang.desktop"
    msgfmt \
        --desktop \
        --locale="$lang" \
        --keyword=Name \
        --keyword=GenericName \
        --template="$desktop_output" \
        --output-file="$translated_desktop" \
        "$compile_po"
    mv "$translated_desktop" "$desktop_output"
done
