# Video Playback

## Product Boundary

KiriView supports the direct video files and eligible video entries inside directly opened collections defined in [File Access](file-access.md#supported-sources) as moving, sound-capable image-like media items.

Supported direct video inputs may come from the startup argument, drop, open dialog selection, or ordinary adjacent direct media navigation from a direct media URL scope.

Direct KDE archive-entry URLs are not KiriView archive collection scope. KiriView may open `zip:///path/archive.zip!/clip.mp4` as a direct media URL while still opening `/path/archive.zip` as an archive collection when the archive file itself is selected.

The product boundary is based on KiriView's opened collection state, not on URL scheme alone. If KiriView has opened an archive as an opened collection, supported video entries inside that collection are opened collection navigation items; eligible stored ZIP and plain TAR entries are played, while ineligible archive video entries show the unsupported-video placeholder. If KiriView has opened a directory as an opened collection, supported video entries inside that directory are opened collection navigation items and are played from the directory entry. If KiriView is handling a direct media URL without an active archive or directory collection context, that URL remains eligible for direct video support even when its scheme is a KDE archive scheme.

Video files do not participate in video-frame preparation for image-style quick navigation, editable image zoom, image pan, image rotate, or two-page spread pairing.

In ordinary direct media URL scopes, showing a video must not clear or stop background preparation for nearby supported image files. KiriView may keep and continue preparing adjacent still images around the current video cursor, but it must not prepare the video itself as still-image quick-navigation data.

Opened collection video playback is limited to eligible stored ZIP archive entries, plain TAR archive entries, and supported video files inside directly opened local directory collections. Collection-internal video metadata, playlists, subtitles, track selection, frame stepping, and timeline preview thumbnails are not provided.

## Source URL Identity

The public source URL for direct image and direct video URLs in direct media scopes is the direct media URL requested by the user, startup input, drop input, open dialog, or navigation action.

Assigning a direct media source starts an open request for that URL. Successful direct image and direct video opens expose the requested direct media URL as the public source. Direct image replacement failures keep the failed target as the public source and error context. Direct video load or playback-preparation failures keep the original direct media URL as the public source and error context.

When playback preparation fails, the user-facing error text is a stable KiriView message that the selected video could not be opened. Platform and playback diagnostic messages are internal diagnostics and are not displayed as the primary error text.

Playback preparation details are never exposed as the public source URL. For direct videos, window title, adjacent direct media navigation, deletion target, error context, and direct-media routing decisions remain based on that public source URL.

Opened collection video playback keeps the collection entry URL as the user-facing source identity. Collection routing and collection-level operations remain based on the opened collection context, and playback preparation details do not create an ordinary direct media scope.

Direct video embedded metadata may populate the Info Panel when available while keeping the original direct media URL as the user-facing source identity.

Collection-internal video metadata is not parsed for the Info Panel.

Opening a video after an image switches to video behavior rather than image behavior.

Opening a direct image after a direct video uses the image's own URL and restores image behavior.

For direct image and direct video URLs in direct media scopes, toolbar readouts, shared action availability, and navigation dispatch use the same active direct media scope defined in [Navigation](navigation.md#adjacent-media). Asynchronous sibling discovery uses the requested direct media URL as the cursor for the eventual readout, not a stale or empty displayed image URL.

## Direct Media Scope

Direct video uses the ordinary direct media URL scope defined in [Navigation](navigation.md#adjacent-media).

Direct KDE archive-entry URLs use the direct media branch unless KiriView has explicitly opened an archive or directory collection scope.

Video mode does not create a separate video-only navigation scope. Supported image files and supported direct video files share one locale-aware sibling order, active navigation readout, non-wrapping navigation model, and boundary-feedback model in ordinary direct media URL scopes.

Adjacent Previous and Next use direct media navigation in ordinary direct media video mode and in image mode only when the active image belongs to an ordinary direct media URL scope. Archive collection and directly opened directory collection scopes keep opened-collection navigation active instead.

## Playback

Opening a video starts playback automatically.

When a video load or playback error is superseded by a later accepted non-error playback state for the same video, KiriView clears the public video error text. Public video states such as loading, ready, or empty must not expose stale error text from an earlier failed playback state.

Video mode shows a video viewport and a Breeze-style playback control panel at the bottom edge of the video viewport.

The regular toolbar remains available in video mode for application menu access and active-scope navigation. It shows Previous and Next controls, the current media item number, the total supported media item count for the active navigation scope when that list is known, and the same trailing control order as image mode.

In ordinary direct media video mode, Right-to-Left Reading and Two-Page Spread are hidden because they belong to opened archive and directory collection scopes. Fit remains in its image-mode position but is disabled, and the zoom control is read-only.

In opened collection video mode, active navigation remains the opened archive or directory collection. Right-to-Left Reading and Two-Page Spread visibility follows the opened collection toolbar rules, but Two-Page Spread never pairs a video item. Fit remains disabled, and the zoom control is read-only.

Video mode shows a read-only zoom percentage when the video frame size, displayed video content rectangle, and target window effective device pixel ratio are known. The value is the current fitted display size in physical pixels relative to the video's intrinsic frame size. When the percentage is unknown, the read-only zoom control displays `? %`. Users cannot edit this value or use image zoom actions for video.

The playback control panel is shown only when the current video is ready and has a video track.

The playback control panel includes icon-only play/pause and mute/unmute buttons, current time, timeline position selection and scrubbing, total duration, and a disabled non-interactive timeline state when the media is not seekable. The timeline row order is play/pause, current time, timeline, total duration, and mute/unmute. Time readouts use fixed-width digits and are the only visible text in the panel.

Toggling mute affects the current video audio output and persists across video source changes during the app session.

In regular windowed and fullscreen video mode, the default playback control style is a floating panel inside the video viewport, aligned to the bottom edge with a large-spacing bottom margin. The floating panel uses the active Kirigami background color with theme-aware translucency, the Kirigami corner radius, a weak shadow, centered content width, and a maximum width of about 75% of the viewport while preserving viewport side margins.

The playback controls switch to a fixed bottom bar inside the video viewport when the viewport is compact, touch/mobile input is active, or system animations are disabled or reduced. Compact means roughly narrower than 32 grid units or shorter than 16 grid units. The fixed bottom bar is full-width, has no floating bottom margin, has no shadow, has square outer bottom corners, and reserves its height from the video display area so the video is not covered by the controls.

When video playback reaches the natural end of the media, KiriView keeps the video output as the active presentation and must not clear the video output into an empty or null-like visual state. Playback stops at the final position. Pressing Play from that ended state restarts playback from the beginning when seeking is available.

The floating playback panel must not reserve page layout height and must remain usable in fullscreen.

Auto-hide applies only in floating control mode while video is playing. Pointer movement, hover, focus, slider drag, button press, tap, or paused playback reveals the controls and keeps them visible while interaction continues. When controls are eligible to hide, they fade after a human-moment delay. If system animations are disabled or reduced, fixed bottom bar mode is used and controls do not auto-hide.

In Flatpak, KiriView supports video playback initialization and audio output using the sandbox multimedia access required for the current video source, without requiring broader filesystem access than the File Access contract allows.

During timeline dragging, the user's drag position remains visually stable instead of being overwritten by ordinary playback position updates.

Unknown duration, invalid duration, and non-seekable media produce a stable disabled timeline state rather than invalid or flickering values.

## Video Navigation and Seeking

In direct video mode, viewer Left and Right and existing adjacent navigation actions move to the previous or next supported media item in the ordinary parent location. They do not seek within the video.

In opened collection video mode, viewer Left and Right and existing adjacent navigation actions move to the previous or next supported media item in the opened archive or directory collection. They do not seek within the video.

In video mode, the shared scan shortcuts also use adjacent navigation in the active navigation scope: `.` and `Space` move to the next supported media item, while `,` and `Shift+Space` move to the previous supported media item. They do not pan or seek within the video.

Video mode supports the shared configurable shortcuts for Open, Move to Trash, Delete Permanently, Previous Media Item, Next Media Item, First Media Item, Last Media Item, Current Content Start, Current Content End, Fullscreen, Keyboard Shortcuts, Configure Shortcuts, and Quit. Shared navigation shortcuts use active-navigation dispatch and availability.

The fixed Show Menubar shortcut follows the Menu Presentation contract in video mode.

The Current Content Start and Current Content End shortcuts are viewer-local configurable video seek actions. By default, `Shift+,` and `Alt+Home` seek to position `0 ms` in the current video, and `Shift+.` and `Alt+End` seek to the video's known positive duration. They are distinct from the fixed `Alt+Arrow` video seek shortcuts.

`P` toggles play/pause for the current video using the same playback command as the playback panel button.

When an image-only configurable shortcut is pressed in video mode, KiriView does not trigger the image action and shows the in-app toast `This action is not available for videos`. Repeated unsupported video shortcut presses update the same toast instance.

When the video play/pause configurable shortcut is pressed while a ready image is displayed, KiriView does not trigger video playback and shows the in-app toast `This action is not available for images`. Empty, loading, error, and unsupported-placeholder states do not show this image unavailable-action toast.

Disabled image-only action placements in video mode do not route through image-document commands. Only the unsupported-video shortcut interception produces the unavailable-action toast.

Timeline dragging and scrubbing is the primary way to seek within the current video.

If keyboard focus is inside the timeline control, that control may handle its own keyboard interaction.

Video mode also supports fixed local seek shortcuts: `Alt+Left` seeks backward 5 seconds, `Alt+Right` seeks forward 5 seconds, `Alt+Up` seeks forward 45 seconds, and `Alt+Down` seeks backward 45 seconds.

The configurable current-content start and end shortcuts follow the same media seekability gates as timeline seeking. Current Content End is unavailable when the duration is unknown, zero, or invalid.

Video seek shortcuts are video-mode-only and must not affect image mode or unsupported-video placeholders.

Video seek shortcuts are best-effort time seeks. They run only when the media is seekable, clamp to the valid `[0, duration]` range when duration is known, and must not promise frame-accurate seeking.

The actual landed position may be adjusted by the playback engine, commonly to a nearby decodable or keyframe position.

## Deletion

Video deletion follows [File Access](file-access.md#deletion).

After a successful direct-video deletion, playback stops before KiriView applies the direct-media follow-up item or empty-state fallback.
