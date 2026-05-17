# KiriView

KiriView is a desktop image viewer built with KDE Kirigami.

> [!IMPORTANT]
> All documentation, code, and artwork in this repository were vibe coded.

## Features

- Supports common image formats through Qt and KDE image plugins.
- Adds dedicated support where Qt/KDE fall short: APNG, AVIF images with alpha channels, HEIF sequences (`.heics`), and HEIF still images using non-HEVC codecs.
- Opens comic book archives: `.cbz`, `.cbr`, `.cb7`, `.cbt`.
- Smoothly zooms and pans large static images with GPU-backed tiled rendering.
- Preloads adjacent images for faster previous/next navigation, while respecting the system Power Saver mode.

## Limitations

KiriView relies on runtime archive libraries instead of implementing archive parsers itself. ZIP, 7Z, and TAR are handled through KDE KArchive, while RAR is handled through libarchive, so archive compatibility, encryption support, and error handling follow those libraries.

As of May 2026, libarchive 3.8.5 in the current Flatpak SDK does not support RAR4 solid archives.

## Install the Current Development Version

These instructions install the current source checkout as a local Flatpak development build. This is not a stable release build.

1. Install `flatpak` and `flatpak-builder`.
1. Add Flathub if it is not already configured:

   ```sh
   flatpak --user remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
   ```

1. Build and install KiriView into your user Flatpak installation:

   ```sh
   flatpak-builder \
       --user \
       --install \
       --install-deps-from=flathub \
       --ccache \
       --keep-build-dirs \
       --disable-tests \
       --force-clean build-dir \
       io.github.hnjae.KiriView.json
   ```

1. Run the installed development build:

   ```sh
   flatpak run io.github.hnjae.KiriView
   ```

To update the installed development build after changing or pulling the source,
rerun the build and install command above.

To remove the local development build:

```sh
flatpak --user uninstall io.github.hnjae.KiriView
```

## Flatpak Packaging Notes

The Flatpak manifest intentionally builds libheif as an application module instead of relying on the Freedesktop or KDE runtime copy. The runtime libheif and codec plugins have historically lagged the HEIF still-image and image sequence support KiriView needs, including JPEG, JPEG 2000, AVC/H.264, HEVC/H.265, and VVC/H.266 HEIF variants.

When updating the Flatpak runtime, SDK, SDK extensions, or codec plugin assumptions, re-check whether the bundled libheif module is still required before removing or changing it. At minimum, verify the current runtime libheif version, HEIF sequence decoding, alpha handling, and `just build-with-tests`.
