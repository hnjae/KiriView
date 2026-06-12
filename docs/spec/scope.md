# Scope

KiriView opens one user-selected image file, direct video file, supported archive, or directly provided local directory. It displays the selected content in the main window and can navigate to adjacent media items or pages in the same location.

When an installed `kiriview` translation catalog is available, KiriView's UI text and desktop metadata follow the user's KDE and Qt language settings. When no matching translation is available, KiriView shows the original English text.

KiriView does not expose a general Settings page in the current scope.

The active navigation thumbnail strip may generate preview thumbnails only for supported direct local image items and supported image pages inside ZIP-backed archive collections: CBZ and directly opened ZIP archive entries whose metadata can identify the entry for thumbnail caching.

## Out of Scope

Editing, filesystem indexer metadata, reverse geocoding, map lookup, archive-collection video playback, directory-collection video playback, collection-internal video metadata, recursive directory video playback, playlists, subtitles, track selection, frame stepping, generated timeline thumbnails, generated video preview thumbnails, archive-entry media preview thumbnails, generated thumbnail previews for CB7/7z and other non-ZIP-backed archive collections, and directory-collection preview thumbnails are out of scope for the current version.
