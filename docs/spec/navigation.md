# Navigation

## Page Controls

### Active Navigation Scope

Toolbar readouts, page-number entry, shared Previous, Next, First, and Last actions, menus, and shortcuts use one active navigation scope at a time.

For ordinary direct media URL scopes, including direct image files, direct video files, and KDE archive-entry media URLs opened as individual media items, page controls follow only the ordinary direct media sibling list.

Page navigation for opened collections is active only for directly opened archive collections and directly opened directory collections.

For ordinary direct media URL scopes, archive collection scopes, and directly opened directory collection scopes, the page navigation controls count and select supported images and supported videos together.

The active navigation state has these user-visible invariants: `available` means the current mode exposes a navigable scope; `known` means KiriView has a confirmed current position and total count for the active scope; `editable` means entering a page number can open an item in that same scope. When navigation is unavailable or unknown, the page-number entry is disabled, Previous, Next, First, and Last are disabled, and KiriView does not display a stale current/count pair. When navigation is known, the current number is within `1..total`, the total count is at least 1, previous and next availability match whether a previous or next target exists in reading order, and first/last boundary state matches whether the active visible item or spread is at the known start or end of the scope.

For image-mode scopes, page controls account for spread-aware display: the visible item may cover a first and last visible page, and Previous, Next, First, and Last availability follow those visible spread boundaries rather than a single raw page number.

### Toolbar Page Controls

The toolbar provides page navigation with Previous and Next actions, the current page number, the total number of supported items in the current scope after that scope's supported list has been confirmed, and editable page number entry.

First and Last are page navigation actions available through their configured shortcuts and menus. They open the first or last known page or media item in the current scope.

The Previous action placement is disabled on the first item, and the Next action placement is disabled on the last item.

Page numbers are shown to users starting at 1.

### Thumbnail Strip Eligibility

When the active navigation list is known, the thumbnail strip exposes one item per supported active navigation item with the same ordering and 1-based numbering as the toolbar readout and page-number entry.

The active navigation thumbnail strip may display generated preview thumbnails for supported direct local image items, supported direct local video items, and supported image pages inside CBZ and directly opened ZIP archive collections. Generated thumbnails are representative previews for navigation, not authoritative media content. Supported direct local video thumbnails may use an embedded video cover or thumbnail image when available. When no usable embedded image is available, KiriView may choose a representative decoded video frame intended to avoid uninformative first frames; it does not promise an exact frame or timestamp. Video thumbnail orientation and mirroring follow the embedded image or decoded frame supplied by the platform. Unsupported, pending, failed, non-local direct video, direct archive-entry media, CB7/7z and other non-ZIP-backed archive-collection items, and directory-collection items keep placeholder media-type icons instead of generated preview thumbnails.

The active navigation thumbnail strip chooses generated preview quality from the thumbnail's physical display size, including the current device pixel ratio. During panel resize, fractional-scale changes, or movement between screens with different scale factors, a previously ready smaller thumbnail may remain visible as an interim fallback while KiriView loads a newly required larger thumbnail, and the strip replaces it only after the newer result is ready. If the newer request fails, KiriView keeps an existing usable thumbnail visible when one is available; otherwise the row uses the normal fallback icon path.

After thumbnails needed for visible, nearby, and user-selected navigation items have been satisfied, KiriView may progressively replace additional eligible placeholder thumbnails in the current active navigation list without user action. Visible, nearby, and user-selected navigation items take precedence over not-yet-visible thumbnail filling. Failures for not-yet-visible thumbnails do not show user-facing errors and keep the normal thumbnail fallback UI.

For image-mode scopes, the thumbnail strip uses page candidate names. Directory and archive collection names may be collection-relative paths so that same-basename items in different folders remain distinguishable.

### Thumbnail Strip Scrolling

The active navigation thumbnail strip keeps the active item visually selected. When adjacent main-view navigation changes the active item, the strip uses a preferred visible zone inset from the viewport edges: if the selected thumbnail remains inside that zone, the strip keeps its scroll position unchanged, and if it leaves that zone, the strip may reveal it back toward a stable visible position. Adjacent Next navigation places the selected thumbnail toward the leading side of the preferred zone so more following items remain visible; adjacent Previous navigation places it toward the trailing side so more preceding items remain visible. This anchoring follows the semantic Previous or Next action, including when Right-to-Left Reading mode maps physical keys differently. Nearby automatic reveal may use a short easing animation, but at most one automatic reveal target is active at a time: repeated navigation replaces older pending scroll movement with the latest selected item, and rapid navigation may update the strip immediately instead of animating each step. Direct thumbnail activation preserves the thumbnail strip scroll position unless model or layout changes require immediate containment. User-initiated thumbnail scrolling temporarily suppresses automatic preferred-zone follow movement unless the selected thumbnail would otherwise leave the visible viewport. Large jumps such as First, Last, and page-number entry may reposition the strip to keep the selected thumbnail discoverable. Opening or loading a new scope may reveal its selected thumbnail, but KiriView does not replay stale or duplicate automatic thumbnail scrolling. KiriView does not promise exact thumbnail pixel offsets, centered positioning, or animation duration.

Activating a thumbnail strip item opens the item at that active navigation number, matching toolbar page-number entry behavior.

### Page Number Editing

Entering a page number and pressing Enter or clicking the image viewing area opens the nearest valid page, returns focus to the image viewing area, and restores viewer keyboard shortcuts.

If the entered text cannot be parsed as a number, KiriView leaves the current item open and restores the displayed page number.

Pressing Escape while editing the page number cancels the edit, restores the displayed page number, returns focus to the image viewing area, and does not leave fullscreen.

### Pending Selection And Loading

When a new directory collection, archive collection, or ordinary direct media scope is being listed and KiriView has no confirmed supported item list for that same scope yet, the current page number and total item count are unknown, and KiriView does not treat the current item as the first or last item.

During that unknown interval, KiriView also exposes an empty thumbnail strip rather than a partial or stale item list.

When KiriView has a confirmed supported item list for the current scope, Previous, Next, First, Last, and page number entry remain available while a selected image or video is still loading.

During that loading interval, the page number shown in the UI is the most recent valid page selection requested by the user.

The active navigation selection and the displayed media item may temporarily differ. The active navigation readout, selected thumbnail, and page-number entry follow the most recent valid page selection, while the displayed image URL or direct video source URL continues to mean the media item actually being shown.

For image selections that are still loading and are not displayed from a prepared image, KiriView may keep the previous committed image visible when the selection stays inside the same active navigation scope. In that case the displayed image URL continues to identify the previous committed image until the selected image replaces it. If no previous committed image is retained, the displayed image URL is empty even though the source URL and active navigation selection already refer to the selected target.

If users make multiple page selections before loading finishes, only the most recent selection is displayed.

If that replacement load fails, KiriView reports the selected target's error state and page navigation remains on the selected target when the target belongs to the confirmed active navigation list.

During empty startup, loading intervals without a confirmed same-scope selection, and mode switches, KiriView clears or marks unknown the active navigation state before exposing any new readout. Values from the previous document are not reused for the current number, total count, editability, action availability, or boundary state.

When moving between items in the current scope, the page navigation controls keep their layout stable.

The current page number updates to the newly selected item, and the known total item count remains visible while KiriView prepares and commits the selected media item.

## Adjacent Media

When an ordinary direct image or video is open, Page Up opens the previous supported media file in the same ordinary direct media URL scope and Page Down opens the next one.

For direct media, sibling discovery may be asynchronous. The eventual active navigation readout follows the direct media URL requested for the active open operation, not a stale or empty displayed image URL observed before image replacement has completed.

After sibling discovery succeeds for the active ordinary direct media scope, the toolbar readout becomes known and shows the current supported media item number and total supported media item count for that scope. Confirming that a pending direct image request is now displayed does not make an already-started sibling discovery result stale when the effective direct media URL and parent scope are unchanged.

If the current media item is the first supported media item, pressing Page Up keeps the current item open and notifies the user that it is the first media item.

If the current media item is the last supported media item, pressing Page Down keeps the current item open and notifies the user that it is the last media item.

Adjacent media candidates use the supported image and direct video source formats defined in File Access.

An ordinary direct media URL scope is the non-recursive parent URL of the active direct media URL. This includes ordinary local parent directories and KDE archive URL parent locations such as `zip:///path/archive.zip!/chapter/`.

The ordinary direct media URL parent is derived from the original direct media URL after KiriView applies its normal navigation-source handling. Local and KDE archive-entry URLs keep their user-visible containing location as the direct media scope.

When an image or video is opened from a KDE-supported archive URL such as `zip://`, `tar://`, or `sevenz://`, KiriView treats the opened item as a single direct media URL, and navigation moves between supported media files in the same directory inside that archive URL.

When an image, playable collection video, or unsupported-video placeholder is displayed from a local CBZ, CBT, CB7, CBR, ZIP, TAR, 7Z, or RAR archive collection opened directly by KiriView, navigation moves between all supported image and video files inside that archive collection, including supported media in subdirectories.

When an image or playable collection video is displayed from a local directory collection opened directly by KiriView, navigation moves between all supported image and video files inside that directory tree, including supported media in subdirectories.

After the archive or directory collection has been listed, page navigation uses all supported image and video files inside that opened collection as its navigation target set.

Supported video entries inside directly opened archive collections and directly opened directory collections are valid opened collection navigation items. Eligible stored ZIP and plain TAR archive entries open in video mode and play inside the opened archive collection scope. Supported directory collection video entries open in video mode and play inside the opened directory collection scope. Ineligible archive video entries keep image mode active and show an unsupported-video placeholder with the message `KiriView can’t play this video from the selected collection.` Entering that placeholder also shows the same text as an in-app toast.

If the parent URL cannot be listed, the current media item is not found, or no adjacent supported media item exists, the current media item remains open and the app remains ready for another open action.

## Sorting and Boundary Feedback

The previous and next files are determined by sorting candidate names with the user's locale-aware file name order.

For ordinary direct media navigation, the candidate name is the file name.

For archive and directory collections opened directly by KiriView, candidate names are collection-relative paths such as `foo/a.jpg` and `bar/a.jpg`.

Navigation does not wrap. Pressing Page Up on the first candidate or Page Down on the last candidate keeps the current item open and notifies the user that it is the first or last media item in ordinary direct media scopes and the first or last item in archive collection or directory collection scopes.

Boundary feedback may be requested by configured shortcuts and fixed viewer navigation paths even when visible Previous or Next menu and toolbar placements are disabled at the boundary.

KiriView shows those first-item and last-item notifications only when the current supported list is known and the current item is at a known boundary.

Previous Archive and Next Archive are comic-book archive navigation actions for the current directly opened comic book archive. When archive navigation is available and users trigger Previous Archive or Next Archive from a menu or configured shortcut, KiriView confirms sibling comic book archives in that direction; if none exists, the current archive remains open and KiriView shows an in-app toast: `No previous collection` or `No next collection`.

KiriView has one visible in-app toast notification slot.

New toast requests replace the current toast and replay the entrance animation, including when the same message and scope are requested again while already visible.

A first-item or last-item toast is scoped to the currently displayed item's known boundary.

When the displayed item changes or is cleared, KiriView removes the current toast only if its scope is the boundary. Non-boundary toasts such as file operation errors remain governed by normal replacement and timeout behavior.

## Panning and Viewer Keys

When a ready image is larger than the viewport at the current zoom, horizontal and vertical scrollbars allow panning across the image.

When the image is smaller than the viewport, it remains centered.

KiriView derives scrollbar visibility, drag-panning availability, scan availability, and Left/Right panning fallback from the active viewport geometry. These affordances must change together and must not briefly disagree during layout or viewport updates.

While the page number or zoom input is not focused, the plain arrow keys are fixed viewer-only shortcuts for keyboard panning and physical adjacent navigation. They are not user-configurable actions and are not listed in Keyboard Shortcuts configuration, shortcut help, or menus.

Fixed viewer-only shortcuts use the same runtime UI gates as configurable application actions. They are inactive while text editing, modal help, or another runtime-suppressed UI state is active.

When UI focus, modal state, active viewport, or transient tool focus changes rapidly, fixed viewer-only shortcuts and configurable shortcuts use the newest observed gate state. Older focus or modal observations must not re-enable or disable a shortcut after a newer observation has already taken effect.

When an image is horizontally pannable at the current zoom, Left and Right pan the image within the available horizontal scroll bounds.

When the current image is not horizontally pannable, Left and Right dispatch Previous or Next against the active image navigation scope with the same boundary behavior as the shared Previous and Next actions.

In Left-to-Right Reading mode, Left opens the previous supported media item and Right opens the next supported media item.

In video mode, Left opens the previous supported media item and Right opens the next supported media item in the active navigation scope. They do not seek within the video.

In Right-to-Left Reading mode, Left and Right keep physical horizontal panning while the image can pan horizontally, but their non-pannable navigation fallback is reversed: Left opens the next supported media item and Right opens the previous supported media item.

When an image is zoomed large enough to pan in any direction, Up and Down pan the image vertically within the available scroll bounds and have no image-navigation fallback. `Shift+,` and `Alt+Home` move the current content to its scan start position, and `Shift+.` and `Alt+End` move the current content to its scan end position. For images, scan start and scan end are the same geometry-aware positions used by scan navigation, including Right-to-Left Reading order. The mouse cursor shows that the image can be dragged to pan.

Keyboard panning and Left/Right viewer navigation are inactive while the page number or zoom input is focused.

## Video Seeking

Video seeking behavior is specified in [Video Playback](video-playback.md#video-navigation-and-seeking).

Navigation-owned viewer shortcut gates also apply to video seek shortcuts. Seek shortcuts are inactive while text input, modal shortcut help, or another viewer-suppressed state is active.

## Scan Shortcuts

When an image is zoomed large enough to pan, `.` or `Space` scans forward through the image from left to right and then top to bottom.

Each scan step moves horizontally or vertically by at most seven eighths of the visible viewport, except that moving from the right edge of one row to the next row jumps directly to the left edge of the next row.

At the final scan position, `.` or `Space` opens the next image.

`,` or `Shift+Space` scans backward through the same positions. At the initial scan position, it opens the previous image, starting that image at its final scan position.

When the current image is not zoomed large enough to pan, `.` or `Space` opens the next image and `,` or `Shift+Space` opens the previous image.

In video mode, `.` or `Space` opens the next supported media item and `,` or `Shift+Space` opens the previous supported media item in the active navigation scope. They do not pan or seek within the video.

In Right-to-Left Reading mode, scan order starts at the top-right and proceeds toward the bottom-left. The forward scan shortcuts still scan forward or open the next image, the backward scan shortcuts still scan backward or open the previous image, `Shift+,` and `Alt+Home` jump to the current content start, and `Shift+.` and `Alt+End` jump to the current content end.

## Background Loading

After an image is displayed, KiriView may make adjacent images available for quicker Previous or Next navigation, so the switch can happen without showing a full-page loading state.

In an ordinary direct media URL scope, direct videos act as navigation positions for this background image loading even though video frames are not prepared for image-style quick navigation. While a direct video is current, KiriView may continue making nearby supported image files available for quick Previous or Next navigation.

This background work must not change what is displayed until the user opens an adjacent image.

When users select another image in the current active navigation scope, KiriView may reprioritize background loading around the newly selected target before that image is displayed.

When Two-Page Spread shows two pages, both the current primary page and the visible secondary page are treated as displayed images for this background work.

When users move quickly through pages, KiriView may briefly postpone this background work and then prioritize pages around the page where navigation settles, rather than every skipped page.

Directly opened archive and directory collections may make more pages available in the current reading direction than ordinary image navigation. Opened collection video items are positions for navigation and background-loading planning, but KiriView prepares only nearby supported images and does not prepare video frames for image-style quick navigation.

When the desktop Power Saver mode is enabled, KiriView does not newly schedule or run background work for adjacent pages.

Power Saver mode does not prevent recently displayed images from staying available for quick navigation.

Foreground image loading and visible image detail updates continue to work while Power Saver mode is enabled.
