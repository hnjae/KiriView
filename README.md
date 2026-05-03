# KiriView

KiriView is a desktop image viewer built with KDE Kirigami.

> [!IMPORTANT]
> All documentation, code, and artwork in this repository were vibe coded.

## Supported Archives

KiriView supports comic book archive containers with `.cbz`, `.cbr`, `.cb7`,
and `.cbt` extensions. Archive decoding is delegated to archive libraries
provided by the application runtime or package: ZIP, 7Z, and TAR containers are
handled through KDE KArchive, while RAR containers are handled through
libarchive. In the Flatpak build, these libraries are supplied by the Flatpak
runtime. KiriView does not implement archive format parsers itself, so archive
compatibility, unsupported variants, encryption behavior, and error handling are
bounded by the capabilities and limitations of those libraries.

As of May 2026, this has been verified with libarchive 3.8.5 in the current
Flatpak SDK and libarchive 3.8.6 in the Nix/devenv environment: RAR4 solid
archives are not supported by libarchive.

## Install the Current Development Version

These instructions install the current source checkout as a local Flatpak
development build. This is not a stable release build.

1. Install `flatpak` and `flatpak-builder`.
1. Add Flathub if it is not already configured:

   ```sh
   flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
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
flatpak uninstall --user io.github.hnjae.KiriView
```
