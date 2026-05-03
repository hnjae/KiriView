# KiriView Specification

## Current Scope

KiriView opens one user-selected image file or supported archive, displays it in
the main window, and can navigate to adjacent images or pages in the same
location.

The main window toolbar shows image controls without a page title. The compact
toolbar always provides open, page navigation, fit, and zoom controls in fixed
positions. Outside fullscreen, the toolbar uses normal application header
placement and reserves layout space above the image viewing area. Controls that
require a ready image are disabled until an image is ready.
When KiriView is launched with a file path or URL argument, including from a
file manager's Open With action, it opens the first provided image or comic book
archive at startup. Activating the open action shows the XDG portal file
chooser, which accepts a single selection only. Pressing Escape closes the main
window.
If KiriView is launched with a local file path or file URL argument and that
file does not exist, it prints a clear error message naming the path and reason
to standard error, does not open the main window, and exits with code 2.

When an image file is displayed, the main window title is the displayed image
file name, a spaced em dash, and `KiriView`. When a CBZ, CBT, CB7, CBR, ZIP,
TAR, 7Z, or RAR archive opened by KiriView is displayed, the title is the
archive file name, a spaced em dash, and `KiriView`. KiriView does not show file
paths in the window title. When no image or archive page is displayed, the
window title is `KiriView`.

The toolbar also provides icon-only Previous Container and Next Container
buttons next to the open action. When container navigation is available, these
buttons navigate between sibling comic book archive containers and are labeled
only by their tooltips.

## File Access

KiriView opens user-selected image URLs, including local files,
KDE-supported remote URLs such as `smb://`, and KDE-supported archive URLs
such as `zip://`, `tar://`, and `sevenz://`.

KiriView also opens local `.cbz`, `.cbt`, `.cb7`, and `.cbr` comic book archives.
When a local comic book archive is opened directly, KiriView owns that archive
as the current archive document and displays the first supported image inside
that archive.

KiriView opens local `.zip`, `.tar`, `.7z`, and `.rar` archives only when they are
directly provided, such as through a startup argument or the open dialog's
`All files (*)` filter. When a local general archive is opened directly,
KiriView owns that archive as the current archive document and displays the
first supported image inside that archive. These general archives are not
advertised through the desktop file's file associations, the open dialog's
default image and comic book filter, or sibling container navigation.

When an image is opened from a KDE-supported archive URL such as `zip://`,
`tar://`, or `sevenz://`, KiriView opens that image URL rather than owning the
whole archive as an archive document.

In Flatpak, adjacent image navigation can list neighboring files under `home`,
`/media`, `/mnt`, `/run/media`, and `$XDG_RUNTIME_DIR/gvfs`. Files outside those
paths remain available only when explicitly provided by the XDG portal.

## Image Display

The UI remains responsive while a selected image is being opened. If no image
is currently displayed, the UI shows a loading state. If an image is already
displayed, that image remains visible until the newly selected image is ready to
replace it, with a compact loading indicator shown at the top-left of the image
viewing area. If another file is selected before the previous load finishes,
only the most recent selection is displayed.

Opened images are displayed centered in the available page area while preserving
their aspect ratio. Image zoom is expressed in physical display pixels: 100%
means one image pixel maps to one physical monitor pixel.
For SVG files, 100% uses the SVG's intrinsic size. SVGs remain sharp instead of
pixelated when Fit mode, manual zoom, window resizing, or display scale changes
the displayed size.

HEIF still images are supported when the primary image item is encoded with
HEVC, AVC/H.264, JPEG, JPEG 2000, or VVC/H.266. If a recognized HEIF file uses
an unsupported compression format or cannot be decoded, KiriView reports the
decode error without replacing the currently displayed image.

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
the same directory or comic book archive. When the displayed image changes
through
ordinary page or container navigation, any panning position from the previous
image is cleared so the newly displayed image starts at its top-left at the
preserved zoom level. The scan-backward shortcut may open the previous image at
its final scan position instead. Starting KiriView or opening an image in a
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

When an image is ready, Ctrl+=, Ctrl++, `=`, or `+` zooms in by 10 percentage
points and Ctrl+- or `-` zooms out by 10 percentage points. Keyboard zoom uses
the same 10% through 800% manual zoom range as the toolbar zoom input. Holding
Ctrl and dragging vertically on the image zooms around the cursor: dragging up
zooms in and dragging down zooms out. The unmodified `-`, `=`, and `+`
shortcuts are inactive while the page number or zoom input is focused.

When an image is ready, the `1` key selects Fit mode, `2` selects Fit Height
mode, `3` selects Fit Width mode, and `0` switches to 100% manual zoom. These
shortcuts are inactive while the page number or zoom input is focused.

When an image is zoomed large enough to pan, the arrow keys pan the image within
the available scroll bounds, `<` moves the pan position to the top-left, `>`
moves the pan position to the bottom-right, Shift+mouse wheel pans horizontally,
and the mouse cursor shows that the image can be dragged to pan. Keyboard
panning is inactive while the page number or zoom input is focused.

When an image is zoomed large enough to pan, the `.` key scans forward through
the image from left to right and then top to bottom. Each scan step moves
horizontally or vertically by at most three quarters of the visible viewport,
except that moving from the right edge of one row to the next row jumps directly
to the left edge of the next row. At the final scan position, `.` opens the next
image. The `,` key scans backward through the same positions; at the initial
scan position, it opens the previous image, starting that image at its final
scan position. When the current image is not zoomed large enough to pan, `.`
opens the next image and `,` opens the previous image. These shortcuts are
inactive while the page number or zoom input is focused.

The toolbar centers page navigation as an up-arrow Previous button, an editable
current page number, the text `of`, the total number of supported images in the
current directory or archive scope, and a down-arrow Next button. The Previous
button is disabled on the first image, and the Next button is disabled on the
last image. Page numbers are shown to users starting at 1. Entering a valid page
number opens that image; entering an invalid number leaves the current image
open and restores the displayed page number.
When moving between images in the current directory or archive scope, the
centered page navigation controls keep their layout stable. The current page
number updates to the newly displayed image, and the known total image count
remains visible while KiriView updates the current position.

Animated image files, including GIF, APNG, and HEIF image sequences, play when
animation frames are available. The first frame is shown once loading succeeds;
later frames use the file's frame delays and loop count. Infinite loops
continue until another image is selected or the view is cleared. APNG
animations and HEIF image sequences play as authored.

When a new image is selected while an image is already displayed, any running
animation keeps playing until the replacement image is ready. If the selected
URL cannot be read or the file is not a decodable image, the current image
remains visible and the app reports the error without disrupting the view. If no
image is currently displayed, failures show an error state. If animation
playback fails for the displayed image, the UI shows an error state and remains
ready for another open action.

## Image Navigation

When an image is open, Page Up opens the previous supported image file in the
same parent URL and Page Down opens the next one. If the current image is the
first image, pressing Page Up keeps the current image open and notifies the user
that it is the first image. If the current image is the last image, pressing
Page Down keeps the current image open and notifies the user that it is the last
image. Supported image extensions match the open dialog: AVIF (`.avif` and
`.avifs`), BMP, GIF, HEIF (`.heic`, `.heics`, `.heif`, `.heifs`, `.hif`,
`.avci`, and `.hej2`), JPEG, JPEG 2000 (`.jp2`), JPEG XL (`.jxl`), PNG, SVG,
and WebP, case-insensitively. JPEG-compressed HEIF files use the generic HEIF
extensions because they do not have a dedicated extension.

When an image is opened from a KDE-supported archive URL such as `zip://`,
`tar://`, or `sevenz://`, navigation moves between supported image files in the
same directory inside the archive URL.

When an image is displayed from a local CBZ, CBT, CB7, CBR, ZIP, TAR, 7Z, or
RAR archive document opened directly by KiriView, navigation moves between all
supported image files inside that archive document, including images in
subdirectories.

The previous and next files are determined by sorting candidate file names with
the user's locale-aware file name order. Navigation does not wrap; pressing Page
Up on the first candidate or Page Down on the last candidate keeps the current
image open.

If the parent URL cannot be listed, the current image is not found, or no
adjacent supported image exists, the current image remains open and the app
remains ready for another open action.

After an image is displayed, KiriView may make adjacent images available for
quicker Previous or Next navigation, so the switch can happen without showing a
full-page loading state. This preparation must not change what is displayed
until the user opens an adjacent image.

## Container Navigation

A container is a local CBZ, CBT, CB7, or CBR comic book archive. The current
container can be the comic book archive whose image is displayed, or an empty
sibling comic book archive reached through container navigation. When the
current image is inside a local comic book archive opened directly by KiriView,
its container is that archive file. The Previous Container and Next Container
toolbar actions open the previous or next sibling comic book archive beside the
current container.

When the current image is a normal image file, inside a KDE-supported archive
URL, or inside a directly opened ZIP, TAR, 7Z, or RAR archive document, the
Previous Container and Next Container toolbar actions are disabled.

Sibling container candidates are local `.cbz`, `.cbt`, `.cb7`, or `.cbr` files
in the current container's parent directory. Candidates are sorted with the same
user locale-aware file name order used for image navigation. Navigation does not
wrap; pressing Previous Container on the first candidate or Next Container on
the last candidate keeps the current view unchanged.

The `[` key opens the previous sibling container and the `]` key opens the next
sibling container when container navigation is available. Home and Ctrl+Home open
the first image in the current container, and End and Ctrl+End open the last
image in the current container. These shortcuts are inactive while the page
number or zoom input is focused.

Opening a comic book archive container displays the first supported image in
that archive using the same archive image ordering as page navigation.

If a target sibling container has no supported images, KiriView clears any
displayed image and shows an error state explaining that the selected container
does not contain supported images. That empty container remains the current
container navigation position, so Previous Container and Next Container can
continue to move to neighboring containers.

## Window Shortcuts

The `f` key and F11 toggle the main window between normal windowed display and
system fullscreen. Fullscreen hides the system titlebar and window decorations
and shows the app toolbar as a compact, translucent overlay above the image
viewing area without reserving layout space. The fullscreen overlay toolbar is
shown when entering fullscreen and when the pointer moves over the window. It
hides after 1.0 seconds of inactivity unless the pointer is over the toolbar, a
toolbar input is focused, or a toolbar menu is open. Leaving fullscreen restores
the window's previous windowed, maximized, or minimized state and restores the
normal header toolbar. The `f` shortcut is inactive while the page number or
zoom input is focused.

The `?` key and F1 open a modal shortcut help dialog listing KiriView's keyboard
and mouse shortcuts. The `?` shortcut is inactive while the page number or zoom
input is focused. While the shortcut help dialog is open, Escape closes the
dialog instead of closing the main window.

Out of scope for the current version: editing, metadata panels, and file
management actions.
