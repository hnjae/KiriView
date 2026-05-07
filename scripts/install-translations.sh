#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later

set -euo pipefail

readonly domain="kiriview"
readonly desktop_id="io.github.hnjae.KiriView"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root

prefix="${1:-/app}"
build_dir="${2:-$repo_root/target/i18n}"
desktop_output="$build_dir/$desktop_id.desktop"

mkdir -p "$build_dir"
cp "$repo_root/$desktop_id.desktop" "$desktop_output"

shopt -s nullglob
for po_file in "$repo_root"/po/*/"$domain.po"; do
    lang="$(basename "$(dirname "$po_file")")"
    locale_dir="$prefix/share/locale/$lang/LC_MESSAGES"

    install -d "$locale_dir"
    msgfmt --output-file="$locale_dir/$domain.mo" "$po_file"

    translated_desktop="$build_dir/$desktop_id.$lang.desktop"
    msgfmt \
        --desktop \
        --locale="$lang" \
        --keyword=Name \
        --keyword=GenericName \
        --template="$desktop_output" \
        --output-file="$translated_desktop" \
        "$po_file"
    mv "$translated_desktop" "$desktop_output"
done
