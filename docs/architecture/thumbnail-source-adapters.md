# Thumbnail Source Adapters

Active navigation thumbnails use source adapters to keep thumbnail scheduling and result projection independent from the kind of navigation row being displayed. The C++ document-session thumbnail runtime owns demand, async job lifetime, stale-completion rejection, foreground and background priority, and the image-provider store; adapters answer whether a row can produce a thumbnail and which cache or generation contract applies.

Adapters consume the active thumbnail source key and demand bucket described by [Extension Contracts](extension-contracts.md). They must not mutate QML-facing model state, schedule jobs independently, install cache entries directly, or bypass runtime stale-completion identity. Adapter outputs are plans: unsupported fallback, cacheable local-file generation, cacheable opened-collection entry generation, or in-memory-only generation.

Thumbnail cache lookup, generation requests, cache original identity, source-kind classification, and bucket sizing are neutral thumbnail infrastructure, not session-owned active-navigation state. The active-navigation strip and main-image preview may both consume these contracts, so decoding code must import thumbnail cache contracts from the thumbnail boundary rather than from `session/`, and active-navigation row kinds must be mapped into thumbnail source kinds before crossing the generation provider boundary.

## Original Identity

Thumbnail cache identity is expressed separately from row source identity. Local files use Freedesktop file-original identity derived from the local path. Cacheable non-file originals use an explicit virtual original identity with URI, mtime, byte size, and optional MIME type supplied by the owning adapter or generation path.

Opened-collection entry thumbnails use archive-record virtual originals when the collection policy allows cache writes. The URI format is `x-kiriview://thumbnail/archive-entry/v1/crc32/<8-lower-hex-crc32>/<decimal-uncompressed-size>`, where the checksum and size come from KArchive-provided archive entry metadata for the uncompressed entry stream. The virtual original mtime is always `0` because freshness is carried by the archive-record URI. The original byte size is the archive entry uncompressed byte length.

KiriView must not parse archive formats to derive thumbnail cache identity and must not use decoded-byte or entry-byte hashes for opened-collection cache keys. The archive backend may expose cacheable metadata only through public KArchive entry APIs. The thumbnail generation job derives the CRC32 metadata URI before reading entry bytes, performs the XDG lookup with that virtual original, decodes and renders only on cache miss, then installs the generated thumbnail with the same virtual identity. CRC32 collisions are tolerated at the thumbnail-cache layer and may show a wrong preview thumbnail, but they must not affect opened image bytes, navigation, deletion, or source document state.

## Direct Local Images

The default direct-image adapter accepts supported direct local image rows only. It exposes the local path bytes used by Freedesktop thumbnail lookup and generation, enables XDG cache lookup, and allows generated results to be installed into the personal thumbnail cache when freshness requirements are satisfied by the cache helper.

Unsupported direct image rows, including non-local URLs, stay on the runtime fallback path unless a future adapter explicitly supplies another renderable plan.

## Direct Video

A future direct-video adapter should supply stable source identity, freshness facts for the backing file, and a representative-frame renderer. When the video source can satisfy Freedesktop personal-cache identity and freshness requirements, it may use the same cache lookup, generation, cache install, image-store, and result publication path as direct images. Video playback state remains independent from thumbnail generation; the adapter must not borrow the playback backend as its state owner.

If representative-frame extraction can render an image but cannot provide cache-safe identity or freshness, the adapter should return an in-memory-only generation plan so the runtime skips XDG lookup and disables cache writes.

## Archive Entries

Archive-entry thumbnails represent an item inside an archive-backed image-document scope. The adapter should identify the entry relative to the archive scope, include backing archive freshness and entry identity when available, and render through the runtime generation provider. Cache writes are allowed only after the Freedesktop cache identity and freshness model is revisited for archive-internal sources; until then, archive-entry thumbnails should use in-memory-only generation.

## Opened Collection Entries

Opened-collection entry thumbnails represent navigation rows inside a directly opened collection whose bytes are owned by a media entry source. The collection module owns the shared policy that decides whether an entry can be cached, and both thumbnail source planning and media entry source metadata loading consume that policy instead of duplicating archive-scheme or entry-kind checks. In v1, only image entries inside directly opened comic-book archives backed by `zip` or `sevenz` roots may return a cacheable opened-collection entry plan, and only when the KArchive backend exposes a usable CRC32 entry checksum plus uncompressed size. This covers cacheable CBZ and CB7 scopes; directory collections, TAR/CBT, RAR/libarchive, generic ZIP/7Z collections, and video entries remain unsupported for cacheable opened-collection thumbnails.

Directory collections, TAR-backed CBT scopes, RAR-backed CBR scopes, generic ZIP or 7Z archive collections that are not comic-book archive scopes, and collection video entries return unsupported fallback unless a future collection-owned policy defines a cache-safe identity for them.

## Archive Collection Rows

Archive-collection thumbnails represent a collection row rather than one directly opened file. The adapter should choose a deterministic representative item or composed preview from the collection scope and keep that representative choice part of the adapter-owned source identity. Cache writes require a stable collection identity and freshness facts for the backing archive plus the selected representative inputs; otherwise the plan must be in-memory-only.

## Directory Collections

Directory-collection thumbnails represent a directory-backed collection row. The adapter should identify the collection relative to the directory scope and use directory or selected-item freshness facts when those facts can be checked without turning QML demand into directory traversal ownership. Cache writes require a stable Freedesktop-compatible identity for the represented collection state; otherwise the adapter must return in-memory-only generation.

## In-Memory-Only Sources

In-memory-only plans skip XDG lookup and disable cache installation. The runtime may still store the generated `QImage` in its image-provider cache, apply retention priority, publish the same ready result roles, and reject stale completions using the same source key and bucket checks as cacheable sources. This path exists for virtual, archive-internal, non-local, or collection sources whose rendered thumbnail is useful for the current session but not yet safe to write to the personal thumbnail cache.
