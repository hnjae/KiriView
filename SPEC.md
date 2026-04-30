# KiriView Specification

## Current Scope

KiriView opens one user-selected image file and displays it in the main window.

## File Access

KiriView treats files as `QUrl`s. User-visible file access is delegated to KIO so local files and KDE-supported remote URLs, such as `smb://`, follow the same path.

The image loading pipeline is:

```text
XDG Portal file chooser -> QUrl -> KIO read -> QByteArray/QIODevice -> QImageReader -> QML display
```

## Image Display

KIO provides the readable image data. Qt image APIs decode that data, and the decoded image is shown in QML.

The first supported behavior is successful display of a single still image. Multi-image browsing, editing, metadata panels, and file management actions are out of scope for the current version.
