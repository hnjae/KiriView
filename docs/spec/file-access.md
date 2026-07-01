# File Access

## Supported Sources

### Direct Files

KiriView opens user-selected direct image URLs, including local files and KDE-supported remote URLs such as `smb://`.

Supported image extensions are AVIF (`.avif` and `.avifs`), BMP, camera RAW (`.3fr`, `.arw`, `.bay`, `.bmq`, `.cr2`, `.cr3`, `.crw`, `.cs1`, `.cs2`, `.dcr`, `.dng`, `.erf`, `.fff`, `.iiq`, `.k25`, `.kdc`, `.mdc`, `.mef`, `.mos`, `.mrw`, `.nef`, `.nrw`, `.orf`, `.pef`, `.raf`, `.raw`, `.rdc`, `.rwl`, `.rw2`, `.sr2`, `.srf`, `.srw`, and `.x3f`), GIF, HEIF (`.heic`, `.heics`, `.heif`, `.heifs`, `.hif`, `.avci`, and `.hej2`), JPEG, JPEG 2000 (`.jp2`), JPEG XL (`.jxl`), PNG, SVG, TIFF (`.tif` and `.tiff`), and WebP, case-insensitively.

JPEG-compressed HEIF files use the generic HEIF extensions because they do not have a dedicated extension.

KiriView opens direct video URLs as direct media items for MP4, M4V, and MOV files, case-insensitively.

Direct video URLs include local paths, `file://` URLs, and KDE-supported remote file URLs such as `smb://`.

Direct-video public source identity follows [Video Playback](video-playback.md#source-url-identity).

### Archive Entry URLs

KiriView opens KDE-supported archive-entry URLs such as `zip://`, `tar://`, and `sevenz://` as single direct media URLs when those URLs point at supported image or direct video files.

Opening a KDE-supported archive-entry URL does not open the whole archive as an archive collection. Opening the backing archive file itself uses the opened-collection rules.

### Opened Collections

When a video is opened from a directly opened archive collection, KiriView plays MP4, M4V, and MOV entries only when they are uncompressed stored ZIP entries inside local CBZ or ZIP archives, or plain TAR entries inside local CBT or TAR archives.

KiriView opens local `.cbz`, `.cbt`, `.cb7`, and `.cbr` comic book archives. When a local comic book archive is opened directly, KiriView uses that archive as the current archive collection and displays the first supported media item inside that archive. If the first supported item is a playable collection video, KiriView plays it. If the first supported item is an ineligible video, KiriView displays the unsupported-video placeholder.

KiriView opens local `.zip`, `.tar`, `.7z`, and `.rar` archives only when they are directly provided, such as through a startup argument or the open dialog's `All files (*)` filter. When a local general archive is opened directly, KiriView uses that archive as the current archive collection and displays the first supported media item inside that archive. If the first supported item is a playable collection video, KiriView plays it. If the first supported item is an ineligible video, KiriView displays the unsupported-video placeholder.

### Directory Collections

When a video is opened from a directly opened local directory collection, KiriView plays MP4, M4V, and MOV files from the directory entry.

KiriView opens local directories only when they are directly provided, such as through a startup argument, file URL, or drop. When a local directory is opened directly, KiriView uses that directory as the current directory collection and displays the first supported media item inside that directory tree. If the first supported item is a video, KiriView plays it.

Opening a directory URL creates a directory collection and does not create a video-capable direct media scope.

Directory collections use the same recursive supported-media page ordering as archive collections, with page names based on directory-relative paths such as `chapter/page001.png`.

If a directly opened archive or directory collection contains no supported media, KiriView clears any displayed media and shows an error state explaining that the selected collection does not contain any supported media.

Errors shown while opening or reading media from a directly opened archive or directory collection use collection-neutral wording such as "selected collection" unless the failed operation specifically concerns an archive file, archive URL scheme, or archive format.

### Advertised Entry Points

General archives are not advertised through the desktop file's file associations, the open dialog's default image, video, and comic book filter, or sibling archive navigation.

Directly opened directories are not advertised through the desktop file's file associations, the open dialog's default image, video, and comic book filter, or sibling archive navigation.

KiriView's open dialog default filter includes supported image files, supported direct video files, and supported comic book archive files.

KiriView's desktop file advertises file-manager Open With handling for supported image, supported direct video, and comic book archive MIME types. For direct videos, the advertised MIME types are `video/mp4` and `video/quicktime`.

## Flatpak Access

In Flatpak, adjacent direct media navigation can list neighboring files under `home`, `/media`, `/mnt`, `/run/media`, and `$XDG_RUNTIME_DIR/gvfs`.

In Flatpak, KiriView grants only the specific sandbox access needed for selected direct-video playback and does not expose the entire user runtime filesystem for playback.

In Flatpak, KiriView shares generated XDG thumbnails with the host thumbnail cache when the sandbox exposes `xdg-cache/thumbnails`.

Files outside those paths remain available only when explicitly provided by the XDG portal.

In Flatpak, KiriView can also request write access to those same locations for file deletion.

## Deletion

When an image, direct video, playable collection video, or opened-collection unsupported-video placeholder has a current deletion target, Delete requests moving that target to trash and Shift+Delete requests permanently deleting that target.

The actions are available from the application menu or menubar File menu and through their shortcuts, but not from the toolbar.

KiriView delegates user confirmation and the actual file operation to KDE's file operation handling, so users see and can cancel the target KDE is about to delete.

If the operation is canceled, the current media item remains open and no notification is shown. If it fails, the current media item remains open and the file operation error is shown as an in-app toast notification.

The deletion target is the displayed image URL for ordinary images, remote URLs, and images opened directly from KDE-supported archive URLs such as `zip://`.

The deletion target is the original direct media URL for direct videos, including videos opened directly from KDE-supported archive URLs such as `zip://`, regardless of any playback preparation needed to play the video.

When the displayed media item is inside a local CBZ, CBT, CB7, CBR, ZIP, TAR, 7Z, or RAR archive collection opened directly by KiriView, the deletion target is the archive file itself rather than the currently displayed internal entry.

When the displayed media item is inside a local directory collection opened directly by KiriView, the deletion target is the directory itself rather than the currently displayed file. Confirming the deletion deletes the entire directly opened directory as handled by KDE.

After deletion succeeds, KiriView immediately clears the deleted image or stops playback for the deleted video.

For ordinary direct media URL scopes, KiriView then opens the next supported media item in the current direct media scope when possible, falls back to the previous supported media item when no next item exists, and otherwise shows the empty state.

Archive collection and directly opened directory collection deletion keep their collection-specific fallback behavior.

After deleting a directly opened comic book archive, KiriView opens the first supported media item in the next sibling comic book archive when possible, falls back to the first supported media item in the previous sibling comic book archive, and otherwise shows the empty state.

After deleting a directly opened general archive, KiriView shows the empty state.

After deleting a directly opened directory collection, KiriView shows the empty state.

## Live Directory Updates

When the current navigation scope is the local `file://` parent directory of an ordinary opened direct image or video file, KiriView keeps that directory's supported-media list live.

External additions and removals update the page number, total item count, and first/last boundary state.

If the currently displayed local image or video is removed outside KiriView, KiriView immediately clears that image or stops playback for that video, opens the next supported media item in the same sorted directory order when possible, falls back to the previous supported media item when no next media item exists, and otherwise shows the empty state.

Non-local URL scopes, explicit archive URL scopes such as `zip://`, directly opened archive collections, and directly opened recursive directory collections are snapshots and do not guarantee live external update handling.

## Current Media Target

The current media target is the current media item for Open With and information-panel file actions. It is distinct from the deletion target.

For direct images, remote images, direct videos, and media opened directly from KDE-supported archive URLs such as `zip://`, the current media target is the displayed or original direct media URL.

For media items displayed inside a directly opened local directory collection, the current media target is the currently displayed image or playable collection video file inside that directory.

For media items displayed inside a directly opened local CBZ, CBT, CB7, ZIP, TAR, or 7Z archive collection, including playable collection videos and unsupported-video placeholders, the current media target is the currently displayed internal media URL when that URL uses a KDE-supported archive scheme such as `zip://`, `tar://`, or `sevenz://`.

For media items displayed inside a directly opened local CBR or RAR archive collection, the current media target is unavailable for Open With, Copy File Path, and Open Containing Folder because the displayed entry has no KDE/KIO-openable media URL.

## Open With

The Open With action opens the current media target with another application and delegates application selection and launching to KDE/KIO open-with handling.

Open With is disabled when no media item is ready, when the current document is empty, loading, or failed, or when KiriView cannot derive a KDE/KIO-openable current media URL. Canceling the KDE/KIO open-with flow leaves KiriView unchanged and does not show an in-app notification.

## Information Panel File Actions

The Info Panel's Copy File Path action copies the current media target's display path to the clipboard. Display paths decode percent-encoded URL path text for user readability.

For local file URLs, Copy File Path copies the local file path. For non-local KDE/KIO URLs, it copies the readable URL path for the current media target.

Copy File Path is disabled when the current media target is unavailable.

The Info Panel's Open Containing Folder action opens the current media target's parent location in the file manager and selects or highlights the target when the desktop environment supports it.

Open Containing Folder is available for local file targets and KDE/KIO URLs with a parent location. It is disabled when no media target is available or when KiriView cannot derive a containing location.
