#!/usr/bin/env -S just --justfile

set unstable
set lazy

export DEVENV_TUI := "false"

_:
    @just --list

[group('ci')]
lint:
    devenv tasks run --refresh-eval-cache --refresh-task-cache --mode before kiriview:lint

alias test-host := test

[group('ci')]
test:
    devenv tasks run --refresh-eval-cache --refresh-task-cache --mode before kiriview:test:host

[group('ci')]
format:
    devenv shell -- treefmt

[group('ci')]
format-check:
    devenv shell -- treefmt --ci

[group('ci')]
i18n-check:
    devenv tasks run --refresh-eval-cache --refresh-task-cache --mode before kiriview:i18n:check

[group('ci')]
i18n-pot-check:
    devenv tasks run --refresh-eval-cache --refresh-task-cache --mode before kiriview:i18n:pot-check

[group('ci')]
check:
    devenv tasks run --refresh-eval-cache --refresh-task-cache --mode before kiriview:check

[group('ci')]
ci:
    just check

[group('build')]
i18n-update:
    devenv shell -- kiriview-update-translations

[group('build')]
build: (_flatpak-builder "build")

[group('build')]
build-with-tests: (_flatpak-builder "build-with-tests")

[group('build')]
run:
    #!/bin/sh
    set -eu

    runtime_dir="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"

    set -- \
        --socket=wayland \
        --socket=pulseaudio \
        --device=dri \
        --talk-name=org.freedesktop.portal.Desktop \
        --talk-name=org.kde.KIOFuse \
        --filesystem=home \
        --filesystem=/media \
        --filesystem=/mnt \
        --filesystem=/run/media \
        --nofilesystem=/run/user \
        --filesystem=xdg-run/pipewire-0 \
        --filesystem=xdg-run/gvfs \
        --env=LIBHEIF_PLUGIN_PATH=/app/lib/libheif \
        --env=QSG_RHI_BACKEND="${QSG_RHI_BACKEND:-vulkan}" \
        --env=WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}" \
        --env=QT_QPA_PLATFORM=wayland \
        --env=QT_LOGGING_RULES="${QT_LOGGING_RULES:-io.github.hnjae.kiriview.input.debug=true;io.github.hnjae.kiriview.predecode.debug=true;io.github.hnjae.kiriview.decode.debug=true}"

    if [ -d "$runtime_dir/doc" ]; then
        set -- "$@" --bind-mount="$runtime_dir/doc=$runtime_dir/doc"
    fi

    for kio_fuse_path in "$runtime_dir"/kio-fuse-*; do
        [ -d "$kio_fuse_path" ] || continue
        set -- "$@" "--filesystem=xdg-run/$(basename "$kio_fuse_path")"
    done

    flatpak build \
        "$@" \
        build-dir \
        kiriview

[group('build')]
install: (_flatpak-builder "install")

_flatpak-builder mode:
    #!/bin/sh
    set -eu

    mode={{ quote(mode) }}

    run_flatpak_builder() {
        if command -v flatpak-builder >/dev/null 2>&1; then
            exec flatpak-builder "$@"
        fi

        if command -v flatpak >/dev/null 2>&1; then
            if flatpak info --user org.flatpak.Builder >/dev/null 2>&1; then
                exec flatpak run --user org.flatpak.Builder "$@"
            fi

            if flatpak info --system org.flatpak.Builder >/dev/null 2>&1; then
                exec flatpak run --system org.flatpak.Builder "$@"
            fi
        fi

        if command -v nix >/dev/null 2>&1; then
            exec nix run nixpkgs#flatpak-builder -- "$@"
        fi

        cat >&2 <<'EOF'
    flatpak-builder was not found.

    Install flatpak-builder with one of these options:
    - Install your distribution's flatpak-builder package.
    - Install the Flatpak app: flatpak install --user flathub org.flatpak.Builder
    - Install Nix so this task can run nixpkgs#flatpak-builder.
    EOF
        exit 127
    }

    flatpak_jobs="${KIRIVIEW_FLATPAK_JOBS:-}"
    if [ -z "$flatpak_jobs" ]; then
        flatpak_jobs="$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || printf '1')"
    fi
    case "$flatpak_jobs" in
        ''|*[!0-9]*|0)
            printf 'Invalid KIRIVIEW_FLATPAK_JOBS value: %s\n' "$flatpak_jobs" >&2
            exit 2
            ;;
    esac

    set -- \
        --default-branch main \
        --user \
        --install-deps-from=flathub \
        --jobs="$flatpak_jobs" \
        --ccache \
        --keep-build-dirs

    case "$mode" in
        build)
            set -- "$@" --disable-tests
            ;;
        build-with-tests)
            ;;
        install)
            set -- "$@" --install --disable-tests
            ;;
        *)
            printf 'Invalid flatpak-builder mode: %s\n' "$mode" >&2
            exit 2
            ;;
    esac

    run_flatpak_builder "$@" \
        --force-clean build-dir \
        io.github.hnjae.KiriView.json
