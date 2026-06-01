# Thumbnail Source Adapters

Active navigation thumbnails use source adapters to keep thumbnail scheduling and result projection independent from the kind of navigation row being displayed. The C++ document-session thumbnail runtime owns demand, async job lifetime, stale-completion rejection, foreground and background priority, and the image-provider store; adapters answer whether a row can produce a thumbnail and which cache or generation contract applies.

Adapters consume the active thumbnail source key and demand bucket. They must not mutate QML-facing model state, schedule jobs independently, install cache entries directly, or bypass runtime stale-completion identity. Adapter outputs are plans: unsupported fallback, cacheable local-file generation, or in-memory-only generation.

## Direct Local Images

The default direct-image adapter accepts supported direct local image rows only. It exposes the local path bytes used by Freedesktop thumbnail lookup and generation, enables XDG cache lookup, and allows generated results to be installed into the personal thumbnail cache when freshness requirements are satisfied by the cache helper.

Unsupported direct image rows, including non-local URLs, stay on the runtime fallback path unless a future adapter explicitly supplies another renderable plan.

## Direct Video

A future direct-video adapter should supply stable source identity, freshness facts for the backing file, and a representative-frame renderer. When the video source can satisfy Freedesktop personal-cache identity and freshness requirements, it may use the same cache lookup, generation, cache install, image-store, and result publication path as direct images. Video playback state remains independent from thumbnail generation; the adapter must not borrow the playback backend as its state owner.

If representative-frame extraction can render an image but cannot provide cache-safe identity or freshness, the adapter should return an in-memory-only generation plan so the runtime skips XDG lookup and disables cache writes.

## Archive Entries

Archive-entry thumbnails represent an item inside an archive-backed image-document scope. The adapter should identify the entry relative to the archive scope, include backing archive freshness and entry identity when available, and render through the runtime generation provider. Cache writes are allowed only after the Freedesktop cache identity and freshness model is revisited for archive-internal sources; until then, archive-entry thumbnails should use in-memory-only generation.

## Archive Collections

Archive-collection thumbnails represent a collection row rather than one directly opened file. The adapter should choose a deterministic representative item or composed preview from the collection scope and keep that representative choice part of the adapter-owned source identity. Cache writes require a stable collection identity and freshness facts for the backing archive plus the selected representative inputs; otherwise the plan must be in-memory-only.

## Directory Collections

Directory-collection thumbnails represent a directory-backed collection row. The adapter should identify the collection relative to the directory scope and use directory or selected-item freshness facts when those facts can be checked without turning QML demand into directory traversal ownership. Cache writes require a stable Freedesktop-compatible identity for the represented collection state; otherwise the adapter must return in-memory-only generation.

## In-Memory-Only Sources

In-memory-only plans skip XDG lookup and disable cache installation. The runtime may still store the generated `QImage` in its image-provider cache, apply retention priority, publish the same ready result roles, and reject stale completions using the same source key and bucket checks as cacheable sources. This path exists for virtual, archive-internal, non-local, or collection sources whose rendered thumbnail is useful for the current session but not yet safe to write to the personal thumbnail cache.
