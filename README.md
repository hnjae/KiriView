# KiriView

KiriView is a desktop image viewer built with KDE Kirigami.

> [!IMPORTANT]
> All documentation, code, and artwork in this repository were vibe coded.

## Features

- Supports common image formats through Qt and KDE image plugins.
- Adds dedicated support for APNG, AVIF images with alpha channels, and HEIF images using non-HEVC codecs, including HEIF sequences, where Qt/KDE support is limited.
- Opens comic book archives: `.cbz`, `.cbr`, `.cb7`, `.cbt`.
- Smoothly zooms and pans large static images with GPU-backed tiled rendering.
- Preloads adjacent images for faster previous/next navigation, while respecting the system Power Saver mode.

## Limitations

KiriView relies on runtime archive libraries instead of implementing archive parsers itself. ZIP, 7Z, and TAR are handled through KDE KArchive, while RAR is handled through libarchive, so archive compatibility, encryption support, and error handling follow those libraries.

As of May 2026, libarchive 3.8.5 in the current Flatpak SDK does not support RAR4 solid archives.

## Development Build

KiriView can be built and installed locally as a Flatpak development build. This is not a stable release build.

```sh
just install
flatpak run io.github.hnjae.KiriView
```

See [Flatpak Development Builds](docs/flatpak.md) for manual build steps, removal instructions, and packaging notes.
