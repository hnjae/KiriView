#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later

set -euo pipefail

readonly domain="kiriview"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
pot_file="$repo_root/po/$domain.pot"
readonly pot_file

mode="update"
if [[ ${1:-} == "--check" ]]; then
    mode="check"
elif [[ ${1:-} != "" ]]; then
    printf 'Usage: %s [--check]\n' "$0" >&2
    exit 2
fi

tmp_dir="$(mktemp -d)"
cleanup() {
    rm -rf "$tmp_dir"
}
trap cleanup EXIT

generated_pot="$tmp_dir/$domain.pot"
mkdir -p "$repo_root/po" "$tmp_dir/src/qml"

mapfile -d "" cpp_files < <(
    cd "$repo_root"
    find src -type f \( -name "*.cpp" -o -name "*.h" \) -print0 | sort -z
)

mapfile -d "" qml_files < <(
    cd "$repo_root"
    find src/qml -type f -name "*.qml" -print0 | sort -z
)

qml_temp_files=()
for qml_file in "${qml_files[@]}"; do
    temp_file="$tmp_dir/$qml_file"
    mkdir -p "$(dirname "$temp_file")"
    sed 's/KI18n\.//g' "$repo_root/$qml_file" >"$temp_file"
    qml_temp_files+=("$qml_file")
done

xgettext_common=(
    --from-code=UTF-8
    --force-po
    --foreign-user
    --package-name=KiriView
    --package-version=0.1.0
    --copyright-holder="KIM Hyunjae"
    --msgid-bugs-address=https://github.com/hnjae/kiriview/issues
    --add-location=file
)

xgettext \
    "${xgettext_common[@]}" \
    --language=C++ \
    --kde \
    --keyword=i18n \
    --keyword=i18nc:1c,2 \
    --keyword=ki18n \
    --keyword=ki18nc:1c,2 \
    --keyword=kli18n \
    --keyword=kli18nc:1c,2 \
    --keyword=imageViewText \
    --output="$generated_pot" \
    --directory="$repo_root" \
    "${cpp_files[@]}"

xgettext \
    "${xgettext_common[@]}" \
    --join-existing \
    --language=JavaScript \
    --keyword=i18n \
    --keyword=i18nc:1c,2 \
    --keyword=xi18n \
    --keyword=xi18nc:1c,2 \
    --output="$generated_pot" \
    --directory="$tmp_dir" \
    "${qml_temp_files[@]}"

xgettext \
    "${xgettext_common[@]}" \
    --join-existing \
    --language=Desktop \
    --keyword=Name \
    --keyword=GenericName \
    --output="$generated_pot" \
    --directory="$repo_root" \
    io.github.hnjae.KiriView.desktop

sed -i \
    's/^"POT-Creation-Date: .*\\n"$/"POT-Creation-Date: 2026-01-01 00:00+0000\\n"/' \
    "$generated_pot"

spdx_pot="$tmp_dir/spdx-$domain.pot"
{
    printf '# SPDX-FileCopyrightText%s 2026 KIM Hyunjae\n' ':'
    printf '# SPDX-License-Identifier%s AGPL-3.0-or-later\n' ':'
    cat "$generated_pot"
} >"$spdx_pot"
mv "$spdx_pot" "$generated_pot"

if [[ $mode == "check" ]]; then
    if ! diff -u "$pot_file" "$generated_pot"; then
        printf 'po/%s.pot is out of date; run just i18n-update.\n' "$domain" >&2
        exit 1
    fi

    shopt -s nullglob
    for po_file in "$repo_root"/po/*/"$domain.po"; do
        temp_po="$tmp_dir/$(basename "$(dirname "$po_file")").po"
        cp "$po_file" "$temp_po"
        msgmerge --quiet --update --backup=off "$temp_po" "$generated_pot"
        if ! diff -u "$po_file" "$temp_po"; then
            printf '%s is out of date; run just i18n-update.\n' "$po_file" >&2
            exit 1
        fi
        msgfmt --check --check-format --output-file=/dev/null "$po_file"
    done
    exit 0
fi

cp "$generated_pot" "$pot_file"

shopt -s nullglob
for po_file in "$repo_root"/po/*/"$domain.po"; do
    msgmerge --quiet --update --backup=off "$po_file" "$pot_file"
done
