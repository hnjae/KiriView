# KiriView Specification

## Current Scope

KiriView opens one user-selected image file and displays it in the main window.

The main window provides one open action. Activating it shows the system file chooser through the XDG portal. The file chooser accepts a single selection only.

## File Access

KiriView treats files as `QUrl`s. User-visible file access is delegated to KIO so local files and KDE-supported remote URLs, such as `smb://`, follow the same path.

The image loading pipeline is:

```text
XDG Portal file chooser -> QUrl -> KIO read -> QByteArray/QIODevice -> Qt image decoder -> QML display
```

When KiriView is launched from the Flatpak build directory for development, the
runtime sandbox must expose the XDG document portal mount at
`$XDG_RUNTIME_DIR/doc` so portal-selected document URLs resolve inside the
sandbox.

## Image Display

KIO provides the readable image data. Qt image APIs decode that data, and the decoded image is shown in QML.

Image loading is asynchronous. While a selected image is being read and decoded, the UI shows a loading state and remains responsive. If another file is selected before the previous load finishes, only the most recent selection is displayed.

Decoded images are displayed centered in the available page area while preserving their aspect ratio. If the image is larger than the available area, it is scaled down to fit. If it is smaller, it remains centered without being scaled up.

Animated image files, such as GIF, are displayed as animations when Qt's decoder reports multiple frames. Animated PNG files are also displayed as animations when APNG control chunks are present, even if Qt's generic PNG decoder exposes only the default still image. The first decoded animation frame is shown once loading succeeds, then subsequent frames replace it using the frame delay reported by the decoder or APNG frame metadata. The animation loops according to the file's loop count metadata, and an infinite loop continues until another image is selected or the view is cleared.

Animated PNG playback follows APNG frame composition rules. KiriView decodes each frame image, composites it onto the PNG canvas using the frame blend and disposal operations, and treats an APNG play count of `0` as infinite looping.

When a new image is selected, any currently running animation stops before the new load starts. If animation frame decoding fails after the first frame was displayed, the UI switches to the error state and remains ready for another open action.

If KIO cannot read the selected URL, or Qt cannot decode the returned bytes as an image, the UI shows an error state and keeps the app ready for another open action.

The first supported behavior is successful display of one selected image, including still images and decoder-supported animated images. Multi-image browsing, editing, metadata panels, zoom controls, pan controls, and file management actions are out of scope for the current version.
