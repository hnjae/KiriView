# KiriView Specification

## Current Scope

KiriView opens one user-selected image file, displays it in the main window, and
can navigate to adjacent images in the same location.

The main window toolbar shows image controls without a page title. The toolbar
always provides open, page navigation, fit, and zoom controls in fixed
positions. Controls that require a ready image are disabled until an image is
ready.
When KiriView is launched with a file path or URL argument, including from a
file manager's Open With action, it opens the first provided image or comic book
archive at startup. Activating the open action shows the XDG portal file
chooser, which accepts a single selection only. Pressing Escape closes the main
window.

When an image file is displayed, the main window title is the displayed image
file name, a spaced em dash, and `KiriView`. When a CBZ comic book archive
opened by KiriView is displayed, the title is the CBZ archive file name, a
spaced em dash, and `KiriView`. KiriView does not show file paths in the window
title. When no image or comic book page is displayed, the window title is
`KiriView`.

The toolbar also provides icon-only Previous Container and Next Container
buttons next to the open action. These buttons navigate between sibling image
containers and are labeled only by their tooltips.

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
remains responsive. If no image is currently displayed, the UI shows a loading
state. If an image is already displayed, that image remains visible until the
newly selected image is ready to replace it. If another file is selected before
the previous load finishes, only the most recent selection is displayed.

Opened images are displayed centered in the available page area while preserving
their aspect ratio. Image zoom is expressed in physical display pixels: 100%
means one image pixel maps to one physical monitor pixel.
For SVG files, 100% uses the SVG's intrinsic size. KiriView renders SVGs for
the current displayed size and display device pixel ratio, so Fit mode, manual
zoom, window resizing, and display scale changes preserve vector sharpness up to
the maximum renderable texture size.

KiriView starts in Fit mode. Fit mode scales the image as large as possible
while keeping the full image visible in the viewport, including upscaling small
images when space is available. Fit Height mode scales the image height to the
viewport height while preserving aspect ratio. Fit Width mode scales the image
width to the viewport width while preserving aspect ratio.

Within the same image container, KiriView preserves the current zoom state while
users navigate between images. If the user has selected Fit, Fit Height, or Fit
Width, that fit mode remains selected and recalculates for each image and
viewport size. If the user has entered a manual zoom value, that exact
percentage remains active while moving to previous, next, or numbered pages in
the same directory or CBZ archive. Starting KiriView or opening an image in a
different container resets zoom to Fit mode.

The image viewing area behind empty, loading, ready, and error states uses
`#3c3c3c` as its background color, so navigation transitions do not flash to a
different page color. The toolbar keeps its normal application styling. When an
image is ready, the image viewing viewport does not add page padding around the
image area.

When a ready image is larger than the viewport at the current zoom, horizontal
and vertical scrollbars allow panning across the image. When the image is
smaller than the viewport, it remains centered.

The toolbar provides a zoom percentage input. When an image is ready, users can
enter manual zoom values from 10% through 800%. Editing the zoom input switches
to manual zoom. A fit menu provides Fit, Fit Height, and Fit Width actions and
shows the selected fit mode. The fit action returns the image to Fit mode.

When an image is ready, Ctrl+= or Ctrl++ zooms in by 10 percentage points and
Ctrl+- zooms out by 10 percentage points. Keyboard zoom uses the same 10%
through 800% manual zoom range as the toolbar zoom input. Holding Ctrl and
dragging vertically on the image zooms around the cursor: dragging up zooms in
and dragging down zooms out.

When an image is zoomed large enough to pan, the arrow keys pan the image within
the available scroll bounds. Arrow-key panning is inactive while the page number
or zoom input is focused.

The toolbar centers page navigation as an up-arrow Previous button, an editable
current page number, the text `of`, the total number of supported images in the
current directory or archive scope, and a down-arrow Next button. Page numbers
are shown to users starting at 1. Entering a valid page number opens that image;
entering an invalid number leaves the current image open and restores the
displayed page number.

Animated image files, including GIF and APNG, play when animation frames are
available. The first frame is shown once loading succeeds; later frames use the
file's frame delays and loop count. Infinite loops continue until another image
is selected or the view is cleared. APNG playback follows frame composition
rules, including blend and disposal operations.

When a new image is selected while an image is already displayed, any running
animation keeps playing until the replacement image is ready. If the selected
URL cannot be read or the file is not a decodable image, the current image
remains visible and the app reports the error without disrupting the view. If no
image is currently displayed, failures show an error state. If animation
playback fails for the displayed image, the UI shows an error state and remains
ready for another open action.

## Image Navigation

When an image is open, Page Up or the Previous window action opens the previous
supported image file in the same parent URL. Page Down or the Next window action
opens the next one. Supported image extensions match the open dialog: AVIF
(`.avif` and `.avifs`), BMP, GIF, HEIF (`.heic`, `.heif`, and `.hif`), JPEG,
JPEG 2000 (`.jp2`), JPEG XL (`.jxl`), PNG, SVG, and WebP, case-insensitively.

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

After an image is displayed, KiriView may prepare adjacent images in the
background so pressing Previous or Next can switch images without showing a
full-page loading state. KiriView keeps prepared static images reusable while
they remain in the current cache window, including the current image, up to two
previous images, and up to ten next images. A decoded-image memory limit may
keep fewer images prepared. Background preparation must not replace the current
image until the user opens that prepared image.

## Container Navigation

A container is either a directory or a local CBZ comic book archive. When the
current image is a normal file, its container is the image's parent directory.
When the current image is inside a CBZ archive opened by KiriView, its container
is that `.cbz` file. The Previous Container and Next Container toolbar actions
open the previous or next sibling container beside the current container.

Sibling container candidates are directly contained directories and local `.cbz`
files in the current container's parent directory. Candidates are sorted with the
same process-locale collation order used for image navigation. Navigation does
not wrap; pressing Previous Container on the first candidate or Next Container on
the last candidate keeps the current view unchanged.

The `[` key opens the previous sibling container and the `]` key opens the next
sibling container when container navigation is available. These shortcuts are
inactive while the page number or zoom input is focused.

Opening a directory container displays the first directly contained supported
image file in that directory. Directory container opening is non-recursive.
Opening a CBZ container displays the first supported image in that archive using
the same archive image ordering as page navigation.

If a target sibling container has no supported images, KiriView clears any
displayed image and shows an error state explaining that the selected container
does not contain supported images. That empty container remains the current
container navigation position, so Previous Container and Next Container can
continue to move to neighboring containers.

## Window Shortcuts

The `f` key and F11 toggle the main window between normal windowed display and
system fullscreen. Fullscreen hides the system titlebar and window decorations
while keeping the app toolbar available. Leaving fullscreen restores the
window's previous windowed, maximized, or minimized state. The `f` shortcut is
inactive while the page number or zoom input is focused.

The `?` key and F1 open a modal shortcut help dialog listing KiriView's keyboard
and mouse shortcuts. While the shortcut help dialog is open, Escape closes the
dialog instead of closing the main window.

Out of scope for the current version: editing, metadata panels, and file
management actions.
