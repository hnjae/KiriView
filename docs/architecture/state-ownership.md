# State Ownership

For QObject-facing workflows, C++ owns the authoritative runtime state. This includes QML-facing properties, Qt notification ordering, `QUrl`, `QImage`, `QString`, async job lifetime, presentation objects, and rendering objects.

Rust reducers operate on value snapshots and plain events. They return explicit state deltas, transition plans, and effect descriptions. Those results describe what C++ should apply; they are not an independent authoritative copy of the same workflow state.

Rust-owned state is reserved for self-contained Qt-independent domains where the state can be represented as plain values and does not mirror authoritative C++ state. Examples include format parsing state and geometry or zoom algorithms. Navigation indices, cache policy state, or other workflow state may move to Rust only when that ownership is documented in this file or an ADR and exposed through value-based FFI.

Avoid split-brain state. A workflow value must have one canonical owner. If both languages need to observe it, one side owns the value and the other side receives a derived snapshot, projection, delta, or completion event.

## Current Cross-Language Ownership

These defaults apply until this document or an ADR explicitly changes the owner. Moving policy into Rust does not move the authoritative runtime state unless the same decision also names the new state owner.

- Actions and shortcuts: C++ owns action identity, KDE action collections, the canonical configured shortcut list, shortcut persistence, and the active shortcut handling installed in Qt. QML presents and invokes C++ actions. Rust may compute stateless shortcut projections, such as menu-safe representatives or derived viewer aliases, but it must not store shortcut state.
- Document and image loading: C++ owns the current document kind, displayed URL or document page, pending load identity, active jobs, decoded image objects, and user-visible load/error state. Rust may compute transition plans from a snapshot, but it must not keep a second displayed-image or pending-selection state.
- Image load sessions: the C++ image loader and session tracker own pending load identity, first-display decode context, archive-document context, and the resolved image URL for direct archive or directory opens. Archive-list completion should update and return the accepted session snapshot; callbacks should not carry a second resolved URL beside the session-owned image URL.
- Supported image lists and page position: C++ owns the confirmed supported image list, current index, boundary state, and pending page selection for a runtime scope unless an ADR gives that scope a different owner. Rust navigation policy may return target indices or follow-up effects from those snapshots.
- Zoom, pan, rotation, scan-start handoff, and spread presentation: the active C++ presentation controller or view facade owns the public presentation state exposed to QML. Rust geometry or spread policy may calculate next values from snapshots. If a presentation mode needs its own state owner, the mode transition must still expose exactly one active public owner.
- View render context: `KiriImageView` owns render-context discovery for the attached document only when it is the primary viewport facade. Secondary page views render secondary snapshots but do not install or clear the document render context provider.
- Location identity: the C++ location domain owns QUrl-based image, container, archive-document, and displayed-image value objects plus normalized URL identity comparison. Runtime domains may store these values as their authoritative state or consume derived snapshots, but they should not invent parallel URL normalization rules.
- Deletion: Rust policy may choose the deletion target and post-delete follow-up target from plain document/navigation snapshots. C++ owns KDE confirmation, the file operation, cancellation and failure handling, notifications, and applying the follow-up plan.
- Preparation and cache: C++ owns prepared image objects, decoder jobs, memory pressure handling, and cache lifetime. Rust may compute preparation priority or eviction policy from plain metadata, but cached runtime objects remain in C++.
- HEIF tiled rendering: C++ owns libheif context and image handles, QImage allocation, tile decoding, and painting into source-rect images. Rust owns tile-grid validation and source-rect tile-region planning from plain numeric tiling metadata.
- System runtime state: C++ owns desktop-environment observer lifetime and platform probes, including power-saver portal subscription state and physical-memory discovery. Rust may compute cache or scheduling policy from plain snapshots derived from those observers.

## Derived Public State

QML-facing values may be derived from multiple C++ runtime states. For example, a public loading or status property may combine document state with an active presentation transition. The derived value must not become a second mutable source of truth. Keep the canonical owners explicit, and make notification dependencies follow the derived value.

When a public value has mode-specific ownership, only the active mode owns that value. Inactive mode state is a cache, projection, or restoration point, not a competing owner. Transition code must synchronize the next active owner before exposing the mode change. For example, different presentation modes may use different internal state owners, but the public value must have exactly one active owner at a time.
