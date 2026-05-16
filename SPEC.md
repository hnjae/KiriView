# KiriView Specification

## Current Scope

KiriView opens one user-selected image file, supported archive, or directly
provided local directory, displays it in the main window, and can navigate to
adjacent images or pages in the same location.

When an installed `kiriview` translation catalog is available, KiriView's UI
text and desktop metadata follow the user's KDE and Qt language settings. When
no matching translation is available, KiriView shows the original English text.

The main window toolbar shows image controls without a page title. It places
the page navigation group at the leading side of the toolbar, containing
Previous Image, the current page number, `of`, the total image count, and Next
Image. It places zoom, Fit, and, when Hamburger Menu presentation is active
outside fullscreen, a toolbar application menu button in a trailing action
toolbar that displays as many trailing controls as fit and moves the rest into
an overflow menu. When the trailing action toolbar runs out of horizontal space,
KiriView keeps the zoom percentage visible the longest, then Fit. Outside
fullscreen, the toolbar uses normal application header placement and reserves
layout space above the image viewing area. Controls that require a ready image
are disabled until an image is ready.
KiriView shows its application menu through a toolbar application menu button by
default outside fullscreen. Users can switch the application menu presentation
between Hamburger Menu and a conventional menubar, and KiriView remembers the
selected presentation as application state across launches. In Hamburger Menu
mode outside fullscreen, the toolbar application menu contains application
actions such as Open, archive navigation, Full Screen, shortcut
configuration, shortcut help, and Quit. In menubar mode outside fullscreen,
those application actions are available from the menubar, and any toolbar
overflow menu appears only when toolbar controls do not fit. In fullscreen,
KiriView hides both the menubar and toolbar application menu button; actions
with configured shortcuts remain available through those shortcuts.
The menubar and toolbar application menu display a representative configurable
shortcut for actions with configured shortcuts through the menu action's
shortcut column. This column is supplied only from the canonical configurable
shortcut list. The representative shortcut is the first entry in the action's
current configured shortcut list that is safe to display in menus. Delete
shortcuts, arrow and navigation-key shortcuts, and unmodified printable
shortcuts are not menu-display safe because they can affect focused text input.
Actions without a menu-display-safe representative shortcut do not show
canonical shortcut text in menus. Fixed shortcuts that are not part of the
configurable shortcut list may be shown only as display-only menu or tooltip
text for the control they activate; they are not canonical action shortcuts. In
the menubar, representative shortcut text is visually deemphasized from the menu
item label when the item is not pressed. In the toolbar application menu,
representative shortcut text is displayed separately on the trailing side of the
menu item. Runtime-only Ctrl-less viewer aliases are never displayed in menus.
When the menubar or toolbar application menu is open, underlined menu access
keys are activatable with either the displayed mnemonic letter alone or Alt plus
that mnemonic letter, so users can choose menu entries through the displayed
mnemonic letters. Pressing and releasing Alt alone while the toolbar
application menu is open keeps the menu open so users can complete Alt plus
mnemonic key access.
KiriView does not expose a general Settings page in the current scope.
The Keyboard Shortcuts help is shown as read-only supporting content over the
main window. It lists the current configured shortcut text for KiriView actions,
not fixed shortcuts or runtime-only Ctrl-less viewer aliases, and can be
dismissed with Escape or the close button.
When KiriView is launched with one or more file path or URL arguments, including
from a file manager's Open With action, it processes only the first argument in
the supplied order and opens it at startup. Activating the open action shows the
XDG portal file chooser, which accepts a single selection only. Dropping one or
more file or URL items onto the running main window opens only the first item in
the order supplied by the desktop environment.
If the first startup argument is a local file path or file URL and that file
does not exist, KiriView prints a clear error message naming the path and reason
to standard error, does not open the main window, and exits with code 2.
KiriView permits compact window sizes down to 14 by 12 Kirigami grid units. When
no saved window geometry overrides the launch size, KiriView opens at 24 by 20
Kirigami grid units.

When an image file is displayed, the main window title is the displayed image
file name, a spaced em dash, and `KiriView`. When a CBZ, CBT, CB7, CBR, ZIP,
TAR, 7Z, or RAR archive opened by KiriView is displayed, the title is the
archive file name, a spaced em dash, and `KiriView`. When a directly opened
local directory is displayed, the title is the directory name, a spaced em dash,
and `KiriView`. KiriView does not show file paths in the window title. When no
image, archive page, or directory page is displayed, the window title is
`KiriView`.

Open, Previous Archive, and Next Archive are provided by the application menu
and shortcuts rather than fixed toolbar buttons. Previous Archive and Next
Archive use visually distinct previous/next-use icons so they are not confused
with page Previous and Next navigation.

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
default image and comic book filter, or sibling archive navigation.

KiriView opens local directories only when they are directly provided, such as
through a startup argument, file URL, or drop. When a local directory is opened
directly, KiriView owns that directory as the current directory document and
displays the first supported image inside that directory tree. Directory
documents use the same recursive supported-image page ordering as archive
documents, with page names based on directory-relative paths such as
`chapter/page001.png`. Directly opened directories are not advertised through
the desktop file's file associations, the open dialog's default image and comic
book filter, or sibling archive navigation.

KiriView's desktop file advertises file-manager Open With handling for the image
and comic book archive MIME types corresponding to its default open dialog
filter.

When an image is opened from a KDE-supported archive URL such as `zip://`,
`tar://`, or `sevenz://`, KiriView treats it as a single image URL supplied by
KIO rather than owning the whole archive as an archive document.

In Flatpak, adjacent image navigation can list neighboring files under `home`,
`/media`, `/mnt`, `/run/media`, and `$XDG_RUNTIME_DIR/gvfs`. Files outside those
paths remain available only when explicitly provided by the XDG portal.
In Flatpak, KiriView can also request write access to those same locations for
file deletion.

When an image is ready, Delete requests moving the displayed file to trash and
Shift+Delete requests permanent deletion. The actions are available from the
application menu or menubar File menu and through their shortcuts, but not from
the toolbar. KiriView delegates user confirmation and the actual file operation
to KDE's file operation handling. If the operation is canceled, the current
image remains open and no notification is shown. If it fails, the current image
remains open and the file operation error is shown as an in-app toast
notification.

The deletion target is the displayed image URL for ordinary images, remote
URLs, and images opened directly from KDE-supported archive URLs such as
`zip://`. When the displayed image is inside a local CBZ, CBT, CB7, CBR, ZIP,
TAR, 7Z, or RAR archive document opened directly by KiriView, the deletion
target is the archive file itself rather than the currently displayed internal
image entry. When the displayed image is inside a local directory document
opened directly by KiriView, the deletion target is the directory itself rather
than the currently displayed image file.

After deletion succeeds, KiriView immediately clears the deleted image. It then
opens the next supported image in the current image scope when possible, falls
back to the previous supported image when no next image exists, and otherwise
shows the empty state. After deleting a directly opened comic book archive,
KiriView opens the first image in the next sibling comic book archive when
possible, falls back to the first image in the previous sibling comic book
archive, and otherwise shows the empty state. After deleting a directly opened
directory document, KiriView shows the empty state.

When the displayed image belongs to a local `file://` directory, KiriView keeps
that directory's supported-image candidate list live while the directory remains
the current navigation scope. External additions and removals update the page
number, total image count, and first/last boundary state. If the currently
displayed local image is removed outside KiriView, KiriView immediately clears
that image, opens the next supported image in the same sorted directory order
when possible, falls back to the previous supported image when no next image
exists, and otherwise shows the empty state. Candidate lists for non-local KIO
URLs, explicit archive URLs such as `zip://`, directly opened archive
documents, and directly opened directory documents are snapshots and do not
guarantee live external update handling.

## Image Display

The UI remains responsive while a selected image is being opened. If no image
is currently displayed, the UI shows a loading state. If no image is selected,
the empty state says that no image is selected and offers Open. If the selected
image cannot be opened while no image is displayed, the error state explains
that the image could not be opened, shows the underlying error when available,
and offers Open. If an image is already displayed, that image remains visible
until the newly selected image is ready to replace it, with a compact loading
indicator shown at the top-left of the image viewing area. If another file is
selected before the previous load finishes, only the most recent selection is
displayed.

Opened images are displayed centered in the available page area while preserving
their aspect ratio. Image zoom is expressed in physical display pixels: 100%
means one image pixel maps to one physical monitor pixel.
For SVG files, 100% uses the SVG's intrinsic size. SVGs remain sharp instead of
pixelated when Fit mode, manual zoom, window resizing, or display scale changes
the displayed size.

Static image files, including bitmap images and SVG files, appear at full
resolution when they are small enough to display directly. Larger JPEG images
may first appear quickly at a lower level of detail, then the visible area
becomes sharper as KiriView prepares the detail needed for the current zoom,
pan position, viewport size, and display scale. Newly visible areas may briefly
show less detail until sharper data is ready, but normal panning should avoid
blank holes. If an adjacent image has already been prepared, KiriView uses that
prepared image before starting a fresh decode, so Previous and Next navigation
can replace the view immediately. If a very large static image cannot be opened
because it is too large, KiriView reports a decode error without replacing the
currently displayed image.

HEIF-family still images, including AVIF still images, are supported when the
still image is encoded with AV1, HEVC, AVC/H.264, JPEG, JPEG 2000, or
VVC/H.266. If a recognized HEIF-family still image uses an unsupported
compression format or cannot be decoded, KiriView reports the decode error
without replacing the currently displayed image.

KiriView starts in Fit mode. Fit mode scales the image as large as possible
while keeping the full image visible in the viewport, including upscaling small
images when space is available. Fit Height mode scales the image height to the
viewport height while preserving aspect ratio. Fit Width mode scales the image
width to the viewport width while preserving aspect ratio.

Within the same directly opened archive document, KiriView preserves the
current zoom state while users move between pages with Previous, Next, or page
number navigation. If the user has selected Fit, Fit Height, or Fit Width, that
fit mode remains selected and recalculates for each page and viewport size. If
the user has entered a manual zoom value, that exact percentage remains active.
When the displayed page changes inside the archive document through ordinary
page navigation, any panning position from the previous page is cleared so the
newly displayed page starts at its scan start at the preserved zoom level:
top-left normally and top-right in Right-to-Left Reading mode. The
scan-backward shortcut may open the previous image at its final scan position
instead: bottom-right normally and bottom-left in Right-to-Left Reading mode.
Starting KiriView, opening a regular image, moving between regular directory
images, opening a KDE archive URL image directly, opening a different archive
or directory document, or moving to a sibling archive resets zoom to Fit mode
when the new image is displayed.

The image viewing area behind empty, loading, ready, and error states uses
`#3c3c3c` as its background color, so navigation transitions do not flash to a
different page color. The toolbar keeps its normal application styling. When an
image is ready, the image viewing viewport does not add page padding around the
image area.

When a ready image is larger than the viewport at the current zoom, horizontal
and vertical scrollbars allow panning across the image. When the image is
smaller than the viewport, it remains centered.

When a directly opened local CBZ, CBT, CB7, or CBR comic book archive is
displayed, Ctrl+S toggles Two-Page Spread. While the page number or zoom input
is not focused, KiriView also accepts the runtime-only alias `s` for Two-Page
Spread. Two-Page Spread displays the current page on the left and the next page
on the right when both pages are eligible. The first archive page is treated as
a cover and is always displayed alone. Any page whose pixel width is greater
than its pixel height is treated as a wide page and is displayed alone. If the
next page after the current page is wide, the current page is displayed alone
and the next navigation action opens that wide page. Two-Page Spread is
unavailable for ordinary image files, KDE-supported archive URLs, directly
opened ZIP, TAR, 7Z, or RAR archives, and directly opened directories.

When a directly opened local CBZ, CBT, CB7, or CBR comic book archive is
displayed, Ctrl+B toggles Right-to-Left Reading mode. While the page number or
zoom input is not focused, KiriView also accepts the runtime-only alias `b` for
Right-to-Left Reading mode. Right-to-Left Reading mode is off by default, is
unavailable for ordinary image files, KDE-supported archive URLs, directly
opened ZIP, TAR, 7Z, or RAR archives, and directly opened directories, and is
not saved as a global setting. Moving to a sibling comic book archive with
Previous Archive or Next Archive preserves the current Right-to-Left Reading
mode state.

When Two-Page Spread shows two pages, zooming and panning operate on the combined
two-page spread as one virtual image. Fit, Fit Height, Fit Width, manual zoom,
scrollbars, drag panning, wheel zoom, keyboard panning, and scan shortcuts use
the full spread bounds. The spread has no added page gap. The page number,
window title, deletion target, and archive navigation position continue to refer
to the left/current page.
In Right-to-Left Reading mode, the current page is rendered on the right and the
next page is rendered on the left. The page number, window title, deletion
target, archive navigation position, and Ctrl+N/Ctrl+P single-page navigation
continue to refer to the current page.
When navigation in Two-Page Spread targets another eligible two-page spread,
KiriView shows the loading state instead of leaving the previous spread visible
or showing only the left target page. The target spread appears only after both
pages are ready. If the target page is the cover, wide, last page, or cannot be
paired with an eligible next page, KiriView displays the target page alone once
that page is ready.

When an image is ready and Two-Page Spread is disabled, Rotate Clockwise turns
the current image view by 90 degrees clockwise and Rotate Counterclockwise turns
it by 90 degrees counterclockwise. Rotation is display-only: it does not modify
image files, metadata, archive contents, remote URLs, cached source data, or any
saved file state. Fit, Fit Height, Fit Width, manual zoom, scrollbars, drag
panning, wheel zoom, keyboard panning, and scan shortcuts use the rotated
logical image bounds. Ctrl+R rotates clockwise, and Ctrl+Shift+R rotates
counterclockwise. While the page number or zoom input is not focused, KiriView
also accepts the runtime-only aliases `r` and Shift+R for those rotation
actions. Rotation resets to 0 degrees whenever a different image or page is
displayed, and it also resets when the displayed image is cleared. Rotation is
unavailable while Two-Page Spread is enabled; enabling Two-Page Spread resets
rotation to 0 degrees.

The toolbar provides a zoom percentage input. When an image is ready, users can
enter manual zoom values from 10% through the maximum percentage that keeps the
long edge of the displayed image within the larger of 65,536 logical pixels or
eight times the viewport long edge. The maximum manual zoom is never lower than
the normal Fit zoom percentage. Pressing Enter or clicking the image viewing
area while editing the zoom input applies the nearest valid manual zoom value,
returns focus to the image viewing area, and restores viewer keyboard shortcuts.
Pressing Escape while editing the zoom input cancels the edit, restores the
currently applied zoom percentage in the toolbar, returns focus to the image
viewing area, and does not leave fullscreen. Editing the zoom input switches to
manual zoom.
A fit menu provides Fit, Fit Height, and Fit Width actions and shows the
selected fit mode. The fit action returns the image to Fit mode.

When an image is ready, Ctrl+= or Ctrl++ zooms in by multiplying the current
zoom by 1.1, and Ctrl+- zooms out by multiplying the current zoom by 1/1.1.
Keyboard zoom uses the same dynamic manual zoom range as the toolbar zoom input.
While the page number or zoom input is not focused, KiriView also accepts the
runtime-only aliases `=`, `+`, and `-` for those zoom actions. Holding Ctrl and
using the mouse wheel over the image zooms around the cursor: wheel up zooms in
by the same multiplicative step and wheel down zooms out by its reciprocal.

When an image is ready, Ctrl+1 selects Fit mode, Ctrl+2 selects Fit Height mode,
Ctrl+3 selects Fit Width mode, and Ctrl+0 switches to 100% manual zoom. While
the page number or zoom input is not focused, KiriView also accepts the
runtime-only aliases `1`, `2`, `3`, and `0` for those fit and actual-size
actions.

When an image is horizontally pannable at the current zoom, Left and Right pan
the image within the available horizontal scroll bounds. When the current image
is not horizontally pannable, Left opens the previous image and Right opens the
next image with the same boundary behavior as the Previous and Next actions.
In Right-to-Left Reading mode, Left and Right keep physical horizontal panning
while the image can pan horizontally, but their non-pannable image navigation
fallback is reversed: Left opens the next image and Right opens the previous
image.
When an image is zoomed large enough to pan in any direction, Up and Down pan
the image vertically within the available scroll bounds, Ctrl+< moves the pan
position to the top-left, Ctrl+> moves the pan position to the bottom-right,
Shift+mouse wheel pans horizontally, and the mouse cursor shows that the image
can be dragged to pan. While the page number or zoom input is not focused,
KiriView also accepts the runtime-only aliases `<` and `>` for pan-boundary
movement. Keyboard panning and Left/Right image navigation are inactive while
the page number or zoom input is focused.

When an image is zoomed large enough to pan, Ctrl+. scans forward through the
image from left to right and then top to bottom. Each scan step moves
horizontally or vertically by at most three quarters of the visible viewport,
except that moving from the right edge of one row to the next row jumps directly
to the left edge of the next row. At the final scan position, Ctrl+. opens the
next image. Ctrl+, scans backward through the same positions; at the initial
scan position, it opens the previous image, starting that image at its final
scan position. When the current image is not zoomed large enough to pan, Ctrl+.
opens the next image and Ctrl+, opens the previous image. While the page number
or zoom input is not focused, KiriView also accepts the runtime-only aliases `.`
and `,` for scan actions.
In Right-to-Left Reading mode, scan order starts at the top-right and proceeds
toward the bottom-left: Ctrl+. still scans forward or opens the next image,
Ctrl+, still scans backward or opens the previous image, Ctrl+< jumps to scan
start, and Ctrl+> jumps to scan end.

The toolbar provides page navigation with Previous and Next actions, the current
page number, the total number of supported images in the current directory or
archive scope after that scope's candidate list has been confirmed, and editable
page number entry. The Previous action is disabled on the first image, and the
Next action is disabled on the last image. Page numbers are shown to users
starting at 1. When a new directory or archive scope is being listed and
KiriView has no confirmed candidate list for that same scope yet, the current
page number and total image count are unknown, and KiriView does not treat the
current image as the first or last image. Entering a page number and pressing
Enter or clicking the image viewing area opens the nearest valid page, returns
focus to the image viewing area, and restores viewer keyboard shortcuts. If the
entered text cannot be parsed as a number, KiriView leaves the current image
open and restores the displayed page number. Pressing Escape while editing the
page number cancels the edit, restores the displayed page number, returns focus
to the image viewing area, and does not leave fullscreen.
When KiriView has a confirmed candidate list for the current image scope,
Previous, Next, First, Last, and page number entry remain available while a
selected image is still loading. During that loading interval, the page number
shown in the UI is the most recent valid page selection requested by the user,
not necessarily the image that remains visible until replacement succeeds. The
displayed image URL continues to mean the image actually being shown. If users
make multiple page selections before loading finishes, only the most recent
selection is displayed. If that replacement load fails, the previously displayed
image remains visible, KiriView reports the error, and page navigation returns
to the still-displayed image.
When moving between images in the current directory or archive scope, the
page navigation controls keep their layout stable. The current page number
updates to the newly displayed image, and the known total image count remains
visible while KiriView updates the current position.

Animated image files, including GIF, APNG, and HEIF-family image sequences such
as `.heics` and `.avifs`, play when animation frames are available. The first
frame is shown once loading succeeds; later frames use the file's frame delays
and loop count. Infinite loops continue until another image is selected or the
view is cleared. APNG animations and HEIF-family image sequences continue to use
full-frame playback and play as authored.

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
TIFF (`.tif` and `.tiff`), and WebP, case-insensitively. JPEG-compressed HEIF
files use the generic HEIF extensions because they do not have a dedicated
extension.

When an image is opened from a KDE-supported archive URL such as `zip://`,
`tar://`, or `sevenz://`, KiriView treats the opened item as a single image URL,
and navigation moves between supported image files in the same directory inside
that archive URL.

When an image is displayed from a local CBZ, CBT, CB7, CBR, ZIP, TAR, 7Z, or
RAR archive document opened directly by KiriView, navigation moves between all
supported image files inside that archive document, including images in
subdirectories. When an image is displayed from a local directory document
opened directly by KiriView, navigation moves between all supported image files
inside that directory tree, including images in subdirectories. After the
archive or directory document has been listed, page navigation uses all
supported image files inside that document as its navigation target set.

The previous and next files are determined by sorting candidate names with the
user's locale-aware file name order. For ordinary directory navigation, the
candidate name is the file name. For archive and directory documents opened
directly by KiriView, candidate names are document-relative paths such as
`foo/a.jpg` and `bar/a.jpg`. Navigation does not wrap; pressing Page Up on the
first candidate or Page Down on the last candidate keeps the current image open
and notifies the user that it is the first or last image. KiriView shows those
first-image and last-image notifications only when the current candidate list
is known and the current image is at a known boundary.

KiriView has one visible in-app toast notification slot. New toast requests
replace the current toast and replay the entrance animation, including when the
same message and scope are requested again while already visible. A first-image
or last-image toast is scoped to the currently displayed image's known
boundary. When the displayed image changes or is cleared, KiriView removes the
current toast only if its scope is the image boundary; non-boundary toasts such
as file operation errors remain governed by normal replacement and timeout
behavior.

If the parent URL cannot be listed, the current image is not found, or no
adjacent supported image exists, the current image remains open and the app
remains ready for another open action.

When Two-Page Spread is enabled for a directly opened comic book archive,
ordinary Previous and Next image navigation move by the currently displayed
spread rather than always by one page. If two pages are visible, Next opens the
page after the right page; if only one page is visible, Next opens the next
page. Previous opens the previous eligible spread, falling back to the
immediately previous page when that page is the cover or a wide page. Ctrl+N and
Ctrl+P move forward or backward by exactly one page and are available while an
image is ready. While the page number or zoom input is not focused, KiriView
also accepts the runtime-only aliases `n` and `p` for single-page navigation.
When a Two-Page Spread navigation target is loading and the candidate list is
known, additional Previous, Next, Ctrl+N, or Ctrl+P navigation is calculated
from the pending primary page selection. KiriView moves by one primary page
selection at a time while loading; the final single-page or two-page spread
pairing is decided after the selected primary page has loaded.

After an image is displayed, KiriView may make adjacent images available for
quicker Previous or Next navigation, so the switch can happen without showing a
full-page loading state. This preparation must not change what is displayed
until the user opens an adjacent image.
When Two-Page Spread shows two pages, both the current primary page and the
visible secondary page are treated as displayed images for this preparation, so
KiriView does not prepare either visible page again or discard either visible
page as a non-displayed adjacent image.
When users move quickly through pages, KiriView may briefly postpone background
preparation and then prepare pages around the page where navigation settles,
rather than preparing every skipped page. Directly opened archive and directory
documents may prepare more pages in the current reading direction than ordinary
image navigation. When the desktop Power Saver mode is enabled, KiriView stops
background adjacent-page preparation and keeps only already displayed pages in
the preparation cache. Foreground image loading and visible image-detail
decoding continue to work while Power Saver mode is enabled.

## Archive Navigation

An archive for archive navigation is a local CBZ, CBT, CB7, or CBR comic book
archive. The current archive can be the comic book archive whose image is
displayed, or an empty sibling comic book archive reached through archive
navigation. When the current image is inside a local comic book archive opened
directly by KiriView, its archive is that archive file. The Previous Archive and
Next Archive actions open the previous or next sibling comic book archive beside
the current archive.

When the current image is a normal image file, inside a KDE-supported archive
URL, or inside a directly opened ZIP, TAR, 7Z, or RAR archive document, the
Previous Archive and Next Archive actions are disabled.
They are also disabled inside directly opened directory documents.

Sibling archive candidates are local `.cbz`, `.cbt`, `.cb7`, or `.cbr` files in
the current archive's parent directory. Candidates are sorted with the same user
locale-aware file name order used for image navigation. Navigation does not
wrap; pressing Previous Archive on the first candidate or Next Archive on the
last candidate keeps the current view unchanged.

Ctrl+[ opens the previous sibling archive and Ctrl+] opens the next sibling
archive when archive navigation is available. While the page number or zoom
input is not focused, KiriView also accepts the runtime-only aliases `[` and
`]` for those archive navigation actions. Home and Ctrl+Home open the first
image in the current archive, and End and Ctrl+End open the last image in the
current archive. Home and End are inactive while the page number or zoom input
is focused.

Opening a comic book archive displays the first supported image in that archive
using the same archive image ordering as page navigation.

If a target sibling archive has no supported images, KiriView clears any
displayed image and shows an error state explaining that the selected archive
does not contain supported images. That empty archive remains the current
archive navigation position, so Previous Archive and Next Archive can continue
to move to neighboring archives.

## Window Shortcuts

Ctrl+F and F11 toggle the main window between normal windowed display and system
fullscreen. While the page number or zoom input is not focused, KiriView also
accepts the runtime-only alias `f` for fullscreen. Fullscreen hides the system
titlebar and window decorations and shows the app toolbar as a top-attached
overlay toolbar above the image viewing area without reserving layout space. The
fullscreen overlay toolbar uses the normal toolbar background and padding,
attaches to the top, left, and right window edges, and does not use an outer
margin, rounded floating-card background, or shadow. The fullscreen overlay
toolbar contains image controls without the toolbar application menu button. The
fullscreen overlay toolbar is shown when entering fullscreen and when the
pointer moves over the window. It hides after 1.0 seconds of inactivity unless
the pointer is over the toolbar or a toolbar input is focused. Leaving
fullscreen restores the window's previous windowed, maximized, or minimized
state and restores the normal header toolbar.

Ctrl+? and F1 open a modal shortcut help dialog listing KiriView's keyboard and
mouse shortcuts. While the page number or zoom input is not focused, KiriView
also accepts the runtime-only alias `?` for shortcut help. While the shortcut
help dialog is open, Escape closes the dialog before any fullscreen handling.

Ctrl+M toggles the application menu presentation between Hamburger Menu and
Menubar. This shortcut is fixed, is not user-configurable, and is not listed in
keyboard shortcut configuration or shortcut help. KiriView may show Ctrl+M as
display-only shortcut text on the Show Menubar menu item or tooltip.
When Hamburger Menu presentation is active outside fullscreen, F10 opens the
toolbar application menu. This shortcut is fixed, is not user-configurable, and
is not listed in keyboard shortcut configuration or shortcut help. KiriView may
show F10 as display-only shortcut text on the toolbar application menu button
tooltip.

Escape first cancels an active page number or zoom input edit. When no toolbar
input is focused, Escape leaves fullscreen if the main window is fullscreen.
Outside fullscreen, Escape does not close the main window. Ctrl+Q closes the
main window. While the page number or zoom input is not focused, KiriView also
accepts the runtime-only alias `q` for Quit. Quit shortcuts using Ctrl, Alt, or
Meta remain active while those inputs are focused.

Users can open Keyboard Shortcuts configuration to configure KiriView's keyboard
shortcuts. Changing a shortcut updates the toolbar, application menu, menubar,
shortcut help, and active keyboard handling consistently. Shortcut changes apply
immediately and persist across launches. Configured shortcuts are the canonical
shortcut list. When a configured shortcut uses Ctrl or Ctrl+Shift with a
printable key, KiriView derives a matching runtime-only viewer alias without
Ctrl, such as Ctrl+L to `l` or Ctrl+Shift+R to Shift+R. Derived aliases are not
stored, are inactive while the page number or zoom input is focused, and are not
listed as separate shortcuts in menus, Keyboard Shortcuts configuration, or the
Keyboard Shortcuts help. Directly configured unmodified ASCII printable
shortcuts are removed from the canonical shortcut list.

Out of scope for the current version: editing and metadata panels.
