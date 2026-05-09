#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later

set -euo pipefail

readonly domain="kiriview"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
pot_file="$repo_root/po/$domain.pot"
readonly pot_file

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

mode="update"
if [[ ${1:-} == "--check" ]]; then
    mode="check"
elif [[ ${1:-} == "--template-check" ]]; then
    mode="template-check"
elif [[ ${1:-} != "" ]]; then
    printf 'Usage: %s [--check|--template-check]\n' "$0" >&2
    exit 2
fi

if [[ $mode == "check" ]]; then
    check_tmp_dir="$(mktemp -d)"
    trap 'rm -rf "$check_tmp_dir"' EXIT

    shopt -s nullglob
    for po_file in "$repo_root"/po/*/"$domain.po"; do
        language="$(basename "$(dirname "$po_file")")"
        checked_po="$check_tmp_dir/$language.po"
        po_with_default_header "$po_file" "$checked_po" "$language"
        msgfmt --check --check-format --output-file=/dev/null "$checked_po"
    done
    exit 0
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

if [[ $mode == "template-check" ]]; then
    if ! diff -u "$pot_file" "$generated_pot"; then
        printf 'po/%s.pot is out of date; run just i18n-update.\n' "$domain" >&2
        exit 1
    fi
    exit 0
fi

cp "$generated_pot" "$pot_file"

shopt -s nullglob
for po_file in "$repo_root"/po/*/"$domain.po"; do
    language="$(basename "$(dirname "$po_file")")"
    merge_po="$tmp_dir/$language.po"

    po_with_default_header "$po_file" "$merge_po" "$language"
    msgmerge --quiet --update --backup=off "$merge_po" "$pot_file"
    cp "$merge_po" "$po_file"
done
