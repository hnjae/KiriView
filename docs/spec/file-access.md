# File Access

## Supported Sources

KiriView opens user-selected image URLs, including local files, KDE-supported remote URLs such as `smb://`, and KDE-supported archive URLs such as `zip://`, `tar://`, and `sevenz://`.

KiriView opens local `.cbz`, `.cbt`, `.cb7`, and `.cbr` comic book archives. When a local comic book archive is opened directly, KiriView uses that archive as the current archive document and displays the first supported image inside that archive.

KiriView opens local `.zip`, `.tar`, `.7z`, and `.rar` archives only when they are directly provided, such as through a startup argument or the open dialog's `All files (*)` filter. When a local general archive is opened directly, KiriView uses that archive as the current archive document and displays the first supported image inside that archive.

General archives are not advertised through the desktop file's file associations, the open dialog's default image and comic book filter, or sibling archive navigation.

KiriView opens local directories only when they are directly provided, such as through a startup argument, file URL, or drop. When a local directory is opened directly, KiriView uses that directory as the current directory document and displays the first supported image inside that directory tree.

Directory documents use the same recursive supported-image page ordering as archive documents, with page names based on directory-relative paths such as `chapter/page001.png`.

Directly opened directories are not advertised through the desktop file's file associations, the open dialog's default image and comic book filter, or sibling archive navigation.

KiriView's desktop file advertises file-manager Open With handling for the image and comic book archive MIME types corresponding to its default open dialog filter.

When an image is opened from a KDE-supported archive URL such as `zip://`, `tar://`, or `sevenz://`, KiriView treats it as a single image URL rather than opening the whole archive as an archive document.

## Flatpak Access

In Flatpak, adjacent image navigation can list neighboring files under `home`, `/media`, `/mnt`, `/run/media`, and `$XDG_RUNTIME_DIR/gvfs`.

Files outside those paths remain available only when explicitly provided by the XDG portal.

In Flatpak, KiriView can also request write access to those same locations for file deletion.

## Deletion

When an image is ready, Delete requests moving the current deletion target to trash and Shift+Delete requests permanently deleting that target.

The actions are available from the application menu or menubar File menu and through their shortcuts, but not from the toolbar.

KiriView delegates user confirmation and the actual file operation to KDE's file operation handling, so users see and can cancel the target KDE is about to delete.

If the operation is canceled, the current image remains open and no notification is shown. If it fails, the current image remains open and the file operation error is shown as an in-app toast notification.

The deletion target is the displayed image URL for ordinary images, remote URLs, and images opened directly from KDE-supported archive URLs such as `zip://`.

When the displayed image is inside a local CBZ, CBT, CB7, CBR, ZIP, TAR, 7Z, or RAR archive document opened directly by KiriView, the deletion target is the archive file itself rather than the currently displayed internal image entry.

When the displayed image is inside a local directory document opened directly by KiriView, the deletion target is the directory itself rather than the currently displayed image file. Confirming the deletion deletes the entire directly opened directory as handled by KDE.

After deletion succeeds, KiriView immediately clears the deleted image.

It then opens the next supported image in the current image scope when possible, falls back to the previous supported image when no next image exists, and otherwise shows the empty state.

After deleting a directly opened comic book archive, KiriView opens the first image in the next sibling comic book archive when possible, falls back to the first image in the previous sibling comic book archive, and otherwise shows the empty state.

After deleting a directly opened directory document, KiriView shows the empty state.

## Live Directory Updates

When the current navigation scope is the local `file://` parent directory of an ordinary opened image file, KiriView keeps that directory's supported-image list live.

External additions and removals update the page number, total image count, and first/last boundary state.

If the currently displayed local image is removed outside KiriView, KiriView immediately clears that image, opens the next supported image in the same sorted directory order when possible, falls back to the previous supported image when no next image exists, and otherwise shows the empty state.

Non-local URL scopes, explicit archive URL scopes such as `zip://`, directly opened archive documents, and directly opened recursive directory documents are snapshots and do not guarantee live external update handling.
