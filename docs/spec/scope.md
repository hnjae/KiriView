# Scope

KiriView opens one user-selected image file, direct video file, supported archive, or directly provided local directory. It displays the selected content in the main window and can navigate to adjacent media items or pages in the same location.

KiriView's installed application, desktop file, and icon identity is `org.hnjae.kiriview`.

When an installed `kiriview` translation catalog is available, KiriView's UI text and desktop metadata follow the user's KDE and Qt language settings. When no matching translation is available, KiriView shows the original English text.

KiriView does not expose a general Settings page.

The active navigation thumbnail strip follows the thumbnail eligibility and fallback behavior defined in [Navigation](navigation.md#page-controls). KiriView does not provide generated navigation thumbnails outside that contract.

## Not Provided

KiriView does not provide editing, filesystem indexer metadata, reverse geocoding, map lookup, opened archive collection video playback outside eligible stored ZIP and plain TAR entries, collection-internal video metadata, playlists, subtitles, track selection, frame stepping, generated video timeline thumbnails, direct archive-entry media preview thumbnails, generated thumbnail previews for CB7/7z and other non-ZIP-backed archive collections, or directory-collection preview thumbnails.
