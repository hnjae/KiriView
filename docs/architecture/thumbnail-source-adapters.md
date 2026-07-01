# Thumbnail Source Adapters

Active navigation thumbnails use source adapters to keep thumbnail scheduling and result projection independent from the kind of navigation row being displayed. The C++ document-session thumbnail runtime owns demand, async job lifetime, stale-completion rejection, foreground and background priority, and the image-provider store; adapters answer whether a row can produce a thumbnail and which cache or generation contract applies.

The document session composes the active-navigation thumbnail strip through one strip dependency port. The strip's lookup provider, generation provider, source adapter, image store, and worker scheduler must cross the session boundary as one active-navigation thumbnail dependency value rather than as unrelated raw providers.

Adapters consume the active thumbnail source key and demand bucket described by [Extension Contracts](extension-contracts.md). They must not mutate QML-facing model state, schedule jobs independently, install cache entries directly, or bypass runtime stale-completion identity. Adapter outputs are plans: unsupported fallback, cacheable local-file generation, cacheable opened-collection entry generation, or in-memory-only generation.

Thumbnail cache lookup, generation requests, cache original identity, source-kind classification, and bucket sizing are neutral thumbnail infrastructure, not session-owned active-navigation state. The active-navigation strip and main-image preview may both consume these contracts, so decoding code must import thumbnail cache contracts from the thumbnail boundary rather than from `session/`, and active-navigation row kinds must be mapped into thumbnail source kinds before crossing the generation provider boundary.

The thumbnail generation core resolves source bytes or video frame extraction, bucket scaling, image decode/render, opened-collection cache identity, and cache lookup/install through injectable thumbnail dependencies before it publishes a generated thumbnail. Production dependencies may use local files, media-entry source metadata, media-entry source bytes, KiriView image decoding, Qt Multimedia video frame extraction, rendering refinement sources, and the XDG thumbnail cache, but tests and independent callers must be able to replace source loading, scaling policy, decoding, frame extraction, and cache repository behavior without constructing session runtimes, mutating global opened-collection state, borrowing playback state, or writing to the user's thumbnail cache.

## Original Identity

Thumbnail cache identity is expressed separately from row source identity. Local files use Freedesktop file-original identity derived from the local path. Cacheable non-file originals use an explicit virtual original identity with URI, mtime, byte size, and optional MIME type supplied by the owning adapter or generation path.

Opened-collection entry thumbnails use archive-record virtual originals when the collection policy allows cache writes. The URI format is `x-kiriview://thumbnail/archive-entry/v1/crc32/<8-lower-hex-crc32>/<decimal-uncompressed-size>`, where the checksum and size come from collection-backend public ZIP entry metadata. The virtual original mtime is always `0` because freshness is carried by the archive-record URI. The original byte size is the archive entry uncompressed byte length.

KiriView must not parse archive formats solely to derive thumbnail cache identity, must not require patched archive-library APIs for cache identity, and must not use decoded-byte or entry-byte hashes for opened-collection cache keys. The thumbnail generation job derives the CRC32 metadata URI before reading entry bytes, performs the XDG lookup with that virtual original, decodes and renders only on cache miss, then installs the generated thumbnail with the same virtual identity. CRC32 cache identity is advisory and isolated to the thumbnail payload; cache collisions must not affect opened image bytes, navigation, deletion, or source document state.

## Direct Local Images

The direct-image adapter may return a cacheable generation plan only for active-navigation rows that satisfy the spec's direct-local-image thumbnail eligibility and provide a stable local-file original identity. It exposes the local path bytes used by Freedesktop thumbnail lookup and generation, enables XDG cache lookup, and allows generated results to be installed into the personal thumbnail cache when freshness requirements are satisfied by the cache boundary.

Direct image rows outside that eligibility return unsupported fallback.

## Direct Video

The direct-video adapter may return a cacheable generation plan only for active-navigation rows that satisfy the spec's direct-local-video thumbnail eligibility and provide a stable local-file original identity. It exposes the local path bytes used by Freedesktop thumbnail lookup and generation, enables XDG cache lookup, and allows generated results to be installed into the personal thumbnail cache when freshness requirements are satisfied by the cache boundary. Direct video rows outside that eligibility return unsupported fallback.

On XDG cache miss, direct-video generation may use an embedded representative image or a decoded representative frame through an isolated, cancelable thumbnail extraction job. The thumbnail path owns extraction lifecycle and result publication, must not reuse the video document playback backend, and returns a representative preview rather than a frame-accurate timestamp guarantee.

Video thumbnail orientation and mirroring follow the image or frame delivered by the platform extraction path.

## Direct Archive-Entry Media

Direct archive-entry media thumbnails represent KDE archive-entry URLs opened as individual direct media items, such as `zip:///path/archive.zip!/page.png`, not entries inside a directly opened collection. These rows return unsupported fallback. They must not reuse opened-collection cache identity, media-entry source metadata, or collection entry byte access.

## Opened Collection Entries

Opened-collection entry thumbnails represent navigation rows inside a directly opened collection whose bytes are owned by a media entry source. The collection access layer owns the shared policy that decides whether an entry can be cached, and both thumbnail source planning and media entry source metadata loading consume that policy instead of duplicating archive-scheme or entry-kind checks. Only opened-collection image rows that satisfy the spec's archive-entry thumbnail eligibility and whose collection backend exposes usable public record metadata may return a cacheable opened-collection entry plan.

Opened-collection rows outside that eligibility return unsupported fallback.

## Archive Collection Rows

Archive-collection thumbnails represent a collection row rather than an entry inside a directly opened collection. Collection-row sources return unsupported fallback and must not choose representative entries or composed previews.

## Directory Collections

Directory-collection thumbnails represent directory-backed collection rows or entries. Directory-collection sources return unsupported fallback and must not choose representative files or perform directory traversal from thumbnail demand.

## In-Memory-Only Sources

In-memory-only plans skip XDG lookup and disable cache installation. The runtime may still store the generated image in its image-provider cache, apply retention priority, publish the same ready result roles, and reject stale completions using the same source key and bucket checks as cacheable sources. This path must not expand the active-navigation thumbnail product contract: rows excluded by the spec return unsupported fallback even if a generation dependency could technically render them.
