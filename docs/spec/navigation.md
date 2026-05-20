# Navigation

## Page Controls

The toolbar provides page navigation with Previous and Next actions, the current page number, the total number of supported images in the current directory or archive scope after that scope's supported image list has been confirmed, and editable page number entry.

First and Last are page navigation actions available through their configured shortcuts and menus. They open the first or last known page in the current image scope.

The Previous action is disabled on the first image, and the Next action is disabled on the last image.

Page numbers are shown to users starting at 1.

When a new directory or archive scope is being listed and KiriView has no confirmed supported image list for that same scope yet, the current page number and total image count are unknown, and KiriView does not treat the current image as the first or last image.

Entering a page number and pressing Enter or clicking the image viewing area opens the nearest valid page, returns focus to the image viewing area, and restores viewer keyboard shortcuts.

If the entered text cannot be parsed as a number, KiriView leaves the current image open and restores the displayed page number.

Pressing Escape while editing the page number cancels the edit, restores the displayed page number, returns focus to the image viewing area, and does not leave fullscreen.

When KiriView has a confirmed supported image list for the current image scope, Previous, Next, First, Last, and page number entry remain available while a selected image is still loading.

During that loading interval, the page number shown in the UI is the most recent valid page selection requested by the user, not necessarily the image that remains visible until replacement succeeds.

The displayed image URL continues to mean the image actually being shown.

If users make multiple page selections before loading finishes, only the most recent selection is displayed.

If that replacement load fails, the previously displayed image remains visible, KiriView reports the error, and page navigation returns to the still-displayed image.

When moving between images in the current directory or archive scope, the page navigation controls keep their layout stable.

The current page number updates to the newly displayed image, and the known total image count remains visible while KiriView updates the current position.

## Adjacent Images

When an image is open, Page Up opens the previous supported image file in the same parent URL and Page Down opens the next one.

If the current image is the first image, pressing Page Up keeps the current image open and notifies the user that it is the first image.

If the current image is the last image, pressing Page Down keeps the current image open and notifies the user that it is the last image.

Supported image extensions match the open dialog: AVIF (`.avif` and `.avifs`), BMP, camera RAW (`.3fr`, `.arw`, `.bay`, `.bmq`, `.cr2`, `.cr3`, `.crw`, `.cs1`, `.cs2`, `.dcr`, `.dng`, `.erf`, `.fff`, `.iiq`, `.k25`, `.kdc`, `.mdc`, `.mef`, `.mos`, `.mrw`, `.nef`, `.nrw`, `.orf`, `.pef`, `.raf`, `.raw`, `.rdc`, `.rwl`, `.rw2`, `.sr2`, `.srf`, `.srw`, and `.x3f`), GIF, HEIF (`.heic`, `.heics`, `.heif`, `.heifs`, `.hif`, `.avci`, and `.hej2`), JPEG, JPEG 2000 (`.jp2`), JPEG XL (`.jxl`), PNG, SVG, TIFF (`.tif` and `.tiff`), and WebP, case-insensitively.

JPEG-compressed HEIF files use the generic HEIF extensions because they do not have a dedicated extension.

When an image is opened from a KDE-supported archive URL such as `zip://`, `tar://`, or `sevenz://`, KiriView treats the opened item as a single image URL, and navigation moves between supported image files in the same directory inside that archive URL.

When an image is displayed from a local CBZ, CBT, CB7, CBR, ZIP, TAR, 7Z, or RAR archive document opened directly by KiriView, navigation moves between all supported image files inside that archive document, including images in subdirectories.

When an image is displayed from a local directory document opened directly by KiriView, navigation moves between all supported image files inside that directory tree, including images in subdirectories.

After the archive or directory document has been listed, page navigation uses all supported image files inside that document as its navigation target set.

If the parent URL cannot be listed, the current image is not found, or no adjacent supported image exists, the current image remains open and the app remains ready for another open action.

## Sorting and Boundary Feedback

The previous and next files are determined by sorting candidate names with the user's locale-aware file name order.

For ordinary directory navigation, the candidate name is the file name.

For archive and directory documents opened directly by KiriView, candidate names are document-relative paths such as `foo/a.jpg` and `bar/a.jpg`.

Navigation does not wrap. Pressing Page Up on the first candidate or Page Down on the last candidate keeps the current image open and notifies the user that it is the first or last image.

KiriView shows those first-image and last-image notifications only when the current supported image list is known and the current image is at a known boundary.

KiriView has one visible in-app toast notification slot.

New toast requests replace the current toast and replay the entrance animation, including when the same message and scope are requested again while already visible.

A first-image or last-image toast is scoped to the currently displayed image's known boundary.

When the displayed image changes or is cleared, KiriView removes the current toast only if its scope is the image boundary. Non-boundary toasts such as file operation errors remain governed by normal replacement and timeout behavior.

## Panning and Viewer Keys

When a ready image is larger than the viewport at the current zoom, horizontal and vertical scrollbars allow panning across the image.

When the image is smaller than the viewport, it remains centered.

While the page number or zoom input is not focused, the plain arrow keys are fixed viewer-only shortcuts for keyboard panning and physical adjacent navigation. They are not user-configurable actions and are not listed in Keyboard Shortcuts configuration, shortcut help, or menus.

When an image is horizontally pannable at the current zoom, Left and Right pan the image within the available horizontal scroll bounds.

When the current image is not horizontally pannable, Left opens the previous image and Right opens the next image with the same boundary behavior as the Previous and Next actions.

In Right-to-Left Reading mode, Left and Right keep physical horizontal panning while the image can pan horizontally, but their non-pannable image navigation fallback is reversed: Left opens the next image and Right opens the previous image.

When an image is zoomed large enough to pan in any direction, Up and Down pan the image vertically within the available scroll bounds and have no image-navigation fallback. Ctrl+< moves the pan position to the top-left, Ctrl+> moves the pan position to the bottom-right, and the mouse cursor shows that the image can be dragged to pan.

Keyboard panning and Left/Right image navigation are inactive while the page number or zoom input is focused.

## Scan Shortcuts

When an image is zoomed large enough to pan, Ctrl+. scans forward through the image from left to right and then top to bottom.

Each scan step moves horizontally or vertically by at most three quarters of the visible viewport, except that moving from the right edge of one row to the next row jumps directly to the left edge of the next row.

At the final scan position, Ctrl+. opens the next image.

Ctrl+, scans backward through the same positions. At the initial scan position, it opens the previous image, starting that image at its final scan position.

When the current image is not zoomed large enough to pan, Ctrl+. opens the next image and Ctrl+, opens the previous image.

In Right-to-Left Reading mode, scan order starts at the top-right and proceeds toward the bottom-left. Ctrl+. still scans forward or opens the next image, Ctrl+, still scans backward or opens the previous image, Ctrl+< jumps to scan start, and Ctrl+> jumps to scan end.

## Background Loading

After an image is displayed, KiriView may make adjacent images available for quicker Previous or Next navigation, so the switch can happen without showing a full-page loading state.

This background work must not change what is displayed until the user opens an adjacent image.

When Two-Page Spread shows two pages, both the current primary page and the visible secondary page are treated as displayed images for this background work.

When users move quickly through pages, KiriView may briefly postpone this background work and then prioritize pages around the page where navigation settles, rather than every skipped page.

Directly opened archive and directory documents may make more pages available in the current reading direction than ordinary image navigation.

When the desktop Power Saver mode is enabled, KiriView does not newly schedule or run background work for adjacent pages.

Power Saver mode does not prevent recently displayed images from staying available for quick navigation.

Foreground image loading and visible image detail updates continue to work while Power Saver mode is enabled.
