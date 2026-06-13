# Video Playback

## Product Boundary

KiriView supports direct video files as moving, sound-capable image-like media items in the MVP.

Supported direct video inputs may come from the startup argument, drop, open dialog selection, or ordinary adjacent direct media navigation from a direct media URL scope.

The MVP direct video format list is MP4, M4V, and MOV, case-insensitively. These cover common camera and phone video files without expanding the initial scope to less common container behavior.

Direct video playback supports direct file-like URLs that KiriView can hand to KDE/KIO as ordinary file items, including local paths, `file://` URLs, and KDE-supported archive URLs that point at a file entry such as `zip:///path/archive.zip!/clip.mp4`.

Direct KDE archive-entry URLs are not KiriView archive collection scope. KiriView may open `zip:///path/archive.zip!/clip.mp4` as a direct media URL while still opening `/path/archive.zip` as an archive collection when the archive itself is selected.

The product boundary is based on KiriView's opened collection state, not on URL scheme alone. If KiriView has opened an archive or directory as an opened collection, supported video entries inside that collection are opened collection navigation items with an unsupported-video placeholder and are not played. If KiriView is handling a direct media URL without an active archive or directory collection context, that URL remains eligible for video support even when its scheme is a KDE archive scheme.

Video files do not participate in video-frame predecode, video-frame image cache, editable image zoom, image pan, image rotate, two-page spread pairing, or Right-to-Left Reading.

In ordinary direct media URL scopes, showing a video must not clear or stop the background predecode/cache lifecycle for nearby supported image files. KiriView may keep and continue predecoding adjacent still images around the current video cursor, but it must not attempt to predecode the video itself.

Archive-collection-internal video playback, directly opened directory-collection video playback, collection-internal video metadata, playlists, subtitles, track selection, frame stepping, and timeline preview thumbnails are out of scope.

KiriView advertises direct video support through the desktop file for the MIME types that cover the MVP MP4, M4V, and MOV format list: `video/mp4` and `video/quicktime`.

## Source URL Identity

KiriView may internally resolve a KIO-backed direct video URL to a local playback URL before handing it to the video backend, such as through KIOFuse or another KIO-backed resolver. Resolution succeeds only when the resolved playback URL can be consumed by the video backend; otherwise the video load fails while keeping the original direct media URL as the public source and error context.

When direct video playback URL resolution fails, the user-facing error text is a stable KiriView message that the selected video could not be opened. Backend, DBus, KIO, and Qt resolver messages are internal diagnostics and are not displayed as the primary error text.

Internal playback URL resolution must not change the user-facing source URL. Window title, adjacent direct media navigation, deletion target, error context, and direct-media versus opened-collection routing decisions remain based on the original direct media URL.

Resolver-local playback URLs are video-only. Opening a direct image after a direct video must route the image through the image document with the original image URL, not through the previous video playback URL or another KIOFuse local playback URL.

Direct video embedded metadata is parsed from the resolved playback URL when it is local and from the original direct local URL otherwise. Parsed metadata may populate the Info Panel while keeping the original direct media URL as the user-facing source identity.

Opening a video after an image must not route the video through `KiriImageDocument`.

Opening an image after a video restores image behavior.

The top-level document session owns the active document kind for direct image and video routing. Its public source URL follows the same user-facing identity as the active image or video document: assigning it starts an open request, successful opens expose the original direct media URL, direct image replacement failures keep the failed target as the session source and error context, direct video source-load failures keep the original direct media URL as the public source and error context, and resolver-local playback URLs are never exposed as the session source.

For direct image and direct video URLs in direct media scopes, the document session owns the active navigation projection used by toolbar readouts, shared action availability, and navigation dispatch. Asynchronous sibling discovery uses the requested session-owned direct media URL as the cursor for the eventual readout, not an image-document displayed URL.

## Ordinary Direct Media URL Scope

An ordinary direct media URL scope is the non-recursive parent URL of the active direct media URL.

This includes ordinary local parent directories and KDE archive URL parent locations such as `zip:///path/archive.zip!/chapter/`.

Opening a directory URL creates the existing directory collection and does not create a video-capable direct media scope.

The ordinary direct media URL parent follows KiriView's existing direct image candidate context rule rather than a new URL-scheme-specific parser: KiriView normalizes the original direct URL through the same `navigationSourceUrl(...)` path used for displayed direct images, then derives the parent with `QUrl::RemoveFilename | QUrl::NormalizePathSegments`.

Direct KDE archive-entry URLs use this direct URL branch unless KiriView has explicitly opened an archive or directory collection scope.

Supported image files and supported direct video files share one locale-aware sibling order in ordinary direct media URL scopes.

Direct media navigation is non-recursive, does not wrap, and uses the same session-owned active navigation projection and boundary-feedback model as image-document page navigation. Boundary feedback in a direct media scope uses neutral first-media-item and last-media-item wording rather than calling every boundary item an image.

The top-level document session dispatches adjacent Previous and Next through direct media navigation in video mode and in image mode only when the active image belongs to an ordinary direct media URL scope. Archive collection and directly opened directory collection scopes keep opened-collection navigation as the session projection's internal source.

## Playback

Opening a video starts playback automatically.

When a video load or backend error is superseded by a later accepted non-error playback state for the same video, KiriView clears the public video error text. Public video states such as Loading, Ready, or Null must not expose stale error text from an earlier failed backend state.

Video mode shows a video viewport and a Breeze-style playback control panel at the bottom edge of the video viewport.

The regular toolbar remains available in video mode for application menu access and ordinary direct media navigation. It shows Previous and Next controls, the current media item number, the total supported media item count for the ordinary direct media URL scope when that list is known, and the same trailing control order as image mode, all from the document session's active navigation projection.

Video mode keeps image-only toolbar controls visible in their image-mode positions but unavailable: Right-to-Left Reading, Two-Page Spread, and Fit are disabled, and the zoom control is read-only.

Video mode shows a read-only zoom percentage when the video frame size, displayed video content rectangle, and target window effective device pixel ratio are known. The value is the current fitted display size in physical pixels relative to the video's intrinsic frame size. When the percentage is unknown, the read-only zoom control displays `? %`. Users cannot edit this value or use image zoom actions for video.

The playback control panel is shown only when the current video is ready and has a video track.

The playback control panel includes icon-only play/pause and mute/unmute buttons, current time, timeline position selection and scrubbing, total duration, and a disabled non-interactive timeline state when the media is not seekable. The timeline row order is play/pause, current time, timeline, total duration, and mute/unmute. Time readouts use fixed-width digits and are the only visible text in the panel.

Muting is session state owned by the video document. Toggling mute affects the current backend audio output and persists across video source changes during the app session, including when a new media backend is created lazily for a later source.

In regular windowed and fullscreen video mode, the default playback control style is a floating panel inside the video viewport, aligned to the bottom edge with a large-spacing bottom margin. The floating panel uses the active Kirigami background color with theme-aware translucency, the Kirigami corner radius, a weak shadow, centered content width, and a maximum width of about 75% of the viewport while preserving viewport side margins.

The playback controls switch to a fixed bottom bar inside the video viewport when the viewport is compact, touch/mobile input is active, or system animations are disabled or reduced. Compact means roughly narrower than 32 grid units or shorter than 16 grid units. The fixed bottom bar is full-width, has no floating bottom margin, has no shadow, has square outer bottom corners, and reserves its height from the video display area so the video is not covered by the controls.

When video playback reaches the natural end of the media, KiriView keeps the direct video output as the active presentation and must not clear the video output into an empty or null-like visual state. Playback stops at the final position. Pressing Play from that ended state restarts playback from the beginning when seeking is available.

The floating playback panel must not reserve page layout height and must remain usable in fullscreen.

Auto-hide applies only in floating control mode while video is playing. Pointer movement, hover, focus, slider drag, button press, tap, or paused playback reveals the controls and keeps them visible while interaction continues. When controls are eligible to hide, they fade after a human-moment delay. If system animations are disabled or reduced, fixed bottom bar mode is used and controls do not auto-hide.

In Flatpak, KiriView exposes the host PipeWire runtime socket for Qt Multimedia video playback initialization while keeping PulseAudio-compatible audio output available.

During timeline dragging, the user's drag position remains visually stable instead of being overwritten by ordinary playback position updates.

Unknown duration, invalid duration, and non-seekable media produce a stable disabled timeline state rather than invalid or flickering values.

## Video Navigation and Seeking

In video mode, viewer Left and Right and existing adjacent navigation actions move to the previous or next supported media item in the ordinary parent location. They do not seek within the video.

In video mode, the shared scan shortcuts also use adjacent navigation: `.` and `Space` move to the next supported media item, while `,` and `Shift+Space` move to the previous supported media item. They do not pan or seek within the video.

Video mode supports the shared configurable shortcuts for Open, Move to Trash, Delete Permanently, Previous Media Item, Next Media Item, First Media Item, Last Media Item, Current Content Start, Current Content End, Fullscreen, Keyboard Shortcuts, Configure Shortcuts, Show Menubar, and Quit. Shared direct media navigation shortcuts use session active-navigation dispatch and availability.

The Current Content Start and Current Content End shortcuts are viewer-local configurable video seek actions. By default, `Shift+,` seeks to position `0 ms` in the current video, and `Shift+.` seeks to the video's known positive duration. They are distinct from the fixed `Alt+Arrow` video seek shortcuts.

`P` toggles play/pause for the current video using the same playback command as the playback panel button.

When an image-only configurable shortcut is pressed in video mode, KiriView does not trigger the image action and shows the in-app toast `This action is not available for videos`. Repeated unsupported video shortcut presses update the same toast instance.

When the video play/pause configurable shortcut is pressed while a ready image is displayed, KiriView does not trigger video playback and shows the in-app toast `This action is not available for images`. Empty, loading, error, and unsupported-placeholder states do not show this image unavailable-action toast.

Disabled image-only action placements in video mode do not route through image-document commands. Only the unsupported-video shortcut interception produces the unavailable-action toast.

Timeline dragging and scrubbing is the primary way to seek within the current video.

If keyboard focus is inside the timeline control, that control may handle its own keyboard interaction.

Video mode also supports fixed local seek shortcuts: `Alt+Left` seeks backward 5 seconds, `Alt+Right` seeks forward 5 seconds, `Alt+Up` seeks forward 45 seconds, and `Alt+Down` seeks backward 45 seconds.

The configurable current-content start and end shortcuts follow the same media seekability gates as timeline seeking. Current Content End is unavailable when the duration is unknown, zero, or invalid.

Video seek shortcuts are video-mode-only and must not affect image mode, archive collection scope, or directly opened directory collection scope.

Video seek shortcuts are best-effort time seeks through Qt Multimedia position seeking. They run only when the media is seekable, clamp to the valid `[0, duration]` range when duration is known, and must not promise frame-accurate seeking.

The actual landed position may be adjusted by the Qt Multimedia backend, commonly to a nearby decodable or keyframe position.

## Deletion

Direct video files support the same user-facing Move to Trash and Delete Permanently actions as images.

The deletion target for a direct video is the original direct media URL, even if KiriView resolved a separate local playback URL internally.

If deletion is canceled, the current video remains open and no notification is shown.

If deletion fails, the current video remains open and the file operation error is shown as an in-app toast notification.

After successful video deletion, playback stops and KiriView opens the next supported media item in the current direct media scope when possible, falls back to the previous supported media item when no next item exists, and otherwise returns to empty state.

In image mode, ordinary direct media deletion uses the same direct-media fallback order, so deleting an image can open a neighboring supported video. Archive collection and directly opened directory collection image deletion keep their image and collection-specific fallback behavior.
