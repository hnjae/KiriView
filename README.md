# KiriView

KiriView is a desktop image viewer built with KDE Kirigami.

> [!IMPORTANT]
> All documentation and code in this repository were vibe coded.

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
