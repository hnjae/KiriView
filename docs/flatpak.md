# Flatpak Development Builds

These instructions install the current source checkout as a local Flatpak development build. This is not a stable release build.

## Prerequisites

1. Install `flatpak` and `flatpak-builder`.
1. Add Flathub if it is not already configured:

   ```sh
   flatpak --user remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
   ```

## Build and Install

Build and install KiriView into your user Flatpak installation:

```sh
flatpak-builder \
    --default-branch main \
    --user \
    --install \
    --install-deps-from=flathub \
    --ccache \
    --keep-build-dirs \
    --disable-tests \
    --force-clean build-dir \
    io.github.hnjae.KiriView.json
```

The same development build can also be installed through the repository task:

```sh
just install
```

Run the installed development build:

```sh
flatpak run io.github.hnjae.KiriView
```

To update the installed development build after changing or pulling the source, rerun the build and install command above.

To remove the local development build:

```sh
flatpak --user uninstall io.github.hnjae.KiriView
```

## Packaging Notes

The Flatpak manifest intentionally builds libheif as an application module instead of relying on the Freedesktop or KDE runtime copy. The runtime libheif and codec plugins have historically lagged the HEIF still-image and image sequence support KiriView needs, including JPEG, JPEG 2000, AVC/H.264, HEVC/H.265, and VVC/H.266 HEIF variants.

When updating the Flatpak runtime, SDK, SDK extensions, or codec plugin assumptions, re-check whether the bundled libheif module is still required before removing or changing it. At minimum, verify the current runtime libheif version, HEIF sequence decoding, alpha handling, and `just build-with-tests`.
