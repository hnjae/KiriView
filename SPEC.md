# KiriView Specification

## Current Scope

KiriView opens one user-selected image file and displays it in the main window.

The main window provides one open action. Activating it shows the system file chooser through the XDG portal. The file chooser accepts a single selection only.

## File Access

KiriView treats files as `QUrl`s. User-visible file access is delegated to KIO so local files and KDE-supported remote URLs, such as `smb://`, follow the same path.

The image loading pipeline is:

```text
XDG Portal file chooser -> QUrl -> KIO read -> QByteArray/QIODevice -> QImageReader -> QML display
```

When KiriView is launched from the Flatpak build directory for development, the
runtime sandbox must expose the XDG document portal mount at
`$XDG_RUNTIME_DIR/doc` so portal-selected document URLs resolve inside the
sandbox.

## Image Display

KIO provides the readable image data. Qt image APIs decode that data, and the decoded image is shown in QML.

Image loading is asynchronous. While a selected image is being read and decoded, the UI shows a loading state and remains responsive. If another file is selected before the previous load finishes, only the most recent selection is displayed.

Decoded images are displayed centered in the available page area while preserving their aspect ratio. If the image is larger than the available area, it is scaled down to fit. If it is smaller, it remains centered without being scaled up.

If KIO cannot read the selected URL, or Qt cannot decode the returned bytes as an image, the UI shows an error state and keeps the app ready for another open action.

The first supported behavior is successful display of a single still image. Multi-image browsing, editing, metadata panels, zoom controls, pan controls, and file management actions are out of scope for the current version.
