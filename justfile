#!/usr/bin/env -S just --justfile

set unstable
set lazy

export DEVENV_TUI := "false"

_:
    @just --list

[group('ci')]
lint:
    devenv shell -- lint

alias test-host := test

[group('ci')]
test:
    devenv shell -- test-host

[group('ci')]
format:
    devenv shell -- prek run --hook-stage pre-commit --all-files

[group('ci')]
format-check:
    cargo fmt --all --check

[group('ci')]
i18n-check:
    devenv shell -- scripts/update-translations.sh --check

[group('build')]
i18n-update:
    devenv shell -- scripts/update-translations.sh

[group('build')]
build:
    devenv shell -- flatpak-builder \
        --user \
        --install-deps-from=flathub \
        --ccache \
        --keep-build-dirs \
        --disable-tests \
        --force-clean build-dir \
        io.github.hnjae.KiriView.json

[group('build')]
build-with-tests:
    devenv shell -- flatpak-builder \
        --user \
        --install-deps-from=flathub \
        --ccache \
        --keep-build-dirs \
        --force-clean build-dir \
        io.github.hnjae.KiriView.json

[group('build')]
run:
    flatpak build \
        --socket=wayland \
        --device=dri \
        --talk-name=org.freedesktop.portal.Desktop \
        --filesystem=home:ro \
        --filesystem=/media:ro \
        --filesystem=/mnt:ro \
        --filesystem=/run/media:ro \
        --filesystem=xdg-run/gvfs:ro \
        --bind-mount="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/doc=${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/doc" \
        --env=LIBHEIF_PLUGIN_PATH=/app/lib/libheif \
        --env=WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}" \
        --env=QT_QPA_PLATFORM=wayland \
        build-dir \
        kiriview

[group('build')]
install:
    devenv shell -- flatpak-builder \
        --user \
        --install \
        --install-deps-from=flathub \
        --ccache \
        --keep-build-dirs \
        --disable-tests \
        --force-clean build-dir \
        io.github.hnjae.KiriView.json
