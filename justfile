#!/usr/bin/env -S just --justfile

set unstable
set lazy

export DEVENV_TUI := "false"

cargo_vendor_root := justfile_directory() + "/.cargo-vendor"
cargo_vendor_config_dir := cargo_vendor_root + "/.cargo"
cargo_vendor_dir := cargo_vendor_root + "/vendor"

_:
    @just --list

[no-cd]
[private]
_cargo-vendor-sources:
    # Shared crate source cache only; host and Flatpak builds still use separate target dirs.
    install -Dm644 '{{ justfile_directory() }}/cargo/config' '{{ cargo_vendor_config_dir }}/config.toml'
    devenv shell -- cargo vendor --locked --versioned-dirs --quiet '{{ cargo_vendor_dir }}'

[private]
_flatpak-build-dir:
    if ! test -f '{{ justfile_directory() }}/build-dir/metadata'; then \
        just build; \
    fi

[private]
_module command: _cargo-vendor-sources _flatpak-build-dir
    flatpak build \
        '--bind-mount=/run/build/kiriview={{ justfile_directory() }}' \
        '--bind-mount=/run/build/kiriview/.cargo={{ cargo_vendor_config_dir }}' \
        '--bind-mount=/run/build/kiriview/cargo/vendor={{ cargo_vendor_dir }}' \
        --build-dir=/run/build/kiriview \
        --env=CARGO_TARGET_DIR=/run/build/kiriview/target/flatpak \
        --env=PATH=/usr/lib/sdk/rust-stable/bin:/app/bin:/usr/bin \
        build-dir \
        {{ command }}

[private]
_module-llvm command: _cargo-vendor-sources _flatpak-build-dir
    llvm_sdk="$(flatpak info --show-location org.freedesktop.Sdk.Extension.llvm21//25.08)/files" && \
    flatpak build \
        "--bind-mount=/usr/lib/sdk/llvm21=$llvm_sdk" \
        '--bind-mount=/run/build/kiriview={{ justfile_directory() }}' \
        '--bind-mount=/run/build/kiriview/.cargo={{ cargo_vendor_config_dir }}' \
        '--bind-mount=/run/build/kiriview/cargo/vendor={{ cargo_vendor_dir }}' \
        --build-dir=/run/build/kiriview \
        --env=CARGO_TARGET_DIR=/run/build/kiriview/target/flatpak \
        --env=PATH=/usr/lib/sdk/rust-stable/bin:/usr/lib/sdk/llvm21/bin:/app/bin:/usr/bin \
        build-dir \
        {{ command }}

[group('ci')]
lint: _cargo-vendor-sources
    devenv shell -- lint-clippy
    devenv shell -- lint-qmllint
    devenv shell -- lint-clang-tidy
    devenv shell -- lint-clazy

[group('ci')]
test:
    just _module 'cargo --offline test --all-targets --all-features'
    just _module 'cmake -S tests/cpp -B target/flatpak/cpp-tests -DCMAKE_BUILD_TYPE=Debug'
    just _module 'cmake --build target/flatpak/cpp-tests'
    just _module 'ctest --test-dir target/flatpak/cpp-tests --output-on-failure'

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
