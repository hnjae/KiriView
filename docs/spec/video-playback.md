# Video Playback

## Product Boundary

KiriView supports direct video files as moving, sound-capable image-like media items in the MVP.

Supported direct video inputs may come from the startup argument, drop, open dialog selection, or ordinary adjacent media navigation from a direct media URL scope.

The MVP direct video format list is MP4, M4V, and MOV, case-insensitively. These cover common camera and phone video files without expanding the initial scope to less common container behavior.

Direct video playback supports direct file-like URLs that KiriView can hand to KDE/KIO as ordinary file items, including local paths, `file://` URLs, and KDE-supported archive URLs that point at a file entry such as `zip:///path/archive.zip!/clip.mp4`.

Direct KDE archive-entry URLs are not KiriView archive document mode. KiriView may open `zip:///path/archive.zip!/clip.mp4` as a direct media URL while still opening `/path/archive.zip` as an archive document when the archive itself is selected.

The product boundary is based on KiriView's document-mode state, not on URL scheme alone. If KiriView has opened an archive or directory as a document, video entries inside that document are out of scope. If KiriView is handling a direct media URL without an active archive or directory document context, that URL remains eligible for video support even when its scheme is a KDE archive scheme.

Video files do not participate in KiriView archive document navigation, comic book archive document handling, directly opened directory documents, video-frame predecode, video-frame image cache, editable image zoom, image pan, image rotate, two-page spread, or Right-to-Left Reading.

In ordinary direct media URL scopes, showing a video must not clear or stop the background predecode/cache lifecycle for nearby supported image files. KiriView may keep and continue predecoding adjacent still images around the current video cursor, but it must not attempt to predecode the video itself.

Archive-document-internal video, directly opened directory-document video, recursive directory video, playlists, subtitles, track selection, metadata panels, frame stepping, and timeline preview thumbnails are out of scope.

KiriView advertises direct video support through the desktop file for the MIME types that cover the MVP MP4, M4V, and MOV format list: `video/mp4` and `video/quicktime`.

## Source URL Identity

KiriView may internally resolve a KIO-backed direct video URL to a local playback URL before handing it to the video backend, such as through KIOFuse or another KIO-backed resolver. Resolution succeeds only when the resolved playback URL can be consumed by the video backend; otherwise the video load fails while keeping the original direct media URL as the public source and error context.

Internal playback URL resolution must not change the user-facing source URL. Window title, adjacent media navigation, deletion target, error context, and document-mode decisions remain based on the original direct media URL.

Resolver-local playback URLs are video-only. Opening a direct image after a direct video must route the image through the image document with the original image URL, not through the previous video playback URL or another KIOFuse local playback URL.

Opening a video after an image must not route the video through `KiriImageDocument`.

Opening an image after a video restores image behavior.

The top-level document session owns the active document kind for direct image and video routing. Its public source URL follows the same user-facing identity as the active image or video document: assigning it starts an open request, successful opens expose the original direct media URL, replacement failures keep the previously displayed direct URL, initial failures may keep the failed request as error context, and resolver-local playback URLs are never exposed as the session source.

For direct video and direct image media scopes, the document session owns the active navigation projection used by toolbar readouts, shared action availability, and navigation dispatch. Asynchronous sibling discovery uses the requested session-owned direct media URL as the cursor for the eventual readout, not an image-document displayed URL.

## Ordinary Direct Media URL Scope

An ordinary direct media URL scope is the non-recursive parent URL of the active direct media URL.

This includes ordinary local parent directories and KDE archive URL parent locations such as `zip:///path/archive.zip!/chapter/`.

Opening a directory URL creates the existing directory document and does not create a video-capable media scope.

The ordinary direct media URL parent follows KiriView's existing direct image candidate context rule rather than a new URL-scheme-specific parser: KiriView normalizes the original direct URL through the same `navigationSourceUrl(...)` path used for displayed direct images, then derives the parent with `QUrl::RemoveFilename | QUrl::NormalizePathSegments`.

Direct KDE archive-entry URLs use this direct URL branch unless KiriView has explicitly opened an archive or directory document scope.

Supported image files and supported direct video files share one locale-aware sibling order in ordinary direct media URL scopes.

Adjacent media navigation is non-recursive, does not wrap, and uses the same session-owned active navigation projection and boundary-feedback model as image navigation. Boundary feedback in a media-aware scope uses neutral first-media-item and last-media-item wording rather than calling every boundary item an image.

The top-level document session dispatches adjacent Previous and Next through media navigation in video mode and in image mode only when the active image belongs to an ordinary direct media URL scope. Archive document and directly opened directory document image scopes keep image-document navigation as the session projection's internal source.

## Playback

Opening a video starts playback automatically.

Video mode shows a video viewport and a Kirigami floating playback panel over the bottom of the video.

The regular toolbar remains available in video mode for application menu access and ordinary direct media navigation. It shows Previous and Next controls, the current media item number, the total supported media item count for the ordinary direct media URL scope when that list is known, and the same trailing control order as image mode, all from the document session's active navigation projection.

Video mode keeps image-only toolbar controls visible in their image-mode positions but unavailable: Right-to-Left Reading, Two-Page Spread, and Fit are disabled, and the zoom control is read-only.

Video mode shows a read-only zoom percentage when the video frame size, displayed video content rectangle, and target window effective device pixel ratio are known. The value is the current fitted display size in physical pixels relative to the video's intrinsic frame size. When the percentage is unknown, the read-only zoom control displays `? %`. Users cannot edit this value or use image zoom actions for video.

The floating playback panel includes play/pause, timeline position selection and scrubbing, duration and position display, and a disabled non-interactive timeline state when the media is not seekable.

The floating playback panel uses a responsive width based on the video viewport, not the full window or only its implicit content width. It targets 65% of the viewport width, remains at least wide enough for its controls and about 24 grid units, caps at about 44 grid units, and preserves side margins on narrow viewports.

When video playback reaches the natural end of the media, KiriView leaves the final decoded frame visible and stops playback at the final position. Pressing Play from that ended state restarts playback from the beginning when seeking is available.

The floating playback panel must not reserve page layout height and must remain usable in fullscreen.

For the MVP, the floating playback panel remains visible while video mode is active. Auto-hide is out of scope unless explicitly specified later.

In Flatpak, KiriView exposes the host PipeWire runtime socket for Qt Multimedia video playback initialization while keeping PulseAudio-compatible audio output available.

During timeline dragging, the user's drag position remains visually stable instead of being overwritten by ordinary playback position updates.

Unknown duration, invalid duration, and non-seekable media produce a stable disabled timeline state rather than invalid or flickering values.

## Video Navigation and Seeking

In video mode, viewer Left and Right and existing adjacent navigation actions move to the previous or next supported media item in the ordinary parent location. They do not seek within the video.

Video mode supports the shared configurable shortcuts for Open, Move to Trash, Delete Permanently, Previous Media Item, Next Media Item, First Media Item, Last Media Item, Fullscreen, Keyboard Shortcuts, Configure Shortcuts, Show Menubar, and Quit. Shared media navigation shortcuts use session active-navigation dispatch and availability.

When an image-only configurable shortcut is pressed in video mode, KiriView does not trigger the image action and shows the in-app toast `This action is not available for videos`. Repeated unsupported video shortcut presses update the same toast instance.

Timeline dragging and scrubbing is the primary way to seek within the current video.

If keyboard focus is inside the timeline control, that control may handle its own keyboard interaction.

Video mode also supports fixed local seek shortcuts: `Alt+Left` seeks backward 5 seconds, `Alt+Right` seeks forward 5 seconds, `Alt+Up` seeks forward 45 seconds, and `Alt+Down` seeks backward 45 seconds.

Video seek shortcuts are video-mode-only and must not affect image mode, archive document mode, or directly opened directory document mode.

Video seek shortcuts are best-effort time seeks through Qt Multimedia position seeking. They run only when the media is seekable, clamp to the valid `[0, duration]` range when duration is known, and must not promise frame-accurate seeking.

The actual landed position may be adjusted by the Qt Multimedia backend, commonly to a nearby decodable or keyframe position.

## Deletion

Direct video files support the same user-facing Move to Trash and Delete Permanently actions as images.

The deletion target for a direct video is the original direct media URL, even if KiriView resolved a separate local playback URL internally.

If deletion is canceled, the current video remains open and no notification is shown.

If deletion fails, the current video remains open and the file operation error is shown as an in-app toast notification.

After successful video deletion, playback stops and KiriView opens the next supported media item in the current ordinary media scope when possible, falls back to the previous supported media item when no next item exists, and otherwise returns to empty state.

In image mode, ordinary direct media deletion uses the same media-aware fallback order, so deleting an image can open a neighboring supported video. Archive document and directly opened directory document image deletion keep their image and document-specific fallback behavior.
