# Extension Contracts

KiriView extension points are adapter contracts, not state backdoors. A media source, thumbnail source, predecode planner, decoder, or render source returns typed keys, capabilities, plans, demands, payloads, or completion results to its owning runtime. It must not mutate QML state, physical item state, public session state, platform action state, cache state, or render-node policy directly.

## Shared Contract Shape

Durable identity is the stable key for the thing being addressed. Freshness generation is the owner's proof that a result still belongs to the currently accepted scope. Equality compares durable identity within one key family only; generation is used to accept or reject work for a current lifecycle. Boundary APIs must encode key families as distinct value types or explicit tagged structs so code cannot accidentally compare ordinary files, opened-collection entries, thumbnails, predecode candidates, and render surfaces through one generic URL identity.

A demand describes requested work from an owner snapshot: key, generation, size bucket, priority, render context, visible area, decode window, or similar inputs. A result reports ready, pending, unsupported, failed, canceled, or stale without changing ownership. Unsupported means the adapter cannot provide the capability for that key; failed means the adapter accepted the demand and could not complete it.

Adapters are synchronous policy or asynchronous payload providers behind an owner. Synchronous adapters return plans and capability facts. Asynchronous adapters return payloads through the owner's lifecycle contract, carrying the owner-held operation id, source key plus generation, demand key, or display-source revision needed for stale-completion rejection. Adapter APIs must not accept QML objects, facade objects, mutable public projection objects, or platform action objects; if a capability needs Qt runtime data, the owner captures that data into a plain demand before calling the adapter.

Capabilities are descriptive. They can say whether bytes can be read, thumbnails can be generated or cached, a still image can be predecoded, a collection video source device can be opened, a decode route is supported, a provider-ready display image can be produced, or a whole-image refinement source is available. Capabilities must not imply ownership of public state or platform side effects.

## Source Key Families

### Direct Media Keys

Ordinary file keys use the top-level source-key contract: normalized path segments, local path cleanup, absolute local identity for relative local paths, fully encoded identity strings, preserved query and fragment, case-sensitive comparison, and no symlink, alias, shortcut, or platform filesystem equivalence resolution.

Direct video keys use the same ordinary direct-media identity rules as direct images. Playback source preparation belongs to the video document runtime; source keys identify routing, navigation, thumbnails, predecode eligibility, and stale completion checks only.

Direct-media scope identity is `{ current key, parent key, generation }`. The current and parent keys first use navigation-source handling, including document-portal host paths and platform archive-entry restoration facts, then apply source-key normalization. The generation changes when the effective current key or parent key changes. Pending direct-image confirmation to an equivalent displayed URL is a phase change within the same generation.

### Collection And Entry Keys

Image-document page keys identify a page URL plus its image-document source scope. Ordinary direct image pages use the direct media key. Opened-collection pages include the opened collection scope and page URL. Page keys preserve page kind so image and video rows do not share render, thumbnail, or predecode capabilities by accident.

Opened-collection scope keys identify the backing collection file URL, collection root URL, and collection kind. Comic-book archive, general archive, and directory scopes are distinct even if URLs match. A collection entry key adds the entry URL relative to that scope and carries the page kind needed to keep playable video source devices separate from image byte reads.

Archive root keys identify the backing archive file and root URL. Archive-entry freshness is owned by the opened collection backend. Thumbnail cache writes may use archive-record virtual originals only when the backend exposes public entry metadata that satisfies the thumbnail contract.

Directory collection keys identify the directory scope root and selected entry. Directory traversal belongs to a collection-owned adapter or runtime, not to QML demand. Active-navigation thumbnail adapters do not use directory keys for cache writes.

### Thumbnail And Predecode Keys

Thumbnail source keys identify the projected active-navigation row, its source kind, row number, URL, label, page kind, and navigation generation. Thumbnail cache original identity is separate from row source identity. Local files use Freedesktop file-original identity. Cacheable opened-collection entries use the virtual original defined by the thumbnail source adapter contract. In-memory-only sources skip XDG lookup and cache installation. The row key is a public navigation identity; cache original identity is a cache-family identity and must not be used as a row identity.

Predecode candidate keys identify still-image payloads eligible for adjacent decode. Direct media predecode is still-image-only; videos may be cursor positions for window planning, but they do not produce cached video frame payloads. Opened-collection predecode candidates carry the opened collection scope so byte access stays behind the media entry source owner.

### Display Source Keys

Display source keys identify the target provider-rendered page or animation role, source identity, selected-source scope, source generation, display-source owner, display revision, render-context or texture-capability generation, allocation cap generation or resolved cap, and bucket or frame demand needed to reject stale completions. Window changes, scene-graph invalidation, DPR changes, and texture-capability changes advance display-source or render-context freshness so stale decode and refinement completions are rejected instead of replacing the accepted provider entry.

Provider-rendering work must carry a display-source demand key and publish only complete display entries, not visual page tiles. Source-internal tiling is allowed only inside a decoder or refinement job that assembles one accepted display `QImage` before returning to the owner.

### Key Family Requirements

Key-family types must be operational contracts, not marker structs. Each family that crosses an adapter, cache, predecode, thumbnail, or render boundary must provide construction from its owning snapshot, durable equality, hashing where used in sets or caches, freshness generation access, and explicit result or capability status. The generic top-level `SourceKey` may appear inside family factories as an implementation detail, but boundary APIs must accept and return the family type so unrelated identities cannot be compared accidentally.

## Adapter Contracts

### Media Entry Source Adapters

Media entry source adapters list and read opened-collection entries. They return candidates, image bytes, optional thumbnail metadata, typed failure payloads, and eligible video playback source devices through the media entry source owner. Video playback source devices may be exposed only for collection entries whose storage backend can provide the final product's playable collection-video contract; unsupported video entries remain navigation candidates without playback devices. A returned video playback source device keeps any backing archive, entry, and device lifetime behind the media entry source contract until the video source owner clears or supersedes it. Failure payloads must preserve backend, operation, collection URL, optional entry path, user-facing text, and diagnostic detail before any document, video, or thumbnail owner projects them into broader UI messages. They do not update document source state, page navigation, deletion state, thumbnails, playback state, or QML models directly.

### Thumbnail Source Adapters

Thumbnail source adapters consume thumbnail source keys and demand buckets, then return unsupported fallback, cacheable local-file generation, cacheable opened-collection entry generation, or in-memory-only generation. The document-session thumbnail runtime owns scheduling, lookup, generation jobs, cache installation, image-store retention, result projection, cancellation, and stale-completion rejection.

### Predecode Planners

Predecode planners consume session or image-document snapshots and produce still-image decode windows. The predecode runtime owns debounce, power-saver suppression, active load admission, decode jobs, cache lifetime, and generation acceptance. The provider-rendering architecture stores provider-ready display `QImage` payloads that can be promoted into the display image store without constructing tile surfaces.

### Decoder Contracts

Decoder contracts are route based. Rust-owned image format policy owns advertised extension/MIME metadata, decoder-family capability, and byte/file-name classification inputs that select one decode route from plain bytes and file-name context. C++ executes the selected decoder route and treats selected-decoder failure as final for that request. A decoder returns decoded static image, animation reader payload, metadata, unsupported, or failure; it must not route to another decoder or mutate document state. Failure payloads preserve the selected route, decoder operation, user-facing text, diagnostic detail, severity, and retryability before the image document maps them into its load-failure projection. Source-neutral display diagnostics expose typed operation outcomes for first-display, blocking-preview, and raster-refinement paths; string error outputs are derived views over those diagnostics. Concrete decoder helpers may expose source-specific aliases or richer diagnostics, but they must not be the only typed path available to production owners.

### Display Provider Contracts

Display provider contracts publish immutable display entries from owner-accepted `QImage` payloads. The ordinary provider request path is cache-only and reentrant: it may look up an existing entry, report original size, and perform bounded downscale-only requested-size handling, but it must not decode, rasterize SVG, perform file I/O, schedule refinement, decide stale acceptance, mutate public state, or depend on QML engine caching for freshness.

### Excluded Extension Paths

Custom image render-source contracts are not production extension points in the provider-backed presenter architecture. Image display extensions must publish provider-ready display entries or return whole-image refinement results to their owner; they must not introduce visual tile scheduling, custom Qt Quick rendering items, scene graph nodes, texture providers, framebuffer paths, direct texture ownership, direct low-level rendering resources, or custom shaders.

## Contract Tests

Contract tests must cover unsupported media, stale source keys or generations, cache identity, opened-collection entry freshness, video-versus-image eligibility, provider request boundaries, display-entry lifetime, forbidden low-level rendering or tile fallback dependencies, and display-source stale-completion rejection after window, DPR, or texture-capability changes. Prefer focused C++ tests where the contract depends on Qt URL, image, object-lifetime, or display data, and Rust tests where the logic is plain value policy.
