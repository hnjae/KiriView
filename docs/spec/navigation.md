# Navigation

## Page Controls

The toolbar provides page navigation with Previous and Next actions, the current page number, the total number of supported items in the current scope after that scope's supported list has been confirmed, and editable page number entry.

The document session owns the active navigation projection used by the toolbar readout, page-number entry, shared Previous, Next, First, and Last actions, menus, and shortcuts. QML renders that projection and calls session dispatch; it does not decide whether the active item is backed by direct media siblings or image-document page navigation.

The document session also owns the active navigation thumbnail-strip projection. When the active navigation list is known, the strip exposes one item per supported active navigation item with the same ordering and 1-based numbering as the toolbar readout and page-number entry.

The active navigation thumbnail strip may display generated preview thumbnails for supported direct local image items and supported image pages inside ZIP-backed archive collections: CBZ and directly opened ZIP archive entries whose metadata can identify the entry for thumbnail caching. Unsupported, pending, failed, direct video, archive-entry media, CB7/7z and other non-ZIP-backed archive-collection items, ZIP-backed entries without usable thumbnail identity metadata, and directory-collection items keep placeholder media-type icons instead of generated preview thumbnails.

The active navigation thumbnail strip chooses generated preview quality from the thumbnail's physical display size, including the current device pixel ratio. During panel resize, fractional-scale changes, or movement between screens with different scale factors, a previously ready smaller thumbnail may remain visible as a temporary fallback while KiriView loads a newly required larger thumbnail, and the strip replaces it only after the newer result is ready. If the newer request fails, KiriView keeps an existing usable thumbnail visible when one is available; otherwise the row uses the normal fallback icon path.

After visible, nearby, and user-navigation thumbnail demand for the current active navigation list has been satisfied, KiriView may silently fill supported direct local image thumbnails and supported archive-metadata-backed image page thumbnails from ZIP-backed archive collections for that active list in the background. Visible, nearby, and user-navigation demand always takes precedence over this background fill work, and new foreground demand, navigation changes, or thumbnail-list resets may cancel background work already in progress. Background fill failures do not show user-facing errors and keep the normal thumbnail fallback UI.

The active navigation thumbnail strip keeps the active item visually selected. When adjacent main-view navigation changes the active item, the strip uses a preferred visible zone inset from the viewport edges: if the selected thumbnail remains inside that zone, the strip keeps its scroll position unchanged, and if it leaves that zone, the strip may reveal it back toward a stable visible position. Adjacent Next navigation places the selected thumbnail toward the leading side of the preferred zone so more following items remain visible; adjacent Previous navigation places it toward the trailing side so more preceding items remain visible. This anchoring follows the semantic Previous or Next action dispatched by the session, including when Right-to-Left Reading mode maps physical keys differently. Nearby automatic reveal may use a short easing animation, but at most one automatic reveal target is active at a time: repeated navigation replaces older pending scroll movement with the latest selected item, and rapid navigation may update the strip immediately instead of animating each step. Direct thumbnail activation preserves the thumbnail strip scroll position unless model or layout changes require immediate containment. User-initiated thumbnail scrolling temporarily suppresses automatic preferred-zone follow movement unless the selected thumbnail would otherwise leave the visible viewport. Large jumps such as First, Last, and page-number entry may reposition the strip to keep the selected thumbnail discoverable. File open or load routing may reveal the selected thumbnail for the newly loaded scope, but KiriView does not replay stale or duplicate automatic thumbnail scrolling during programmatic synchronization. KiriView does not promise exact thumbnail pixel offsets, centered positioning, or animation duration.

For ordinary direct media URL scopes, including direct image files, direct video files, and KDE archive-entry media URLs opened as individual media items, the document session's direct media navigation is the only active page-control navigation source. The image document must not keep a competing ordinary direct media page-navigation list for those scopes.

Image-document page navigation is active only for image-document page scopes such as directly opened archive collections and directly opened directory collections.

For ordinary direct media URL scopes, archive collection scopes, and directly opened directory collection scopes, the page navigation controls count and select supported images and supported videos together.

First and Last are page navigation actions available through their configured shortcuts and menus. They open the first or last known page or media item in the current scope.

The Previous action placement is disabled on the first item, and the Next action placement is disabled on the last item.

Page numbers are shown to users starting at 1.

The active navigation projection has these user-visible invariants: `available` means the current mode exposes a navigable scope; `known` means KiriView has a confirmed current position and total count for the active scope; `editable` means entering a page number can dispatch to that same scope. When navigation is unavailable or unknown, the page-number entry is disabled, Previous, Next, First, and Last are disabled, and KiriView does not display a stale current/count pair. When navigation is known, the current number is within `1..total`, the total count is at least 1, previous and next availability match whether a previous or next target exists in reading order, and first/last boundary state matches whether the active visible item or spread is at the known start or end of the scope.

For image-mode scopes, the active navigation projection consumes the image document's full page-navigation snapshot rather than a single raw current page number. The snapshot includes the current first and last visible page for spread-aware display, total count, previous and next availability, and first and last boundary state, so QML does not recompute spread boundaries.

For image-mode scopes, the thumbnail strip uses the image-document page candidate names. Directory and archive collection names may be collection-relative paths so that same-basename items in different folders remain distinguishable.

When a new directory collection, archive collection, or ordinary direct media scope is being listed and KiriView has no confirmed supported item list for that same scope yet, the current page number and total item count are unknown, and KiriView does not treat the current item as the first or last item.

During that unknown interval, KiriView also exposes an empty thumbnail strip rather than a partial or stale item list.

Entering a page number and pressing Enter or clicking the image viewing area opens the nearest valid page, returns focus to the image viewing area, and restores viewer keyboard shortcuts.

If the entered text cannot be parsed as a number, KiriView leaves the current item open and restores the displayed page number.

Pressing Escape while editing the page number cancels the edit, restores the displayed page number, returns focus to the image viewing area, and does not leave fullscreen.

When KiriView has a confirmed supported item list for the current scope, Previous, Next, First, Last, and page number entry remain available while a selected image or video is still loading.

During that loading interval, the page number shown in the UI is the most recent valid page selection requested by the user.

The displayed image URL or direct video source URL continues to mean the media item actually being shown.

For image selections that are still loading and are not yet displayed from predecode, the displayed image URL is empty even though the source URL and active navigation selection already refer to the selected target.

If users make multiple page selections before loading finishes, only the most recent selection is displayed.

If that replacement load fails, KiriView reports the selected target's error state and page navigation remains on the selected target when the target belongs to the confirmed active navigation list.

During empty startup, loading intervals without a confirmed same-scope selection, and mode switches, KiriView clears or marks unknown the active navigation projection before exposing any new readout. Values from the previous document are not reused for the current number, total count, editability, dispatch availability, or boundary state.

When moving between items in the current scope, the page navigation controls keep their layout stable.

The current page number updates to the newly displayed item, and the known total item count remains visible while KiriView updates the current position.

## Adjacent Media

When an ordinary direct image or video is open, Page Up opens the previous supported media file in the same ordinary direct media URL scope and Page Down opens the next one.

For direct media, sibling discovery may be asynchronous. The cursor used for the eventual active navigation readout is the session-owned direct media URL requested for the active open operation, not an image-document displayed URL observed before image replacement has completed.

After sibling discovery succeeds for the active ordinary direct media scope, the toolbar readout becomes known and shows the current supported media item number and total supported media item count for that scope. Confirming that a pending direct image request is now displayed does not make an already-started sibling discovery result stale when the effective direct media URL and parent scope are unchanged.

If the current media item is the first supported media item, pressing Page Up keeps the current item open and notifies the user that it is the first media item.

If the current media item is the last supported media item, pressing Page Down keeps the current item open and notifies the user that it is the last media item.

Supported image extensions match the open dialog: AVIF (`.avif` and `.avifs`), BMP, camera RAW (`.3fr`, `.arw`, `.bay`, `.bmq`, `.cr2`, `.cr3`, `.crw`, `.cs1`, `.cs2`, `.dcr`, `.dng`, `.erf`, `.fff`, `.iiq`, `.k25`, `.kdc`, `.mdc`, `.mef`, `.mos`, `.mrw`, `.nef`, `.nrw`, `.orf`, `.pef`, `.raf`, `.raw`, `.rdc`, `.rwl`, `.rw2`, `.sr2`, `.srf`, `.srw`, and `.x3f`), GIF, HEIF (`.heic`, `.heics`, `.heif`, `.heifs`, `.hif`, `.avci`, and `.hej2`), JPEG, JPEG 2000 (`.jp2`), JPEG XL (`.jxl`), PNG, SVG, TIFF (`.tif` and `.tiff`), and WebP, case-insensitively.

JPEG-compressed HEIF files use the generic HEIF extensions because they do not have a dedicated extension.

Supported direct video extensions are MP4 (`.mp4`), M4V (`.m4v`), and MOV (`.mov`), case-insensitively.

An ordinary direct media URL scope is the non-recursive parent URL of the active direct media URL. This includes ordinary local parent directories and KDE archive URL parent locations such as `zip:///path/archive.zip!/chapter/`.

The ordinary direct media URL parent follows KiriView's existing direct image candidate context rule rather than a new URL-scheme-specific parser: KiriView normalizes the original direct URL through the same `navigationSourceUrl(...)` path used for displayed direct images, then derives the parent with `QUrl::RemoveFilename | QUrl::NormalizePathSegments`.

When an image or video is opened from a KDE-supported archive URL such as `zip://`, `tar://`, or `sevenz://`, KiriView treats the opened item as a single direct media URL, and navigation moves between supported media files in the same directory inside that archive URL.

When an image or unsupported-video placeholder is displayed from a local CBZ, CBT, CB7, CBR, ZIP, TAR, 7Z, or RAR archive collection opened directly by KiriView, navigation moves between all supported image and video files inside that archive collection, including supported media in subdirectories.

When an image or unsupported-video placeholder is displayed from a local directory collection opened directly by KiriView, navigation moves between all supported image and video files inside that directory tree, including supported media in subdirectories.

After the archive or directory collection has been listed, page navigation uses all supported image and video files inside that opened collection as its navigation target set.

Supported video entries inside directly opened archive collections and directly opened directory collections are valid opened collection navigation items. KiriView does not play those videos while an opened collection scope is active; selecting one keeps image mode active and shows an unsupported-video placeholder with the message `KiriView can’t play videos inside directly opened archives or directories yet.` Entering that placeholder also shows the same text as an in-app toast.

If the parent URL cannot be listed, the current media item is not found, or no adjacent supported media item exists, the current media item remains open and the app remains ready for another open action.

## Sorting and Boundary Feedback

The previous and next files are determined by sorting candidate names with the user's locale-aware file name order.

For ordinary direct media navigation, the candidate name is the file name.

For archive and directory collections opened directly by KiriView, candidate names are collection-relative paths such as `foo/a.jpg` and `bar/a.jpg`.

Navigation does not wrap. Pressing Page Up on the first candidate or Page Down on the last candidate keeps the current item open and notifies the user that it is the first or last media item in ordinary direct media scopes and the first or last item in archive collection or directory collection scopes.

Boundary feedback may be requested by configured shortcuts and fixed viewer navigation paths even when visible Previous or Next menu and toolbar placements are disabled at the boundary.

KiriView shows those first-item and last-item notifications only when the current supported list is known and the current item is at a known boundary.

Previous Archive and Next Archive are collection navigation actions for the current opened collection. When users trigger Previous Archive or Next Archive from a menu or configured shortcut and KiriView confirms that no adjacent collection exists in that direction, the current collection remains open and KiriView shows an in-app toast: `No previous collection` or `No next collection`.

KiriView has one visible in-app toast notification slot.

New toast requests replace the current toast and replay the entrance animation, including when the same message and scope are requested again while already visible.

A first-item or last-item toast is scoped to the currently displayed item's known boundary.

When the displayed item changes or is cleared, KiriView removes the current toast only if its scope is the boundary. Non-boundary toasts such as file operation errors remain governed by normal replacement and timeout behavior.

## Panning and Viewer Keys

When a ready image is larger than the viewport at the current zoom, horizontal and vertical scrollbars allow panning across the image.

When the image is smaller than the viewport, it remains centered.

KiriView derives scrollbar visibility, drag-panning availability, scan availability, and Left/Right panning fallback from the active viewport geometry frame. QML layout measurements must not independently decide whether an image is pannable.

While the page number or zoom input is not focused, the plain arrow keys are fixed viewer-only shortcuts for keyboard panning and physical adjacent navigation. They are not user-configurable actions and are not listed in Keyboard Shortcuts configuration, shortcut help, or menus.

Fixed viewer-only shortcuts use the same runtime UI gates as configurable application actions. They are inactive while text editing, modal help, or another runtime-suppressed UI state is active.

When UI focus, modal state, active viewport, or transient tool focus changes rapidly, fixed viewer-only shortcuts and configurable shortcuts use the newest observed gate state. Older focus or modal observations must not re-enable or disable a shortcut after a newer observation has already taken effect.

When an image is horizontally pannable at the current zoom, Left and Right pan the image within the available horizontal scroll bounds.

When the current image is not horizontally pannable, Left opens the previous supported media item and Right opens the next supported media item with the same boundary behavior as the Previous and Next actions in an ordinary direct media scope.

In video mode, Left opens the previous supported media item and Right opens the next supported media item in the ordinary direct media scope. They do not seek within the video.

In Right-to-Left Reading mode, Left and Right keep physical horizontal panning while the image can pan horizontally, but their non-pannable ordinary direct media navigation fallback is reversed: Left opens the next supported media item and Right opens the previous supported media item.

When an image is zoomed large enough to pan in any direction, Up and Down pan the image vertically within the available scroll bounds and have no image-navigation fallback. `Shift+,` and `Alt+Home` move the current content to its scan start position, and `Shift+.` and `Alt+End` move the current content to its scan end position. For images, scan start and scan end are the same geometry-aware positions used by scan navigation, including Right-to-Left Reading order. The mouse cursor shows that the image can be dragged to pan.

Keyboard panning and Left/Right viewer navigation are inactive while the page number or zoom input is focused.

## Video Seeking

Timeline dragging and scrubbing is the primary way to seek within the current video.

If keyboard focus is inside the timeline control, that control may handle its own keyboard interaction.

Video mode supports fixed local seek shortcuts: `Alt+Left` seeks backward 5 seconds, `Alt+Right` seeks forward 5 seconds, `Alt+Up` seeks forward 45 seconds, and `Alt+Down` seeks backward 45 seconds.

Video mode supports configurable viewer-local current-content boundary shortcuts: `Shift+,` and `Alt+Home` seek to the start of the current video, and `Shift+.` and `Alt+End` seek to the end of the current video when the video is seekable and has a known positive duration.

Video seek shortcuts are video-mode-only and do not affect image mode, archive collection scope, or directly opened directory collection scope.

Video seek shortcuts are best-effort time seeks through Qt Multimedia position seeking. They run only when the media is seekable, clamp to the valid `[0, duration]` range when duration is known, and do not promise frame-accurate seeking.

The actual landed position may be adjusted by the Qt Multimedia backend, commonly to a nearby decodable or keyframe position.

## Scan Shortcuts

When an image is zoomed large enough to pan, `.` or `Space` scans forward through the image from left to right and then top to bottom.

Each scan step moves horizontally or vertically by at most seven eighths of the visible viewport, except that moving from the right edge of one row to the next row jumps directly to the left edge of the next row.

At the final scan position, `.` or `Space` opens the next image.

`,` or `Shift+Space` scans backward through the same positions. At the initial scan position, it opens the previous image, starting that image at its final scan position.

When the current image is not zoomed large enough to pan, `.` or `Space` opens the next image and `,` or `Shift+Space` opens the previous image.

In video mode, `.` or `Space` opens the next supported media item and `,` or `Shift+Space` opens the previous supported media item in the ordinary direct media scope. They do not pan or seek within the video.

In Right-to-Left Reading mode, scan order starts at the top-right and proceeds toward the bottom-left. The forward scan shortcuts still scan forward or open the next image, the backward scan shortcuts still scan backward or open the previous image, `Shift+,` and `Alt+Home` jump to the current content start, and `Shift+.` and `Alt+End` jump to the current content end.

## Background Loading

After an image is displayed, KiriView may make adjacent images available for quicker Previous or Next navigation, so the switch can happen without showing a full-page loading state.

In an ordinary direct media URL scope, direct videos act as navigation positions for this background image loading even though video frames are not predecoded. While a direct video is current, KiriView may continue making nearby supported image files available for quick Previous or Next navigation.

This background work must not change what is displayed until the user opens an adjacent image.

When Two-Page Spread shows two pages, both the current primary page and the visible secondary page are treated as displayed images for this background work.

When users move quickly through pages, KiriView may briefly postpone this background work and then prioritize pages around the page where navigation settles, rather than every skipped page.

Directly opened archive and directory collections may make more pages available in the current reading direction than ordinary image navigation. Opened collection video items are positions for navigation and predecode planning, but KiriView predecodes only nearby supported images.

When the desktop Power Saver mode is enabled, KiriView does not newly schedule or run background work for adjacent pages.

Power Saver mode does not prevent recently displayed images from staying available for quick navigation.

Foreground image loading and visible image detail updates continue to work while Power Saver mode is enabled.
