#!/usr/bin/env -S just --justfile

set unstable
set lazy

export DEVENV_TUI := "false"

_:
    @just --list

[no-cd]
[private]
_cargo-vendor:
    install -Dm644 '{{ justfile_directory() }}/cargo/config' '{{ justfile_directory() }}/.flatpak-cargo/.cargo/config.toml'
    devenv shell -- cargo vendor --locked --versioned-dirs --quiet '{{ justfile_directory() }}/.flatpak-cargo/vendor'

[private]
_flatpak-build-dir:
    if ! test -f '{{ justfile_directory() }}/build-dir/metadata'; then \
        just build; \
    fi

[private]
_module command: _cargo-vendor _flatpak-build-dir
    flatpak build \
        '--bind-mount=/run/build/kiriview={{ justfile_directory() }}' \
        '--bind-mount=/run/build/kiriview/.cargo={{ justfile_directory() }}/.flatpak-cargo/.cargo' \
        '--bind-mount=/run/build/kiriview/cargo/vendor={{ justfile_directory() }}/.flatpak-cargo/vendor' \
        --build-dir=/run/build/kiriview \
        --env=PATH=/usr/lib/sdk/rust-stable/bin:/app/bin:/usr/bin \
        build-dir \
        {{ command }}

[group('ci')]
lint:
    just _module 'cargo --offline clippy --all-targets --all-features -- -D warnings'

[group('ci')]
test:
    just _module 'cargo --offline test --all-targets --all-features'

[group('ci')]
format:
    devenv shell -- prek run --hook-stage pre-commit --all-files

[group('ci')]
format-check:
    cargo fmt --all --check

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
        --env=WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}" \
        --env=QT_QPA_PLATFORM=wayland \
        build-dir \
        kiriview
