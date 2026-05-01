# KiriView Specification

## Current Scope

KiriView opens one user-selected image file, displays it in the main window, and
can navigate to adjacent images in the same location.

The main window toolbar shows image controls without a page title. The toolbar
always provides open, previous, next, fit, and zoom controls in fixed positions.
Controls that require a ready image are disabled until an image is ready.
Activating the open action shows the XDG portal file chooser, which accepts a
single selection only.

## File Access

KiriView opens user-selected image URLs, including local files,
KDE-supported remote URLs such as `smb://`, and KDE-supported archive URLs
such as `zip://`.

KiriView also opens local CBZ comic book archives with the
`application/vnd.comicbook+zip` MIME type. Opening a CBZ archive displays the
first supported image inside that archive.

In Flatpak, adjacent image navigation can list neighboring files under `home`,
`/media`, `/mnt`, `/run/media`, and `$XDG_RUNTIME_DIR/gvfs`. Files outside those
paths remain available only when explicitly provided by the XDG portal.

## Image Display

Image loading is asynchronous. While a selected image is being opened, the UI
shows a loading state and remains responsive. If another file is selected before
the previous load finishes, only the most recent selection is displayed.

Opened images are displayed centered in the available page area while preserving
their aspect ratio. Image zoom is expressed in physical display pixels: 100%
means one image pixel maps to one physical monitor pixel. On displays using
desktop scaling, the default zoom for a newly opened image matches the display's
device pixel ratio. For example, a small image opens at 200% on a display using
200% scaling, which gives the image a logical 1:1 size in the desktop
coordinate system. If the image would exceed the available viewport at that
default zoom, it is scaled down only as far as needed to fit.

When a ready image is larger than the viewport at the current zoom, horizontal
and vertical scrollbars allow panning across the image. When the image is
smaller than the viewport, it remains centered.

The toolbar provides a zoom percentage input. When an image is ready, users can
enter manual zoom values from 10% through 800%. A fit action returns the image
to the default display-scale-aware fit behavior.

Animated image files, including GIF and APNG, play when animation frames are
available. The first frame is shown once loading succeeds; later frames use the
file's frame delays and loop count. Infinite loops continue until another image
is selected or the view is cleared. APNG playback follows frame composition
rules, including blend and disposal operations.

When a new image is selected, any running animation stops before the new load
starts. If the selected URL cannot be read, the file is not a decodable image,
or animation playback fails, the UI shows an error state and remains ready for
another open action.

## Image Navigation

When an image is open, Page Up or the Previous window action opens the previous
supported image file in the same parent URL. Page Down or the Next window action
opens the next one. Supported image extensions match the open dialog: AVIF, BMP,
GIF, JPEG, PNG, SVG, and WebP, case-insensitively.

When an image is opened from a KDE-supported archive URL such as `zip://`,
navigation moves between supported image files in the same directory inside the
archive.

When an image is opened by selecting a CBZ archive, navigation moves between all
supported image files inside that archive, including images in subdirectories.

The previous and next files are determined by sorting candidate file names with
the process locale collation order, including `LC_COLLATE`. Navigation does not
wrap; pressing Page Up on the first candidate or Page Down on the last candidate
keeps the current image open.

If the parent URL cannot be listed, the current image is not found, or no
adjacent supported image exists, the current image remains open and the app
remains ready for another open action.

Out of scope for the current version: editing, metadata panels, and file
management actions.
