# KiriView

KiriView is a desktop image viewer built with KDE Kirigami.

> [!IMPORTANT]
> This repository was developed with AI-assisted workflows.

## Features

- Supports common image formats through Qt and KDE image plugins.
- Adds dedicated support for APNG, RAW images, AVIF images with alpha channels, and HEIF images using non-HEVC codecs, including HEIF sequences, where Qt/KDE support is limited.
- Opens comic book archives: `.cbz`, `.cbr`, `.cb7`, `.cbt`.
- Preloads adjacent images for faster previous/next navigation, while respecting the system Power Saver mode.

## Limitations

KiriView relies on runtime archive libraries instead of implementing archive parsers itself. ZIP, 7Z, and TAR are handled through KDE KArchive, while RAR is handled through libarchive, so archive compatibility, encryption support, and error handling follow those libraries.

As of May 2026, libarchive 3.8.5 in the current Flatpak SDK does not support RAR4 solid archives.

## Controls

- Alt+mouse wheel scrolls horizontally on KDE/Qt.
- Ctrl+mouse wheel zooms around the cursor.

## Known Issues

On some Plasma Wayland systems with KWin 6.4 or 6.5, rapidly opening or closing the toolbar application menu can crash KWin. KDE tracks the related compositor crash as [Bug 506916](https://bugs.kde.org/show_bug.cgi?id=506916), fixed in KWin 6.6.1. If you encounter this, update Plasma/KWin to 6.6.1 or newer.

## Development Build

KiriView can be built and installed locally as a Flatpak development build. This is not a stable release build.

```sh
just install
flatpak run org.hnjae.kiriview
```

See [Flatpak Development Builds](docs/flatpak.md) for manual build steps, removal instructions, and packaging notes.
