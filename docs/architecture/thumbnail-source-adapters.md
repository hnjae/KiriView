# Thumbnail Source Adapters

Active navigation thumbnails use source adapters to keep thumbnail scheduling and result projection independent from the kind of navigation row being displayed. The C++ document-session thumbnail runtime owns demand, async job lifetime, stale-completion rejection, foreground and background priority, and the image-provider store; adapters answer whether a row can produce a thumbnail and which cache or generation contract applies.

The document session composes the active-navigation thumbnail strip through one strip dependency port. Session runtime construction may choose the strip's lookup provider, generation provider, source adapter, image store, and worker scheduler, but those dependencies must cross the session boundary as one active-navigation thumbnail dependency value rather than as unrelated raw providers.

Adapters consume the active thumbnail source key and demand bucket described by [Extension Contracts](extension-contracts.md). They must not mutate QML-facing model state, schedule jobs independently, install cache entries directly, or bypass runtime stale-completion identity. Adapter outputs are plans: unsupported fallback, cacheable local-file generation, cacheable opened-collection entry generation, or in-memory-only generation.

Thumbnail cache lookup, generation requests, cache original identity, source-kind classification, and bucket sizing are neutral thumbnail infrastructure, not session-owned active-navigation state. The active-navigation strip and main-image preview may both consume these contracts, so decoding code must import thumbnail cache contracts from the thumbnail boundary rather than from `session/`, and active-navigation row kinds must be mapped into thumbnail source kinds before crossing the generation provider boundary.

The default thumbnail generation core resolves source bytes or video frame extraction, bucket scaling, image decode/render, opened-collection cache identity, and cache lookup/install through injectable thumbnail dependencies before it publishes a generated thumbnail. Default dependencies may use local files, media-entry source metadata, media-entry source bytes, KiriView image decoding, Qt Multimedia video frame extraction, rendering refinement sources, and the XDG thumbnail cache, but tests and independent callers must be able to replace source loading, scaling policy, decoding, frame extraction, and cache repository behavior without constructing session runtimes, mutating global opened-collection state, borrowing playback state, or writing to the user's thumbnail cache.

## Original Identity

Thumbnail cache identity is expressed separately from row source identity. Local files use Freedesktop file-original identity derived from the local path. Cacheable non-file originals use an explicit virtual original identity with URI, mtime, byte size, and optional MIME type supplied by the owning adapter or generation path.

Opened-collection entry thumbnails use archive-record virtual originals when the collection policy allows cache writes. The URI format is `x-kiriview://thumbnail/archive-entry/v1/crc32/<8-lower-hex-crc32>/<decimal-uncompressed-size>`, where the checksum and size come from upstream KArchive ZIP entry metadata: `KZipFileEntry::crc32()` and the entry's uncompressed byte length. The virtual original mtime is always `0` because freshness is carried by the archive-record URI. The original byte size is the archive entry uncompressed byte length.

KiriView must not parse archive formats to derive thumbnail cache identity and must not use decoded-byte or entry-byte hashes for opened-collection cache keys. The archive backend may expose cacheable metadata only through public upstream KArchive entry APIs; KiriView does not carry a downstream generic `KArchiveFile::contentChecksum()` API. The thumbnail generation job derives the CRC32 metadata URI before reading entry bytes, performs the XDG lookup with that virtual original, decodes and renders only on cache miss, then installs the generated thumbnail with the same virtual identity. CRC32 collisions are tolerated at the thumbnail-cache layer and may show a wrong preview thumbnail, but they must not affect opened image bytes, navigation, deletion, or source document state.

## Direct Local Images

The default direct-image adapter accepts supported direct local image rows only. It exposes the local path bytes used by Freedesktop thumbnail lookup and generation, enables XDG cache lookup, and allows generated results to be installed into the personal thumbnail cache when freshness requirements are satisfied by the cache helper.

Unsupported direct image rows, including non-local URLs, stay on the runtime fallback path.

## Direct Video

The default direct-video adapter accepts supported direct local video rows only. It exposes the local path bytes used by Freedesktop thumbnail lookup and generation, enables XDG cache lookup, and allows generated results to be installed into the personal thumbnail cache when freshness requirements are satisfied by the cache helper. Non-local direct video rows stay on the runtime fallback path.

On XDG cache miss, direct-video generation owns a short-lived Qt Multimedia extraction job. The job creates its own `QMediaPlayer` and `QVideoSink`, checks player metadata for `QMediaMetaData::CoverArtImage` and then `QMediaMetaData::ThumbnailImage`, and uses the first usable embedded `QImage` before decoded-frame fallback. When no usable embedded image is available and the media has a known positive duration, the job seeks through representative candidate positions, converts delivered frames to `QImage`, accepts the first candidate whose simple RGB-byte variance exceeds the boring-frame threshold, and falls back to the last captured candidate if every captured candidate is boring. When duration or seekability is unavailable, the job keeps a best-effort first delivered frame fallback. The extractor owns metadata-first extraction, candidate seeking, boring-frame scoring, fallback selection, cancellation, and result publication; the video document playback backend is not reused for thumbnails, and thumbnail jobs must be cancelable by stopping and deleting their extractor object.

The embedded cover image is preferred because it is the media-provided representative image. The Totem-inspired decoded-frame scoring is simple RGB-byte variance, not semantic content detection, and the selected decoded frame is representative rather than a frame-accurate timestamp guarantee. Rotation or mirroring metadata is honored only when Qt's delivered embedded image or frame image already reflects it.

## Direct Archive-Entry Media

Direct archive-entry media thumbnails represent KDE archive-entry URLs opened as individual direct media items, such as `zip:///path/archive.zip!/page.png`, not entries inside a directly opened collection. These rows return unsupported fallback. They must not reuse opened-collection cache identity, media-entry source metadata, or collection entry byte access.

## Opened Collection Entries

Opened-collection entry thumbnails represent navigation rows inside a directly opened collection whose bytes are owned by a media entry source. The collection module owns the shared policy that decides whether an entry can be cached, and both thumbnail source planning and media entry source metadata loading consume that policy instead of duplicating archive-scheme or entry-kind checks. Only supported image entries inside directly opened archive collections backed by `zip` roots may return a cacheable opened-collection entry plan, and only when the KArchive backend exposes usable ZIP CRC32 metadata plus uncompressed size through upstream APIs. This covers cacheable CBZ and directly opened ZIP scopes; directory collections, TAR/CBT, RAR/libarchive, 7Z/CB7 collections, and video entries remain unsupported for cacheable opened-collection thumbnails.

Directory collections, TAR-backed CBT scopes, RAR-backed CBR scopes, 7Z/CB7 archive collections, and collection video entries return unsupported fallback.

## Archive Collection Rows

Archive-collection thumbnails represent a collection row rather than an entry inside a directly opened collection. KiriView's active-navigation thumbnail strip does not use generated collection-row thumbnails, so archive-collection rows return unsupported fallback and must not choose representative entries or composed previews.

## Directory Collections

Directory-collection thumbnails represent directory-backed collection rows or entries. KiriView's active-navigation thumbnail strip does not use generated directory-collection thumbnails, so directory-collection rows and entries return unsupported fallback and must not choose representative files or perform directory traversal from thumbnail demand.

## In-Memory-Only Sources

In-memory-only plans skip XDG lookup and disable cache installation. The runtime may still store the generated `QImage` in its image-provider cache, apply retention priority, publish the same ready result roles, and reject stale completions using the same source key and bucket checks as cacheable sources. This path does not expand the active-navigation thumbnail product contract: active-navigation rows outside supported direct local images, supported direct local videos, and supported image entries inside cacheable CBZ or directly opened ZIP archive collections return unsupported fallback.
