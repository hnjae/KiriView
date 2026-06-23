# Current Product Boundaries

This page tracks deferred product areas that shape current milestone scope. User-visible unsupported behavior remains specified in `../spec/`; durable implementation boundaries remain in `../architecture/`.

## Deferred Media Scope

KiriView currently focuses video support on direct video files in ordinary direct media scopes. Archive-collection video playback, directory-collection video playback, collection-internal video metadata, recursive directory video playback, playlists, subtitles, track selection, frame stepping, and generated timeline thumbnails are deferred.

## Deferred Thumbnail Scope

Generated navigation thumbnails currently focus on supported direct local image items, supported direct local video items, and supported ZIP-backed archive image pages when usable thumbnail identity metadata is available. Direct archive-entry media preview thumbnails, generated thumbnails for CB7/7z and other non-ZIP-backed archive collections, and directory-collection thumbnails are deferred.
