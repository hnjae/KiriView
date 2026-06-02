# Extension Contracts

KiriView extension points are adapter contracts, not state backdoors. A media source, thumbnail source, predecode planner, decoder, or render source returns typed keys, capabilities, plans, demands, payloads, or completion results to its owning runtime. It must not mutate QML state, physical item state, public session state, platform action state, cache state, or render-node policy directly.

## Shared Contract Shape

Durable identity is the stable key for the thing being addressed. Freshness generation is the owner's proof that a result still belongs to the currently accepted scope. Equality compares durable identity within one key family only; generation is used to accept or reject work for a current lifecycle.

A demand describes requested work from an owner snapshot: key, generation, size bucket, priority, render context, visible area, decode window, or similar inputs. A result reports ready, pending, unsupported, failed, canceled, or stale without changing ownership. Unsupported means the adapter cannot provide the capability for that key; failed means the adapter accepted the demand and could not complete it.

Adapters are synchronous policy or asynchronous payload providers behind an owner. Synchronous adapters return plans and capability facts. Asynchronous adapters return payloads through the owner's lifecycle contract, carrying the owner-held operation id, source key plus generation, demand key, or render-frame revision needed for stale-completion rejection.

Capabilities are descriptive. They can say whether bytes can be read, thumbnails can be generated or cached, a still image can be predecoded, a decode route is supported, or tiles can be requested. Capabilities must not imply ownership of public state or platform side effects.

## Source Key Families

Ordinary file keys use the top-level source-key contract: normalized path segments, local path cleanup, absolute local identity for relative local paths, fully encoded identity strings, preserved query and fragment, case-sensitive comparison, and no symlink, alias, shortcut, or platform filesystem equivalence resolution.

Direct video keys use the same ordinary direct-media identity rules as direct images. Playback source resolution belongs to the video document runtime; source keys identify routing, navigation, thumbnails, predecode eligibility, and stale completion checks only.

Direct-media scope identity is `{ current key, parent key, generation }`. The current and parent keys first use navigation-source handling, including document-portal host paths and KIOFuse archive restoration, then apply source-key normalization. The generation changes when the effective current key or parent key changes. Pending direct-image confirmation to an equivalent displayed URL is a phase change within the same generation.

Image-document page keys identify a page URL plus its image-document source scope. Ordinary direct image pages use the direct media key. Opened-collection pages include the opened collection scope and page URL. Page keys preserve page kind so image and video rows do not share render, thumbnail, or predecode capabilities by accident.

Opened-collection scope keys identify the backing collection file URL, collection root URL, and collection kind. Comic-book archive, general archive, and directory scopes are distinct even if URLs match. A collection entry key adds the entry URL relative to that scope.

Archive root keys identify the backing archive file and root URL. Archive-entry freshness is owned by the opened collection backend. Thumbnail cache writes may use archive-record virtual originals only when the backend exposes public entry metadata that satisfies the thumbnail contract.

Directory collection keys identify the directory scope root and selected entry or representative inputs. Directory traversal and representative choice belong to a collection-owned adapter or runtime, not to QML demand. Cache writes require a stable directory freshness rule; otherwise the adapter returns in-memory-only or unsupported.

Thumbnail source keys identify the projected active-navigation row, its source kind, row number, URL, label, page kind, and navigation generation. Thumbnail cache original identity is separate from row source identity. Local files use Freedesktop file-original identity. Cacheable opened-collection entries use the virtual original defined by the thumbnail source adapter contract. In-memory-only sources skip XDG lookup and cache installation.

Predecode candidate keys identify still-image payloads eligible for adjacent decode. Direct media predecode is still-image-only; videos may be cursor positions for window planning, but they do not produce cached video frame payloads. Opened-collection predecode candidates carry the opened collection scope so byte access stays behind the media entry source owner.

Render surface keys identify the current displayed surface generation, page role, and render source family. Tile keys identify tile level, coordinates, and scale bucket within that surface generation. Render-frame revisions identify projected draw entries and missing tile demands for one GUI-to-render handoff.

## Adapter Contracts

Media entry source adapters list and read opened-collection entries. They return candidates, image bytes, and optional thumbnail metadata through the media entry source owner. They do not update document source state, page navigation, deletion state, thumbnails, or QML models directly.

Thumbnail source adapters consume thumbnail source keys and demand buckets, then return unsupported fallback, cacheable local-file generation, cacheable opened-collection entry generation, or in-memory-only generation. The document-session thumbnail runtime owns scheduling, lookup, generation jobs, cache installation, image-store retention, result projection, cancellation, and stale-completion rejection.

Predecode planners consume session or image-document snapshots and produce still-image decode windows. The predecode runtime owns debounce, power-saver suppression, active load admission, decode jobs, cache lifetime, and generation acceptance.

Decoder contracts are route based. Rust-owned byte classification selects one decode route from plain bytes and file-name context. C++ executes the selected decoder and treats selected-decoder failure as final for that request. A decoder returns decoded static image, animation reader payload, metadata, unsupported, or failure; it must not route to another decoder or mutate document state.

Render-source contracts produce immutable render entries and missing demands from a presentation-owned surface snapshot. Tile sources decode only requested tile payloads. Scene-graph nodes consume prepared entries, own graphics resources, and follow Qt scene graph lifecycle rules for `updatePaintNode`, `releaseResources`, node destruction, scene-graph invalidation, window changes, device loss, texture creation and destruction thread, and QRhi or `QSGTexture` ownership. They must not read GUI or domain state to recompute source selection, cache policy, tile priority, or fallback behavior.

## Contract Tests

Contract tests should cover unsupported media, stale source keys or generations, cache identity, opened-collection entry freshness, video-versus-image eligibility, render-source fallback behavior, and surface-generation invalidation after window or device changes. Prefer focused C++ tests where the contract depends on `QUrl`, `QImage`, Qt object lifetime, or render data, and Rust tests where the logic is plain value policy.
