# Design Review: Correct End State

## Executive Summary

KiriView already has a clear intended direction in `docs/architecture/`: C++ owns authoritative runtime state, Rust policy/reducer code computes value decisions where practical, QML renders projections and reports local facts, and facades should expose QML API rather than own domain behavior. The main design risk is that several important features only partially follow that direction. State clusters still have public setters outside their transition paths, session and facade objects still coordinate too many workflows, and some feature modules leak across archive, document, session, decoding, rendering, and QML boundaries.

The most important correct end state is not a broad rewrite. It is to finish landing the repository's existing architectural intent: centralize invariant-coupled state transitions, move duplicated policy into single owners, isolate external effects behind narrow ports, and make facades and QML consume typed projections rather than recomputing shared decisions.

## Top Design Risks

1. Invariant-coupled state is still representable as independent fields in `ImageDocumentState`, `VideoDocumentState`, and `ImagePresentationRuntime`, so valid snapshots depend on call order rather than on central transition APIs.
2. `DocumentSessionRuntime` and `KiriViewApplication` still own broad routing and coordination responsibilities, making session behavior, actions, thumbnails, direct media, deletion, Open With, and predecode harder to reason about independently.
3. Thumbnail, opened-collection, predecode, and secondary-page behavior cross module boundaries in ways that make features hard to remove or replace cleanly.
4. Core workflow tests depend on real Qt/KDE effects such as `QThreadPool::globalInstance()`, `QTimer`, `KCoreDirLister`, DBus, filesystem notifications, and broad test link targets.
5. Failures are mostly represented as raw `QString` values, with some errors displayed directly, some swallowed, and no consistent distinction between user-facing messages, internal diagnostics, retryable adapter errors, and fatal runtime errors.

## Single Source of Truth Violations

### Finding: Display image cache budget has competing defaults

**Priority:** P1

#### Evidence

- `src/policy/cachebudget.rs` computes display cache budget from a caller-provided preferred display budget.
- `src/cache/imagecachepolicy.cpp` resolves `request.displayImageCachePreferredByteBudget`.
- `src/document/imagedocumentruntimedependencies.cpp` defaults `displayImageCachePreferredByteBudget` from `imageFullDecodeFallbackByteLimit`.
- `src/rendering/staticimage.h` defines `imageFullDecodeFallbackByteLimit = 512 * 1024 * 1024`.
- `src/rendering/displayimagestore.cpp` has a separate `fallbackDisplayStoreByteBudget = 64 * 1024 * 1024` and resolves an empty `ImageCacheBudgetRequest`.
- `src/presentation/imagepagesurfacecontroller.cpp` stores a resolved display budget but uses `m_displayImageStore->byteBudget()` when a shared store is present.

#### Current state

Document runtime dependency construction and shared display-store construction can resolve different default display-image budgets. The page surface controller can then use one budget when no store exists and a different budget when a shared store is injected.

#### Design concern

Display-image cache budget is a product and safety policy, but it has multiple defaults. Refinement and cache behavior can depend on composition path rather than a single budget rule.

#### Correct end state

The cache policy layer should own one named default for display-image cache budget. `DisplayImageStore`, document runtime dependency construction, and page surface controllers should all receive or derive the same resolved value. If full-decode fallback size intentionally informs display-cache budget, that relationship should be encoded as an explicit policy name rather than as an incidental reuse of `imageFullDecodeFallbackByteLimit`.

#### Suggested migration

Add characterization tests that compare document dependency budgets and `DisplayImageStore` defaults under the same memory snapshot. Introduce one display-image preferred-budget provider in cache policy. Make `imageDocumentCacheBudgetRequestWithDefaults()` and `DisplayImageStore` delegate to that provider or receive the same resolved budget from application/session composition.

#### Acceptance criteria

- Only one production default for display image cache preferred budget remains.
- `DisplayImageStore` no longer has an unrelated private fallback budget.
- `ImagePageSurfaceController` does not silently switch between unrelated budget sources.
- Tests prove the shared store and document dependency path resolve the same budget.

### Finding: Display bucket, memory safety, and rotation rules are duplicated

**Priority:** P1

#### Evidence

- `src/policy/rasterdisplaybucketpolicy.rs` defines raster display demand validation, desired bucket sizing, rotation axis swap, size safety, and `DISPLAY_BYTES_PER_PIXEL`.
- `src/rendering/rasterdisplaybucketpolicy.cpp` defines SVG-specific demand validation, desired scale, rotation axis swap, display size safety, and display bucket decision logic.
- `src/cache/imagebytecost.cpp` estimates RGBA byte cost independently.
- `src/policy/imagerendergeometry.rs` owns `rust_normalized_image_rotation_degrees()` and `rust_image_rotation_swaps_axes()`.
- `src/policy/imagetilegeometry.rs`, `src/policy/rasterdisplaybucketpolicy.rs`, and `src/rendering/rasterdisplaybucketpolicy.cpp` repeat rotation normalization or axis-swap logic.

#### Current state

Raster bucket decisions live in Rust policy and are bridged to C++. SVG bucket decisions live in C++ and repeat common demand, safety, byte-cost, and rotation concepts. Rotation policy exists as a shared render-geometry policy but is also reimplemented in tile and bucket policies.

#### Design concern

Display safety is correctness-sensitive because it protects texture and memory limits. Drift between raster, SVG, tile geometry, render geometry, and byte-cost estimation can produce inconsistent bucket decisions for the same image, rotation, or memory budget.

#### Correct end state

A single display bucket policy should own common validation, rotation, byte-cost, and safety status semantics. SVG-specific scale behavior can be a parameter or strategy, but common safety and axis-swap behavior should be shared, preferably in the Rust policy layer already used by raster decisions. Thin C++ wrappers are acceptable; duplicate production implementations are not.

#### Suggested migration

Extract shared Rust helpers for normalized rotation, axis swapping, demand validation, RGBA byte cost, and safe-size checks. Move SVG bucket decision into Rust or make the C++ SVG path call the shared primitives. Then remove local C++ safety helpers and local Rust rotation copies.

#### Acceptance criteria

- Only one production implementation of quarter-turn normalization and axis swapping remains, excluding bridge wrappers.
- `src/rendering/rasterdisplaybucketpolicy.cpp` no longer contains local copies of demand validation or display-image safety checks.
- Raster and SVG bucket tests cover the same memory and rotation edge cases.
- Byte-cost assumptions are named once and consumed by cache and bucket policy.

### Finding: Opened-collection thumbnail identity eligibility is duplicated

**Priority:** P2

#### Evidence

- `src/archive/openedcollectionthumbnailpolicy.cpp` has `rootSchemeSupportsContentThumbnailIdentity()` for `zip` and `sevenz`.
- `src/archive/mediaentrysourcebackend_karchive.cpp` has `rootSchemeSupportsThumbnailMetadata()` for the same root schemes.
- `src/session/documentsessionruntime.cpp` uses `documentSessionThumbnailSourceAdapter()` to consume `openedCollectionThumbnailSourcePlan()`.
- `docs/architecture/thumbnail-source-adapters.md` describes v1 support as image entries inside comic-book archives backed by `zip` or `sevenz` roots.

#### Current state

The session planner decides that certain opened-collection entries can provide stable thumbnail identity, and the KArchive backend independently decides whether it can provide thumbnail metadata for the same schemes and entry kinds.

#### Design concern

Planner and backend support can drift. A new checksum-capable archive backend or changed archive eligibility rule would require coordinated edits in at least two private helper functions.

#### Correct end state

Archive thumbnail identity eligibility should have one owner in the archive or opened-collection source layer. The session thumbnail planner should consume a capability fact or shared policy rather than duplicating archive scheme knowledge.

#### Suggested migration

Add a shared archive capability function or method for thumbnail content identity support. Use it from both `openedcollectionthumbnailpolicy.cpp` and `mediaentrysourcebackend_karchive.cpp`. Keep the external behavior unchanged.

#### Acceptance criteria

- No duplicated `zip`/`sevenz` thumbnail metadata helper remains in production code.
- Planner and backend use the same capability source.
- Tests cover supported CBZ/CB7 image entries and rejected non-image, non-comic-book, and unsupported archive cases.

### Finding: Filename extension parsing is duplicated in Rust policy

**Priority:** P2

#### Evidence

- `src/policy/imageinputclassification.rs` defines `extension_for_file_name(name)`.
- `src/policy/archiveformat.rs` defines a private `extension_for_file_name(name)`.
- `src/policy/imageformatregistry.rs` and `src/policy/videoformatregistry.rs` import the image classification extension helper.

#### Current state

Image and video format matching share one parser, while archive format matching uses a duplicate parser.

#### Design concern

Extension parsing edge cases such as hidden files, trailing dots, and case normalization are part of file support behavior. Archive matching can drift from image/video matching if either parser changes.

#### Correct end state

A neutral Rust policy helper should own extension extraction. Image, video, and archive format registries should import that helper.

#### Suggested migration

Move `extension_for_file_name()` to a neutral policy module, update image classification and archive format matching to use it, and preserve current behavior with focused tests.

#### Acceptance criteria

- `rg "fn extension_for_file_name"` finds one production implementation.
- Archive, image, and video extension matching use the same helper.
- Tests cover hidden files, trailing dots, and uppercase extensions.

### Finding: RTL navigation presentation ordering is recomputed in QML

**Priority:** P2

#### Evidence

- `src/navigation/imageshortcutnavigationpolicy.cpp` owns physical arrow mapping with `horizontalArrowAction()` and `singlePageArrowAction()`.
- `src/facade/kiriviewapplication.cpp` applies that policy in `executeHorizontalArrowShortcut()` and `executeSinglePageArrowShortcut()`.
- `src/qml/ImageDocumentPageNavigation.qml` defines `leftNavigationAction` and `rightNavigationAction` from `rightToLeftReadingActive`.
- `src/qml/ImageActions.qml` constructs `applicationMenuNavigationActions` with RTL-dependent ordering.
- `src/qml/ApplicationMenuBar.qml` maps leading/trailing menu actions and first/last image icons based on RTL state.

#### Current state

Keyboard physical-arrow semantics are centralized in C++ policy, but toolbar and menu presentation order is recomputed in several QML files.

#### Design concern

The concept "leading/trailing navigation action under the active reading direction" has multiple presentation definitions. Toolbar, menu, and keyboard behavior can drift as navigation evolves.

#### Correct end state

One navigation presentation projection should expose ordered leading/trailing/first/last action slots for the active reading direction. QML should render that projection. Physical keyboard shortcut policy may remain separate, but should share the same direction-mapping primitive where applicable.

#### Suggested migration

Add a small navigation ordering policy or projection. Replace QML ternaries with projected action references. Add verification for LTR and RTL toolbar, menu bar, and application menu ordering.

#### Acceptance criteria

- Production QML no longer directly maps `rightToLeftReadingActive` to previous/next navigation actions.
- One policy or projection owns RTL-aware presentation ordering.
- LTR and RTL ordering are verified across the relevant presentation surfaces.

### Finding: Wheel zoom step conversion is duplicated in QML

**Priority:** P3

#### Evidence

- `src/qml/ImageZoomControls.qml` defines `wheelAngleDeltaPerStep`, `wheelZoomStepCount(wheel)`, and `handleZoomWheel()`.
- `src/qml/ImageViewport.qml` defines the same wheel angle divisor and conversion helper in `handleWheelZoom()`.

#### Current state

Both QML components convert wheel delta into zoom step count with the same `120` divisor. `ImageZoomControls.qml` applies an additional `0.5` multiplier that is not named as policy.

#### Design concern

This is a small duplication, but touchpad/wheel normalization changes would need coordinated edits. The toolbar sensitivity difference may be intentional, but it is not explicit.

#### Correct end state

One QML helper or exposed input policy should own wheel-to-zoom-step conversion. Any per-surface sensitivity difference should be named.

#### Suggested migration

Extract wheel step normalization into a shared helper and keep toolbar scaling as a named multiplier if current behavior is desired.

#### Acceptance criteria

- `wheelAngleDeltaPerStep` and `wheelZoomStepCount()` appear once in production code or in a shared helper.
- Toolbar-specific wheel scaling, if retained, has a named constant or policy.

## Invariant and Correctness Risks

### Finding: Image document open state can be partially mutated outside the central transition workflow

**Priority:** P1

#### Evidence

- `src/document/imagedocumentstate.h` exposes independent setters for source URL, source kind, displayed image location, status, loading, error string, container navigation URL, unsupported opened-collection video, and metadata.
- `src/document/imagedocumentstate.cpp` applies those setters independently.
- `src/document/imageopentransition.h` defines `ImageOpenStateDelta`, but it does not include every open-state fact, such as source kind, unsupported opened-collection video, or embedded metadata.
- `src/document/imageopentransitionapplier.cpp` applies transition fields one by one to `ImageDocumentState`.
- `src/document/imageopencontroller.cpp` combines workflow transitions with additional direct state mutations.
- `src/document/imageopencontroller.cpp` handles unsupported opened-collection video by manually batching state setters instead of expressing the case as a normal transition outcome.

#### Current state

The image-open workflow centralizes some loading/status/error/displayed-location behavior, but several related facts are still set directly by `ImageOpenController`. Unsupported opened-collection video is a hand-built state cluster rather than a first-class workflow outcome.

#### Design concern

The valid image document state is a cross-field invariant. Production code can represent combinations such as ready status with stale error text, loading true with a terminal status, or unsupported opened-collection video with unrelated source facts if a call path misses part of the setter sequence.

#### Correct end state

Image document open state should change through one transition/reducer API that owns source kind, source URL, displayed location, loading, status, error, container navigation, unsupported opened-collection video, and metadata. Low-level setters for invariant-coupled fields should be private, test-only, or available only to a narrow transition applier.

#### Suggested migration

Add characterization tests for empty source, resolved source, successful image load, load error, container navigation error, and unsupported opened-collection video. Extend `ImageOpenStateDelta` or introduce a richer typed result that covers all invariant-coupled fields. Move unsupported opened-collection video into the same transition path.

#### Acceptance criteria

- All production image-open outcomes are expressed through one transition path.
- `ImageDocumentState` cannot be driven to inconsistent status/loading/error/displayed-location combinations by production code.
- Unsupported opened-collection video has explicit transition tests.
- Direct setters for invariant-coupled fields are private, removed, or guarded by a transition applier.

### Finding: Video status and error text are independent mutable facts

**Priority:** P2, uncertain

#### Evidence

- `src/video/videodocumentstate.h` exposes `setStatus()` and `setErrorString()` independently.
- `src/video/videodocumentstate.cpp` clears error on source reset/load but implements status and error setters separately.
- `src/video/videodocumentruntime.cpp` sets error text and status for source-load failure.
- `src/video/videodocumentruntime.cpp` updates status from backend media status without clearing error text when the computed status is non-error.
- `src/session/documentsessionpublicprojection.cpp` exposes `input.video.errorString` for video sessions independent of video status.

#### Current state

Video status and error string are modeled as separate fields. Error paths usually set both, but later non-error backend status updates can change status without clearing a previous error string.

#### Design concern

A public state such as `Ready` with a stale nonempty error string is representable. This may or may not occur with supported Qt backends, but the invariant is not encoded in the design.

#### Correct end state

Video load state should be represented as one status/error value where `Error` carries error text and non-error states cannot carry one. Public projection should receive normalized video state rather than combining status and error fields independently.

#### Suggested migration

Add a characterization test for backend error followed by a loaded/buffered status, or explicitly document and test that backend errors are terminal for the current supported backend behavior. Replace direct production use of `setStatus()` and `setErrorString()` with a transition method or plan applier that clears, preserves, or replaces error text centrally.

#### Acceptance criteria

- A non-error `VideoDocumentStatus` cannot be published with stale error text through production paths.
- Tests cover error, recovery-or-terminal semantics, source reset, and source-load failure.
- Public document-session projection receives normalized video error state.
- Direct independent status/error mutation is removed from production paths or restricted behind a central applier.

### Finding: Presentation mode and secondary-page visibility can form impossible snapshots

**Priority:** P2

#### Evidence

- `src/presentation/imagepresentationruntime.h` exposes independent mutators for two-page mode, mode, secondary-page visibility, primary slot, and secondary slot.
- `src/presentation/imagepresentationruntime.cpp` masks `secondaryPageVisible()` so it only returns true in `TwoPageSpread`.
- `src/presentation/imagepresentationruntime.cpp` mutates `m_mode` and `m_secondaryPageVisible` independently.
- `src/presentation/imagepresentationruntime.cpp` returns `currentSnapshot()` with raw `m_mode` and raw `m_secondaryPageVisible`.
- `src/presentation/imagespreadpresentationcontroller.cpp` manually sequences two-page mode and secondary visibility outcomes.

#### Current state

The runtime stores presentation mode, two-page setting, secondary slot, and secondary visibility as separate mutable fields. One getter masks invalid visibility, but the snapshot exposes the raw stored value.

#### Design concern

`ImagePresentationSnapshot` can expose impossible combinations, such as single-page mode with secondary page visibility true. Consumers may accidentally depend on raw invalid values.

#### Correct end state

Presentation state should be represented as an invariant-preserving mode state, for example `SinglePage` versus `TwoPageSpread` with explicit secondary-slot semantics. Transitions such as "finish secondary page visible," "finish secondary as primary," and "clear secondary page" should be atomic runtime operations.

#### Suggested migration

First harden `currentSnapshot()` to emit normalized visibility. Add tests that assert snapshots cannot report secondary visibility in single-page mode. Then introduce runtime transition methods for existing controller sequences and collapse mode, visibility, and slot fields into a typed presentation-state value when practical.

#### Acceptance criteria

- `ImagePresentationSnapshot` cannot expose `SinglePage` with `secondaryPageVisible == true`.
- Production code no longer sets secondary visibility and mode as separate public operations for one domain transition.
- Controller methods call atomic runtime transitions for spread/single-page outcomes.
- Tests cover visible secondary, hidden secondary, clear secondary, two-page disable, and transition-state behavior.

### Finding: QML-owned revision counters are used as C++ stale gates

**Priority:** P2

#### Evidence

- `src/qml/Main.qml` defines `property int actionUiGateRevision`, increments it, and passes it to `updateActionUiGateSnapshot(...)`.
- `src/facade/kiriviewapplication.h` exposes `updateActionUiGateSnapshot(quint64 revision, ...)`.
- `src/facade/kiriviewapplication.cpp` rejects only snapshots whose revision is lower than `m_actionUiGateRevision`.
- `src/qml/VideoViewport.qml` defines `property int videoOutputClaimRevision`, increments it, and sends it to `reportVideoOutputSurfaceClaim(...)`.
- `src/facade/kiridocumentsession.h` exposes `reportVideoOutputSurfaceClaim(quint64 claimRevision, quint64 projectionRevision, ...)`.
- `src/session/documentsessionruntime.cpp` uses `claimRevision < m_videoOutputSurfaceClaimRevision` as a stale-claim guard.

#### Current state

QML manufactures integer revisions used by C++ to reject stale action UI gate snapshots and video output surface claims.

#### Design concern

The layer enforcing monotonicity does not own the monotonic token. QML `int` counters cross into C++ `quint64` stale gates, so wrap, reuse, or uncoordinated call paths can weaken stale-update rejection.

#### Correct end state

C++ should own revision generation for stale-sensitive state, or QML should receive opaque tokens that it cannot arithmetically manipulate. QML should report facts and object identity, not manufacture correctness tokens.

#### Suggested migration

Introduce C++ token owners for action UI gate snapshots and video output surface claims. Replace QML integer counters with opaque tokens or C++-issued handles, keeping the existing `quint64` API temporarily behind an adapter while tests are added.

#### Acceptance criteria

- No QML `property int` is used as a monotonic stale token for C++ correctness gates.
- Stale action UI and video output updates are rejected based on C++-owned identity.
- Tests prove older tokens cannot overwrite newer state.
- Tests prove equal or reused tokens cannot publish different facts unless explicitly allowed.

## Cohesion, Coupling, and Ownership Problems

### Finding: `DocumentSessionRuntime` concentrates too many coordinator roles

**Priority:** P1

#### Evidence

- `src/session/documentsessionruntime.cpp` is one of the largest files in the repository and coordinates active navigation, public projections, routing, video output claims, direct media, predecode, deletion, and Open With.
- `src/session/documentsessionruntime.h` dependency construction includes navigation, deletion, Open With, thumbnail, predecode, direct media, image document, and video document dependencies.
- `src/session/documentsessionruntime.cpp` `connectDocuments()` wires image and video leaf signals to session projection and action availability refresh.
- `src/session/documentsessionruntime.cpp` `executeRoutePlan()` applies operations across session state, image document, video document, direct navigation, predecode, and projection refresh.
- `src/session/documentsessionpublicprojection.cpp` already contains a pure projection layer, but `DocumentSessionRuntime` still assembles large leaf snapshots and owns many side effects.

#### Current state

`DocumentSessionRuntime` is the application-level coordinator for mixed-media sessions, but it also stores and orchestrates multiple feature runtimes and side-effect workflows.

#### Design concern

The session runtime has unclear internal ownership boundaries. Changes to one feature can affect routing, projections, thumbnails, deletion, direct media, and predecode through a single class. That makes the file hard to audit and makes regressions more likely.

#### Correct end state

`DocumentSessionRuntime` should own session-level composition and revisioned public snapshots. Feature-specific runtimes should own their own state and side effects behind narrow ports: active navigation thumbnails, direct media navigation, media deletion, Open With, video output claims, and predecode coordination. The session runtime should issue high-level commands, receive typed outputs, and update `DocumentSessionState`.

#### Suggested migration

Keep current behavior and add characterization tests around route execution, projection refresh, active document switches, deletion completion, Open With availability, and direct-media navigation. Extract one feature coordinator at a time, starting with high-churn areas such as active navigation thumbnails or direct-media predecode. Preserve `DocumentSessionPublicProjection` as the pure projection boundary.

#### Acceptance criteria

- `DocumentSessionRuntime` no longer stores low-level job state for thumbnails, deletion, Open With, direct media, and predecode directly.
- Each feature runtime has a small public port and focused tests.
- Session projection refresh consumes typed feature outputs rather than reaching into each leaf workflow.
- `DocumentSessionPublicProjection` remains pure and owns public snapshot derivation.

### Finding: Thumbnail infrastructure is not a self-contained feature boundary

**Priority:** P1

#### Evidence

- `src/facade/kiridocumentsession.h` exposes thumbnail model, demand/result enums, activation, bucket calculation, and demand reporting through the session QML facade.
- `src/qml/ThumbnailPanel.qml` binds to `activeNavigationThumbnailModel` and computes demand bucket/priority before reporting it back to the session.
- `src/session/documentsessionruntime.h` injects thumbnail lookup, generation, source adapter, and image-store dependencies into `DocumentSessionRuntime`.
- `src/session/activenavigationthumbnailruntime.h` owns model, lookup/generation jobs, row state, background work, image retention, and demand handling.
- `src/session/thumbnailgeneration.cpp` imports archive, decoding, rendering, Rust thumbnail cache, and session cache lookup, then loads local or opened-collection bytes, decodes, scales, converts, and installs thumbnails.
- `src/decoding/imagedecodejob.cpp` uses the thumbnail cache lookup provider for main-image XDG thumbnail preview delivery.
- `src/decoding/thumbnailpreview.h` imports rendering and session thumbnail cache/demand types.

#### Current state

Thumbnail behavior is spread across QML demand reporting, QML-facing session API, session runtime scheduling, cache lookup/generation, archive metadata, image decoding, rendering payloads, and main-image thumbnail preview.

#### Design concern

The active-navigation thumbnail strip and main-image thumbnail preview are separate product behaviors, but they share session-owned cache/demand types and low-level generation code without a neutral thumbnail subsystem. Removing or replacing thumbnails would require edits across UI, facade, session, decoding, archive, and rendering.

#### Correct end state

Thumbnail cache identity, lookup, generation, and XDG cache integration should live in a dedicated thumbnail module. Active navigation should be one client adapter of that module. Main-image preview should be another client adapter. `decoding/` should not depend on session thumbnail demand/cache types.

#### Suggested migration

Move `ThumbnailCacheLookupProvider`, generation requests/results, bucket conversion, and original identity into a neutral `thumbnail/` module. Give active navigation a `SessionThumbnailStripRuntime` adapter that owns row state and demand policy. Give image decode/open a separate preview adapter that consumes neutral lookup results and maps them to `StaticDisplayImagePayload`.

#### Acceptance criteria

- `src/decoding/` no longer includes `session/activenavigationthumbnaildemand.h` or `session/thumbnailcachelookup.h`.
- `DocumentSessionRuntimeDependencies` no longer exposes raw thumbnail cache/generation providers directly; it receives one thumbnail-strip dependency or port.
- Active-navigation thumbnails and main-image thumbnail previews can be disabled or replaced independently in tests.
- Default thumbnail generation has a focused core with injectable file/archive bytes, decoder, cache repository, and scaling policy.

### Finding: Opened-collection/archive support leaks across feature boundaries

**Priority:** P1

#### Evidence

- `src/archive/mediaentrysourcebackend.h` imports location and navigation page types and makes `MediaEntrySource` return navigation candidates, image data, and thumbnail metadata.
- `src/archive/mediaentrysourcestore.h` wraps both `ImageDocumentPageCandidateProvider` and `ImageDecodeDependencies` around opened-collection scope handling.
- `src/document/imageloader.cpp` branches into `LoadOpenedCollectionScopeCandidates` and loads opened-collection candidates before decoding.
- `src/predecode/predecodecache.h` stores opened-collection scope directly in predecode requests and cache entries.
- `src/predecode/predecodepolicy.cpp` varies predecode source profile based on `OpenedCollectionScopeLocation`.
- `src/session/thumbnailgeneration.cpp` loads opened-collection thumbnail bytes and handles opened-collection thumbnail metadata and virtual identity.
- `src/session/mediaopenwithplan.cpp` imports archive format rules into session Open With availability.

#### Current state

Opened-collection support is represented directly in archive backends, navigation candidate loading, document loading, predecode cache keys, predecode policy, thumbnail generation, and session Open With policy.

#### Design concern

Archive/directory collection support is a source extension, but it is not behind one extension boundary. Removing archive support or changing how collections are loaded would require coordinated edits across many modules.

#### Correct end state

Opened collections should be represented by a source adapter contract that owns collection opening, entry listing, entry bytes, entry metadata, and capability facts. Navigation, document loading, predecode, thumbnails, and Open With should consume typed capabilities and results from that adapter rather than importing archive-specific rules or carrying raw `OpenedCollectionScopeLocation` pairs everywhere.

#### Suggested migration

Extract an `OpenedCollectionSourcePort` with list, read bytes, thumbnail metadata, and capability methods. Make `MediaEntrySourceStore` the first implementation. Then migrate thumbnail and Open With policies to consume adapter capability facts.

#### Acceptance criteria

- `session/` modules no longer include `archive/archiveformat.h` for opened-collection operation policy.
- Thumbnail generation consumes an opened-collection metadata/bytes port, not global `loadMediaEntrySource*` functions.
- Predecode keys and cache entries use a typed opened-collection entry key or source handle instead of raw URL plus scope pairs.
- Archive support can be excluded from a focused build/test fixture by replacing one adapter.

### Finding: Two-page spread presentation owns secondary-page loading

**Priority:** P1

#### Evidence

- `src/presentation/imagespreadpresentationcontroller.h` imports cache, decoding, document state, navigation, predecode, and rendering dependencies.
- `src/presentation/imagespreadpresentationcontroller.h` constructs `ImageSpreadPresentationController` with `ImageDocumentState`, `ImageDocumentPageCandidateProvider`, `ImageDecodeDependencies`, and `ImageCacheBudgets`.
- `src/presentation/imagespreadpresentationcontroller.cpp` constructs `ImageSecondaryPageController` from presentation code.
- `src/presentation/imagespreadpresentationcontroller.cpp` starts secondary-page load using document state and first-display decode context.
- `src/presentation/imagesecondarypagecontroller.cpp` imports `document/imageloader.h`, constructs `ImagePageSurfaceController` and `ImageLoader`, and starts image loading.

#### Current state

The spread presentation controller owns both spread layout/state and secondary-page load workflow, including candidate resolution, predecode lookup, decode dependencies, image loader construction, and page-surface presentation.

#### Design concern

Presentation is deciding how a secondary slot is resolved and loaded, not just how a ready slot is presented. That pulls document/source-loading concerns into a module that should own presentation state and projection.

#### Correct end state

Presentation should request a secondary page slot by URL/page identity and consume a typed load result. Document/image-load ownership should decide candidate resolution, predecode hit/miss, decode jobs, stale-load rejection, and load errors. Presentation should own spread state, secondary visibility, geometry, and display projection after a slot result is accepted.

#### Suggested migration

Introduce a secondary-page load request/result port. Move `ImageLoader` construction out of `ImageSecondaryPageController` into document-owned loading code. Keep `ImageSecondaryPageController` as a page-slot presenter that accepts slot-ready or slot-failed events.

#### Acceptance criteria

- `src/presentation/` no longer includes `document/imageloader.h`.
- `ImageSpreadPresentationController` no longer receives `ImageDecodeDependencies` or `ImageDocumentPageCandidateProvider`.
- Secondary-page tests verify ready, failed, stale, opened-collection, and predecode-hit behavior through the new load boundary.

### Finding: Decoding, rendering, and session thumbnail dependencies form confusing ownership cycles

**Priority:** P1

#### Evidence

- `src/decoding/decodedimageresult.h` returns `StaticDisplayImagePayload` from `src/rendering/staticimage.h`.
- `src/decoding/thumbnailpreview.h` imports rendering display types and session thumbnail cache/demand types.
- `src/decoding/qimagereaderdecoder.cpp` and `src/decoding/heifdecoder.cpp` create rendering `ImageTileSource` implementations.
- `src/rendering/qimagereadertilesource.cpp` imports `src/decoding/bufferedimagereader.h`.
- `src/rendering/heiftilesource.h` imports `src/decoding/heiftilingplan.h`.
- `src/session/thumbnailgeneration.cpp` consumes decoding and rendering internals for thumbnail generation.

#### Current state

Decoding returns rendering payloads, rendering sources import decoding helpers, decoding imports session thumbnail types for preview, and session thumbnail generation imports both decoding and rendering.

#### Design concern

The language boundary is not the issue here; the ownership boundary is. Decode result, display payload, thumbnail preview, and source refinement are tightly interwoven, which makes it hard to change one feature without touching several layers.

#### Correct end state

Decoding should return neutral decoded media/source results. Rendering should own display payload construction and refinement source adapters. Thumbnail preview should depend on a neutral thumbnail/cache port, not on session-specific demand types. Where shared implementation helpers are needed, they should live in a neutral module with one directional dependency.

#### Suggested migration

Start by moving thumbnail cache/demand types out of `session/`. Then split static display refinement source types from decoding result types. Preserve existing decode/render behavior while making dependency direction explicit.

#### Acceptance criteria

- `decoding/` no longer imports session thumbnail demand/cache headers.
- Rendering source helpers do not require decoding-internal headers unless those helpers are moved to a neutral support module.
- Decode result APIs are understandable without session thumbnail concepts.

### Finding: Application actions are not owned at one removable boundary

**Priority:** P1

#### Evidence

- `src/application/kiriviewapplicationactions.cpp` defines the canonical action table with ids, names, text, icons, shortcuts, scopes, and categories.
- `src/application/applicationactionstatepolicy.cpp` has a separate switch over action ids for enabled/checked/placement state.
- `src/facade/kiriviewapplication.cpp` has `handleRuntimeActionTriggered(...)` with another switch over action ids for command dispatch and shell signals.
- `src/qml/ImageActions.qml` manually constructs action arrays and instantiates one `ManagedAction` per action id.
- `src/facade/kiriviewapplication.h` defines a QML-facing `ActionId` enum parallel to `KiriView::ApplicationActions::ActionId`.

#### Current state

There is a canonical action definition table, but command routing, availability, placement, QML object instantiation, and facade enum exposure repeat action identity across layers.

#### Design concern

Adding, removing, or reshaping an action requires lockstep edits across application policy, facade routing, QML placement, and tests. This increases the risk of orphaned commands, stale menu entries, or enabled actions without valid dispatch.

#### Correct end state

Action identity should remain canonical in the application layer. Command dispatch should live in an application command router with explicit ports. Placement/group metadata should be data-driven from the action registry or from one typed presentation model. The facade should expose QML surfaces and shell callbacks, not switch over every domain action.

#### Suggested migration

Extract `handleRuntimeActionTriggered(...)` into an application command router first. Then move QML menu grouping and placement toward a generated or projected action model, leaving QML to render action groups rather than hand-list every action id.

#### Acceptance criteria

- `KiriViewApplication` no longer switches over every domain `ActionId`.
- Action command routing is unit-testable without a QML-facing facade object.
- Adding or removing an action requires updating the canonical action definition plus one clear placement/command metadata location, not hand-edited QML arrays and a facade switch.

### Finding: Image removal fallback policy creates a document/navigation ownership cycle

**Priority:** P2

#### Evidence

- `src/navigation/imagedocumentpagenavigationcontroller.cpp` includes `document/imageremovalfallback.h` and uses it when the current page disappears.
- `src/document/imageremovalfallback.h` and `.cpp` import navigation and location types, sort candidates, and compute fallback targets.
- `src/document/imagedocumentdeletioncontroller.cpp` also uses `imageRemovalPlanForDisplayedLocation()`.

#### Current state

The fallback policy for a displayed image removed from its candidate set lives in `document/`, but navigation also uses it.

#### Design concern

The policy is really about candidate navigation after removal. Its placement in `document/` creates a small but real cross-layer cycle and makes ownership unclear.

#### Correct end state

Removal fallback should live in a neutral navigation policy module or in `navigation/`, with document and deletion code consuming the same policy.

#### Suggested migration

Move the fallback value policy to a navigation policy location and keep call sites unchanged through a compatibility include if needed. Add tests that cover deletion and live-candidate removal through the shared policy.

#### Acceptance criteria

- Navigation code no longer imports a document-owned fallback policy.
- Document deletion and live navigation use the same neutral removal fallback policy.
- Tests prove current fallback behavior is preserved.

## Logic Placement and Flow Predictability

### Finding: Shared action availability depends on QML-computed presentation facts

**Priority:** P1

#### Evidence

- `src/qml/Main.qml` computes and publishes action UI gate facts, including `page.imageMode && mediaWorkspaceHost.imageInteractionSurface.imagePannable`.
- `src/qml/ImageViewport.qml` derives `imagePannable` from `root.presentationActive && root.imageDocument.viewportPannable`.
- `src/facade/kiriviewapplication.cpp` builds `ImageActionAvailabilityInput` from `facts.imageReady && m_imagePannable`.
- `src/application/applicationshortcutruntime.cpp` gates fixed pan shortcuts on `pannableViewerShortcutsEnabled`.
- `src/qml/VideoViewport.qml` handles video seek shortcuts locally with hard-coded `Alt+Left`, `Alt+Right`, `Alt+Up`, and `Alt+Down`, checking `videoDocument.seekable` before calling `seekBy`.

#### Current state

Action availability and shortcut behavior partly live in application policy, partly in the facade, and partly in QML. QML supplies a shared pannability fact used by application action availability, and video seek shortcuts bypass the application shortcut runtime.

#### Design concern

QML is no longer just rendering projections and reporting local facts. It is combining presentation and document state into shared command availability, while another set of shortcuts is implemented outside the central action/shortcut runtime. This makes command behavior hard to audit.

#### Correct end state

Application action and shortcut availability should be computed by application/runtime policy from authoritative state and typed leaf facts. QML may report local viewport facts, but it should not own shared command decisions. Fixed video seek shortcuts should be registered and routed through the same application shortcut runtime as image pan/zoom/navigation shortcuts.

#### Suggested migration

Define a C++-owned action gate snapshot input that distinguishes local viewport facts from domain decisions. Move video seek shortcut definitions into the application action/shortcut registry. Keep QML event handlers only as input capture where Qt requires it, forwarding events to the central runtime.

#### Acceptance criteria

- QML no longer combines image mode, presentation activity, and pannability to decide shared action availability.
- Video seek shortcuts are represented in the application action/shortcut registry or a clearly equivalent central runtime.
- Application action availability tests cover image pannability and video seekability without QML.
- QML input handlers are thin event forwarding surfaces.

### Finding: `KiriViewApplication` facade owns broad command routing

**Priority:** P1

#### Evidence

- `src/facade/kiriviewapplication.cpp` `handleRuntimeActionTriggered(...)` dispatches file/open-with/deletion/navigation/zoom/rotation/pan/fullscreen/help actions.
- `src/facade/kiriviewapplication.cpp` `handleScanForwardAction()`, `handleScanBackwardAction()`, `executeHorizontalArrowShortcut()`, `executeSinglePageArrowShortcut()`, and `executeVerticalPanShortcut()` contain command routing logic and use `m_navigationPolicy`.
- `src/application/applicationactionruntime.cpp` owns QAction setup/state but delegates action triggers back to facade callbacks.

#### Current state

The facade is a CXX-Qt/QML-facing object, but it still decides how many domain commands route to documents, sessions, shell signals, and navigation policy.

#### Design concern

This violates the intended thin-facade boundary and makes command logic harder to test without facade construction and QML-facing concerns.

#### Correct end state

`KiriViewApplication` should expose QML properties, action objects, and shell signals. An application command router in `src/application/` should own action dispatch and depend on explicit ports for file dialog requests, session commands, image document commands, video commands, shell/help signals, and navigation policy.

#### Suggested migration

Extract command routing into a pure or Qt-light application command router while leaving facade method signatures in place. Have the facade translate QML action triggers into router calls and emit shell-facing signals from router effects.

#### Acceptance criteria

- Command dispatch is unit-testable without a CXX-Qt facade.
- `KiriViewApplication` no longer contains broad action switch statements.
- Navigation shortcut policy is consumed by the application command router.
- QML-facing action objects continue to behave the same.

### Finding: Fit mode toolbar keeps a second mutable selected state

**Priority:** P2

#### Evidence

- `src/qml/ImageToolBar.qml` defines mutable `selectedFitMode`.
- `src/qml/ImageToolBar.qml` has `syncSelectedFitModeFromZoomMode()` and `triggerFitMode(zoomMode)` assigning `selectedFitMode = zoomMode` before the action has necessarily completed.
- `src/qml/FitModeMenuButton.qml` uses `selectedFitMode` for checked state.
- The authoritative flow is through `KiriImageDocument::requestFitMode(...)` and `ImagePresentationRuntime::setFitMode(...)` / `zoomMode()`.

#### Current state

QML mirrors fit mode locally for menu selection while C++ owns the actual presentation zoom mode.

#### Design concern

The UI can temporarily or permanently display a selected fit mode that does not match the authoritative runtime if a command is rejected, delayed, or superseded.

#### Correct end state

The menu checked state should come from the authoritative presentation/runtime projection. QML may use transient pressed/hover states, but not a second durable selected-fit-mode state.

#### Suggested migration

Bind fit mode checked state to the document/presentation projection. If optimistic UI is required, model it explicitly as pending command state with clear invalidation.

#### Acceptance criteria

- QML no longer has a durable `selectedFitMode` mirror of runtime zoom mode.
- Fit mode menu checked state follows the authoritative projection.
- Tests or QML verification cover command rejection and repeated fit-mode changes.

### Finding: Menu access-key routing depends on reflective QML contracts and multiple global filters

**Priority:** P2

#### Evidence

- `src/facade/menuaccesskeyrouter.cpp` installs a `QCoreApplication` event filter in each `MenuAccessKeyRouter` instance.
- `src/qml/ApplicationMenuBar.qml`, `src/qml/ImageToolBar.qml`, and `src/qml/ContextActionMenu.qml` create multiple routers.
- `src/application/menuaccesskeymenuruntime.cpp` detects menus by `object->inherits("QQuickMenu")` and string properties such as `opened`, `visible`, and `enabled`.
- `src/application/menuaccesskeymenuruntime.cpp` invokes `itemAt`, reads `subMenu`, reads `text`, calls `popup`, sets `accessKeysActive`, and calls `click` through string-named QObject APIs.
- `src/qml/MenuActionItem.qml` declares the `accessKeysActive` hook consumed by C++.

#### Current state

Mnemonic behavior is implemented as C++ runtime logic over arbitrary QML `QObject` menus, with several router instances observing global events and mutating QML item state through string names.

#### Design concern

The C++/QML contract is implicit and reflective. QML menu changes can break C++ behavior without compiler feedback, and multiple router instances participate in application-wide event filtering.

#### Correct end state

There should be one menu access-key owner per application/window. Menu interaction should use an explicit typed adapter contract for item enumeration, submenu access, triggering, and mnemonic visual state. Alternatively, the feature should be kept local in QML and the C++ reflective adapter removed.

#### Suggested migration

Keep `MenuAccessKeySessionState` as the pure state machine. Introduce a typed menu adapter and route one event-filter owner to the active adapter. Replace normal operation through `QObject::property()` and `invokeMethod()` string contracts incrementally.

#### Acceptance criteria

- Normal menu access-key routing does not rely on string-named `itemAt`, `subMenu`, `popup`, `click`, or `accessKeysActive`.
- There is one event-filter owner for menu access-key routing.
- Existing nested submenu and mnemonic tests pass through the explicit adapter.

## Testability Problems

### Finding: Async worker execution is a hidden global dependency

**Priority:** P1

#### Evidence

- `src/async/imageasyncworker.h` `runAsyncWorker` dispatches through `QThreadPool::globalInstance()` and delivers through `QCoreApplication::instance()` / `QMetaObject::invokeMethod`.
- `src/async/imageioworkerjob.h` `startImageIoWorkerJob` delegates to `runAsyncWorker`.
- `src/decoding/imagedecodejob.cpp` calls `runAsyncWorker` directly for decode and raw embedded thumbnail preview validation.
- `src/archive/mediaentrysourceruntime.cpp` calls `runAsyncWorker` in `startCandidateLoad` and uses `startImageIoWorkerJob` for opened-collection image data.
- `tests/cpp/test_imagedecodejob.cpp` uses `QSemaphore`, `QThreadPool::globalInstance()->waitForDone(5000)`, and repeated waits.
- `tests/cpp/test_imageioworkerjob.cpp` uses eventual `QTRY_*` and `QTest::qWait` patterns.

#### Current state

Async work is scheduled by a free template function that chooses Qt global thread-pool behavior whenever a QObject context and application instance exist.

#### Design concern

Core workflow tests become timing-sensitive and depend on global thread-pool state. Ordering and stale-completion behavior are harder to prove deterministically.

#### Correct end state

Async execution should be represented by an explicit executor/scheduler port with production Qt-thread-pool and test inline/manual implementations. `ImageDecodeJob`, `MediaEntrySourceRuntime`, `startImageIoWorkerJob`, and default worker-backed providers should receive or resolve that executor through dependencies.

#### Suggested migration

Introduce a minimal `WorkerScheduler` or `AsyncExecutor` interface. Keep `runAsyncWorker` as the production adapter. Add defaulted executor fields to `ImageDecodeDependencies` and runtime/provider dependency structs. Convert `ImageDecodeJob` first, then `MediaEntrySourceRuntime`, then `startImageIoWorkerJob` users.

#### Acceptance criteria

- `ImageDecodeJob` stale-result tests run without changing `QThreadPool::globalInstance()` and without `QTest::qWait`.
- `MediaEntrySourceRuntime` candidate/data completion ordering can be driven by a manual executor.
- Production behavior still uses the Qt thread pool by default.

### Finding: Timer and clock behavior lacks a manual scheduler seam

**Priority:** P1

#### Evidence

- `src/predecode/predecodescheduleruntime.h` owns `QTimer m_debounceTimer`, `QTimer m_neutralTimer`, and `QElapsedTimer m_monotonicClock`.
- `src/predecode/predecodescheduleruntime.cpp` starts the monotonic clock, configures timers, and reads `currentMonotonicMsec()`.
- `src/predecode/predecodeschedulestate.cpp` is pure and accepts `monotonicMsec`.
- `src/presentation/imageanimationplayer.h` owns `QTimer m_timer`.
- `src/presentation/imageanimationplayer.cpp` directly starts `m_timer`.
- `tests/cpp/test_predecodescheduleruntime.cpp`, `tests/cpp/test_imagedocumentpredecodecontroller.cpp`, and `tests/cpp/test_imageanimationplayer.cpp` use waits and timeouts.

#### Current state

Runtime objects directly own real Qt timers and monotonic clocks. Tests verify eventual behavior by waiting for wall-clock time and processing Qt events.

#### Design concern

Predecode and animation behavior is time-sensitive. Tests are slower and can be flaky under load, even though the predecode state layer already accepts explicit timestamps.

#### Correct end state

Timer and clock effects should live behind a runtime scheduler boundary. Production should use `QTimer` / `QElapsedTimer`; tests should use manual clock and timer handles. Domain state should continue receiving timestamps and events as plain values.

#### Suggested migration

Add `MonotonicClock` and single-shot timer scheduler dependencies for `PredecodeScheduleRuntime`. Replace direct `QTimer` use in `ImageAnimationPlayer` with a frame scheduler. Convert wait-based tests to advance the manual clock or fire scheduled callbacks.

#### Acceptance criteria

- Predecode debounce and neutral-settle tests do not call `QTest::qWait`.
- Animation playback loop/restart/error tests can trigger scheduled frames synchronously.
- Production defaults still use `QTimer` and `QElapsedTimer`.

### Finding: Directory listing and live candidate refresh are tied to real `KCoreDirLister`

**Priority:** P1

#### Evidence

- `src/navigation/imagedocumentpagecandidatedirectoryentry.cpp` constructs `KCoreDirLister`, calls `openUrl`, and wires to `KCoreDirLister` / `KIO::Job` signals.
- `src/navigation/imagedocumentpagecandidatedirectoryentry.cpp` uses `QTimer::singleShot(0, ...)` in `notifyChanged`.
- `src/async/directorylistingjob.cpp` constructs another `KCoreDirLister` for one-shot listing.
- `src/navigation/directmedianavigationcandidateprovider.cpp` defaults to `startDirectoryItemList`.
- `tests/cpp/test_imagedocumentpagecandidatedirectoryentry.cpp`, `tests/cpp/test_imagedocumentpagecandidatestore.cpp`, and `tests/cpp/test_directorylistingjob.cpp` use temp directories, KDirNotify, and long waits.

#### Current state

Directory candidate discovery and live updates are implemented directly on top of KDE directory listers and filesystem notifications.

#### Design concern

Candidate refresh is core navigation behavior, but edge cases such as rapid updates, stale callbacks, errors, and deletion ordering are hard to test without real filesystem/KDE behavior and long timeouts.

#### Correct end state

Directory listing and watching should be behind injectable provider/event-source abstractions. Production can use `KCoreDirLister`; tests should provide synthetic snapshots, changes, deletions, and errors. Candidate conversion should remain pure.

#### Suggested migration

Introduce `DirectoryListingProvider` and `DirectoryWatchProvider` ports. Refactor `ImageDocumentPageCandidateDirectoryEntry`, `startDirectoryItemList`, and default direct media navigation candidate provider to consume the same production adapter.

#### Acceptance criteria

- Directory entry/store tests can load, change, delete, and error without `QTemporaryDir`, `KDirNotify`, or 10-second waits.
- One integration test may remain for the `KCoreDirLister` adapter.
- Direct media navigation and image page candidate stores use the same directory provider boundary.

### Finding: Video playback URL resolver default adapter is only integration-testable

**Priority:** P2

#### Evidence

- `src/video/videoplaybackurlresolver.cpp` private `KioFuseVideoPlaybackUrlResolver` directly calls `KProtocolInfo::protocolClass`, `QDBusConnection::sessionBus()`, `QDBusInterface`, and `QDBusPendingCallWatcher`.
- `src/video/videoplaybackurlresolver.cpp` `createDefaultVideoPlaybackUrlResolver()` always returns the KIOFuse resolver.
- `src/video/videosourceloadruntime.cpp` accepts `VideoPlaybackUrlResolver`, but default construction creates the concrete resolver.
- `tests/cpp/test_videoplaybackurlresolver.cpp` uses event loops, timers, session bus checks, and skips when KIOFuse or a KIO worker is unavailable.

#### Current state

Higher-level video source loading can use a fake resolver, but the default resolver's protocol classification and DBus mount behavior are embedded in a private concrete class.

#### Design concern

DBus errors, invalid mount replies, stale callbacks, and cancellation behavior cannot be covered deterministically. Important adapter coverage can disappear on machines without KIOFuse.

#### Correct end state

Split default resolution into pure URL-resolution policy plus injectable platform ports: protocol classifier and KIOFuse mount client. Production uses DBus; tests use a fake mount client.

#### Suggested migration

Extract protocol classification and add a `KioFuseMountClient` interface. Keep `createDefaultVideoPlaybackUrlResolver()` wiring the real classifier/client. Add unit tests with fake success, failure, delayed completion, cancellation, and invalid paths.

#### Acceptance criteria

- Resolver tests for success, DBus failure, invalid mount path, and cancellation run without session bus or KIOFuse.
- Existing skip-prone tests are reduced to adapter smoke coverage.
- `VideoSourceLoadRuntime` continues to use resolver injection unchanged.

### Finding: C++ test target structure forces broad integration dependencies

**Priority:** P2

#### Evidence

- `tests/cpp/CMakeLists.txt` builds `kiriview_test_core` from both `src/cpp_core_sources.txt` and `src/cpp_cxxqt_sources.txt`.
- `tests/cpp/CMakeLists.txt` links `kiriview_test_core` against Qt Core/DBus/Gui/Multimedia/Qml/Quick/QuickControls2/Widgets, KF6 Archive/Config/I18n/KIO/JobWidgets/Service, Kirigami, LibArchive, LibHeif, LibRaw, LibPng, and the Rust static library.
- Every `test_*.cpp` executable links `kiriview_test_core`.
- `src/cpp_core_sources.txt` mixes pure policy/runtime files with adapters such as archive backends, directory listing, application runtime, decoding backends, rendering, presentation, and facade-adjacent logic.

#### Current state

All C++ tests, including narrow policy and state tests, link against one broad static app-core target.

#### Design concern

The build boundary does not reflect the intended architecture. Small tests inherit heavy platform dependencies, slowing iteration and making pure logic isolation harder to maintain.

#### Correct end state

C++ code should be split into smaller linkable targets matching ownership boundaries: pure policy/state/runtime helpers, Qt runtime without KDE side effects, KDE/KIO adapters, rendering/codec adapters, and facades. Tests should link the narrowest target that contains the behavior under test.

#### Suggested migration

Add a small target for pure C++ policy/state/runtime code. Move adapter files into separate targets while preserving the app build. Update tests incrementally, starting with state/policy tests.

#### Acceptance criteria

- Pure tests for action policy, cache policy, navigation policy, and state can build without KIO, QML, multimedia, LibRaw, LibHeif, or LibArchive.
- Adapter tests still link required adapter libraries.
- Adding a pure logic test does not require broad platform dependencies by default.

## Error Handling and Observability Problems

### Finding: Runtime media failures collapse into raw strings

**Priority:** P1

#### Evidence

- `src/decoding/decodedimageresult.h` uses `DecodedImageFailure { QString errorString; }`.
- `src/document/imageloadtypes.h` `ImageLoadError` has only broad values such as `Generic` and `EmptyOpenedCollection`.
- `src/async/imageasynccallbacks.h` defines shared `ErrorCallback = std::function<void(const QString&)>`.
- `src/archive/mediaentrysourcebackend.h` uses `MediaEntrySourceError { QString errorString; }`.
- `src/video/videosourceloadplan.h` `PublishVideoSourceLoadFailureOperation` carries only a `QString`.
- `src/system/filedeletion.cpp` can report deletion failure with an empty `QString` for an empty target.

#### Current state

Many layers propagate failure as raw text, sometimes with a small status enum, and the same string may serve as user-facing message, internal diagnostic, and control signal.

#### Design concern

The system cannot consistently distinguish recoverable adapter failures, user-visible validation errors, internal diagnostics, stale/cancelled completions, and fatal runtime failures. This makes behavior harder to test and harder to debug.

#### Correct end state

Use typed domain failure values with at least kind, user-facing text or text id, diagnostic detail, affected source identity, retryability where applicable, and severity. Raw backend strings should be diagnostic payloads, not the primary error representation.

#### Suggested migration

Start with foreground image/video source load failures and directory listing errors. Add typed error structs while preserving current user-visible text. Then migrate async callbacks and archive/decode providers to return typed failures. Finally, update public projections to map typed failures to localized display text.

#### Acceptance criteria

- Foreground image and video load failures publish typed failure values internally.
- Public UI text is derived from typed errors, not directly from backend strings.
- Tests assert error kind and user-facing message separately.
- Empty or vague failure strings are replaced by explicit error kinds.

### Finding: Backend diagnostics can be displayed directly to users

**Priority:** P1

#### Evidence

- `src/video/videoplaybackurlresolver.cpp` emits raw strings such as invalid video URL text and DBus `reply.error().message()`.
- `src/video/videodocumentruntime.cpp` stores resolver and backend error strings in document state.
- QML displays video and image document error strings through document/session projections.

#### Current state

Some infrastructure strings become document error strings and can appear in the UI.

#### Design concern

Backend messages may be vague, unlocalized, too technical, or inconsistent. Internal diagnostics and user-facing text should not be the same field.

#### Correct end state

Infrastructure adapters should produce typed diagnostic failures. Presentation should map those failures to localized, safe user-facing messages while logs retain backend detail.

#### Suggested migration

Introduce error ids for video URL resolution and media backend failures. Preserve backend messages in diagnostics. Map current user-visible messages through a presentation/localization layer.

#### Acceptance criteria

- DBus/KIO/Qt backend messages are not directly displayed as primary user-facing text.
- Logs or diagnostics retain raw backend detail.
- Tests prove user-facing text and diagnostic text can differ.

### Finding: Adjacent container navigation listing errors are swallowed

**Priority:** P1

#### Evidence

- `src/navigation/imagecontainernavigationcontroller.cpp` error callbacks ignore the provided listing error string.
- `src/navigation/imagecontainernavigationcontroller.cpp` finishes container navigation list errors without preserving the diagnostic.
- Direct media navigation providers and tests show a contrasting path that forwards or logs explicit errors.

#### Current state

When adjacent container navigation listing fails, the error text is discarded.

#### Design concern

Navigation failures become difficult to diagnose, and behavior can appear as simply no adjacent media rather than a failed listing.

#### Correct end state

Navigation listing errors should be represented as typed effects or diagnostics, even if the user-facing behavior remains quiet. The system should distinguish "no candidates" from "listing failed."

#### Suggested migration

Add a typed container-listing failure result and thread it through `ImageContainerNavigationController`. Decide whether it affects UI, logs only, or both. Preserve current user-visible behavior until a product decision changes it.

#### Acceptance criteria

- Listing failure and empty listing are distinct internal states or effects.
- The original diagnostic is available to logs/tests.
- Tests cover list success, empty result, and list failure.

### Finding: Background thumbnail, predecode, and refinement failures lose diagnostics

**Priority:** P2

#### Evidence

- `src/predecode/predecodeloadcontroller.cpp` error callbacks ignore the supplied string, and `finishLoadError` logs only generation/source identity.
- `src/session/thumbnailgeneration.cpp` creates detailed error strings, but `src/session/activenavigationthumbnailruntime.cpp` primarily logs lookup/generation status rather than a consistent diagnostic payload.
- `src/presentation/imagepagesurfacecontroller.cpp` `runRasterDisplayRefinement()` captures `errorString` from `decodeRasterDisplayImage(...)` and immediately discards it with `Q_UNUSED(errorString)`.

#### Current state

Background failures often preserve enough state to avoid breaking UI, but diagnostics are not consistently retained.

#### Design concern

These failures are hard to debug when they affect performance, cache behavior, thumbnail quality, or image refinement. Silent background failure also makes tests assert only high-level status, not cause.

#### Correct end state

Background workflows should have lightweight diagnostics with source identity, operation kind, failure kind, and backend detail when available. Not every background failure needs user-facing UI, but it should be observable in logs or test hooks.

#### Suggested migration

Introduce a small diagnostic event type for background image operations. Thread it through predecode load, thumbnail lookup/generation, and raster refinement. Use typed error kinds as they become available.

#### Acceptance criteria

- Predecode, thumbnail, and refinement tests can assert failure cause where relevant.
- Logs include operation kind and source identity for background failures.
- Diagnostic collection does not change current user-visible behavior.

### Finding: Retry and cancellation semantics are implicit

**Priority:** P2, uncertain

#### Evidence

- KIO directory listing, KIOFuse DBus resolution, image decode workers, thumbnail generation, and predecode loads mostly perform single attempts.
- Existing code has stale-result and cancellation checks in several runtimes, but retryability is not represented as an error property.

#### Current state

Retries appear to be absent or ad hoc, and failures do not generally encode whether retrying would be useful.

#### Design concern

Without explicit retry/cancellation semantics, future retry behavior could be added inconsistently or at the wrong layer. This is uncertain because the current product may intentionally prefer single-attempt operations.

#### Correct end state

Typed failures should identify retryability where it matters. Retry policy should be owned by effect adapters or workflow runtimes, not by UI call sites. If no retries are desired, that should be an explicit product policy.

#### Suggested migration

Do not add retries immediately. First add failure kinds and diagnostic ownership. Then decide per adapter whether retryability is relevant and test those semantics.

#### Acceptance criteria

- Retryability is either explicitly absent by policy or represented on typed failures for relevant adapters.
- No UI call site invents retry behavior independently.

## Deletion, Modularity, and Abstraction Problems

### Finding: Static display refinement still exposes a tile-era abstraction

**Priority:** P2

#### Evidence

- `src/rendering/staticimage.h` defines `ImageTileSource` with mandatory `decodeTile(...)`, first-display decode, raster refinement, and blocking-display methods.
- `src/rendering/staticimage.h` stores `std::shared_ptr<ImageTileSource> refinementSource` in `StaticDisplayImagePayload`.
- `src/decoding/qimagereaderdecoder.cpp` opens a `QImageReaderTileSource` and passes it into static decoded image results.
- `src/decoding/staticimagedecode.cpp` builds display payloads from `ImageTileSource` but active production consumers use first-display or whole-display methods.
- `src/presentation/imagepagesurfacecontroller.cpp` uses `supportsRasterDisplayRefinement()` and `decodeRasterDisplayImage(...)`.
- `src/session/thumbnailgeneration.cpp` uses `refinementSource->decodeBlockingDisplayImage(...)`.
- `src/rendering/qimagereadertilesource.cpp`, `src/rendering/heiftilesource.cpp`, `src/rendering/svgtilesource.cpp`, and `src/decoding/rawdecoder.cpp` still implement `decodeTile(...)`.

#### Current state

The active provider-display path consumes whole display images and whole-raster refinement, but the payload-facing source interface still requires tile vocabulary and APIs.

#### Design concern

The abstraction preserves an apparent visual tile extension point even though the architecture direction says provider display is cache-only whole-image display. This makes source helpers harder to simplify and invites future work down the wrong path.

#### Correct end state

Replace `ImageTileSource` as the payload-facing type with a whole-image `StaticDisplayRefinementSource` interface: source size, first-display decode, blocking thumbnail/display decode, whole-raster refinement, byte cost, transform, and resolution-independence. Any source-internal tile mechanics should be private to specific implementations.

#### Suggested migration

Add a new whole-image refinement interface and adapt existing sources to it. Keep `decodeTile` only as an internal helper while tests migrate. Remove `ImageTileSource::decodeTile` from payload-facing APIs once production and tests use the whole-image interface.

#### Acceptance criteria

- `StaticDisplayImagePayload` no longer stores `ImageTileSource`.
- Static image source implementations are not required to expose `DecodedTile` or `TileRequest` through the public refinement interface.
- Production code contains no visual tile decode entry point except private source-internal helpers where still needed.

### Finding: Predecode coordination is split across two similar owner paths

**Priority:** P2

#### Evidence

- `src/predecode/imagepredecodecoordinator.h` defines `ImagePredecodeCoordinator`, owning a candidate repository, `PredecodeLoadController`, and `PredecodeScheduleRuntime`.
- `src/predecode/mediapredecodecoordinator.h` defines `MediaPredecodeCoordinator`, also owning `PredecodeLoadController` and `PredecodeScheduleRuntime`.
- `src/document/imagedocumentpredecodecontroller.cpp` constructs `ImagePredecodeCoordinator` from image document state, page surface, presentation runtime, candidate provider, decode dependencies, power saver, and cache budget.
- `src/session/documentsessionruntime.cpp` schedules media predecode from direct-media candidates and caches displayed images for media predecode.
- `src/document/imagedocumentpredecodecontroller.cpp` includes an `ordinaryDirectMediaPredecodeEnabled` gate.

#### Current state

Image-document predecode and mixed direct-media predecode share core load/schedule components, but each has a separate coordinator and integration path.

#### Design concern

The split is understandable by scope, but predecode as a feature remains difficult to disable, test, or reason about because scheduling, power saver, active loads, and cache lifetime are wired through multiple owners.

#### Correct end state

Keep scope-specific planning adapters, but centralize shared predecode runtime ownership: debounce, power saver, active loads, cache lifetime, displayed-image caching, and decode load admission. Image-document and session code should provide candidate windows and displayed-image snapshots through typed ports.

#### Suggested migration

Extract common `PredecodeLoadController` plus `PredecodeScheduleRuntime` ownership into a shared runtime facade. Make `ImagePredecodeCoordinator` and `MediaPredecodeCoordinator` thin planners. Move ordinary direct-media gating to the session/direct-media predecode boundary.

#### Acceptance criteria

- Only one runtime class owns predecode load scheduling/cache lifetime.
- Image-document and direct-media predecode differ by planning input, not duplicate runtime ownership.
- Disabling ordinary direct-media predecode is represented at the session/direct-media boundary.

### Finding: Typed source-key families are declared but not enforced at runtime boundaries

**Priority:** P2

#### Evidence

- `src/location/sourcekey.h` declares family-specific key structs: `OrdinaryFileSourceKey`, `DirectMediaSourceKey`, `DirectMediaScopeKey`, `ImageDocumentPageSourceKey`, `OpenedCollectionEntrySourceKey`, `ThumbnailSourceKey`, `PredecodeCandidateKey`, and `RenderSurfaceKey`.
- `src/session/activenavigationthumbnailruntime.h` defines a separate `ActiveNavigationThumbnailSourceKey` instead of using `ThumbnailSourceKey`.
- `src/predecode/predecodecache.h` uses `QUrl` plus `OpenedCollectionScopeLocation` for predecode requests and cached images instead of `PredecodeCandidateKey`.
- `src/presentation/imagepagesurfacecontroller.cpp` builds raster refinement demand keys from display identity, original size, page role, and revisions instead of `RenderSurfaceKey`.
- `src/decoding/staticimagedecode.cpp` and `src/decoding/qimagereaderdecoder.cpp` derive plain string identities from `sourceKeyForUrl(...).identity`.

#### Current state

Typed key families exist, but runtime boundaries still carry parallel local key structs, raw URL/scope pairs, or plain strings.

#### Design concern

This is an incomplete abstraction. The declared key families do not prevent unrelated key families from being compared or serialized incorrectly at stale-completion, cache, thumbnail, predecode, or render-demand boundaries.

#### Correct end state

Either adopt typed source-key families at actual runtime boundaries, or remove unused family abstractions and keep only the generic source key until a boundary needs typed identity. The code should not contain both declared family types and parallel runtime-specific key shapes for the same concept.

#### Suggested migration

Pick one boundary first, likely thumbnails or predecode. Replace the local key shape with the typed family key and add tests proving cross-family keys cannot be compared accidentally. Then migrate render demand/source identity or delete unused key families that remain unadopted.

#### Acceptance criteria

- `ThumbnailSourceKey`, `PredecodeCandidateKey`, and `RenderSurfaceKey` are used by their corresponding runtime boundaries, or removed if not needed.
- Parallel local key structs for the same family are deleted or narrowed to UI-only row metadata.
- Tests assert equality/stale rejection through typed key families rather than raw strings or URL/scope pairs.

## Recommended Correct End-State Architecture

KiriView's correct end state should preserve the existing Rust + CXX-Qt + Kirigami structure, but make ownership stricter.

Ownership boundaries should be explicit. `src/application/` should own action identity, command routing, shortcut policy, and high-level app commands. `src/facade/` should expose QML-facing API and forward commands/events. `src/session/` should own mixed-media session state, active document selection, and public projections. Feature runtimes such as thumbnails, direct media, deletion, Open With, and predecode should sit behind narrow session ports instead of being stored and operated directly inside `DocumentSessionRuntime`.

Domain rules should live in policy/reducer modules, preferably Rust where that is already the local pattern. Display cache budgets, display bucket safety, rotation normalization, extension parsing, navigation ordering, source identity, and open-state transitions should each have one owner. Thin C++ wrappers and QML projections are fine; duplicate rule implementations are not.

State should be defined as invariant-preserving values. Image open state, video load state, and presentation mode/secondary-page state should be changed through typed transitions, not through independent public setters. Stale-sensitive state should use C++-owned revision tokens or opaque handles, not QML integer counters.

Validation should happen at the boundary that owns the invariant. Source/load validation should be part of document or source adapter transitions. Action availability should be computed by application policy from authoritative runtime facts. Presentation should validate presentation state and expose normalized snapshots. QML should render projections and report local input facts only.

External effects should be isolated behind ports. Async workers, timers/clocks, directory listing/watching, KIOFuse DBus mounts, thumbnail cache/filesystem access, archive bytes/metadata, and decode adapters should have production implementations plus manual/fake test implementations. Pure state and policy tests should not link or boot broad KDE/QML/codec dependencies.

Errors should be represented as typed failures. User-facing messages, localization ids, backend diagnostics, source identities, retryability, and severity should not be collapsed into one `QString`. Background operations should emit lightweight diagnostic events even when user-visible behavior remains quiet.

Tests should be layered around these boundaries. Characterization tests should lock current behavior before migration. Pure policy/state tests should use narrow link targets and no Qt/KDE effects where practical. Runtime tests should use manual executors, manual clocks, fake directory providers, fake mount clients, and typed source adapters. Adapter smoke tests should remain for KDE, DBus, codec, and filesystem integration.

## Suggested Refactoring Sequence

1. Add characterization tests around current behavior: image open outcomes, video error/status transitions, presentation spread snapshots, action routing, thumbnail cache identity, opened-collection metadata, directory listing failure, and predecode scheduling.
2. Centralize duplicated rules/state: display cache budgets, bucket safety and rotation, extension parsing, opened-collection thumbnail eligibility, RTL navigation ordering, and wheel zoom normalization.
3. Isolate core domain logic from external effects: introduce worker executor, clock/timer scheduler, directory provider, KIOFuse mount client, thumbnail generation core, and opened-collection source port.
4. Clarify ownership boundaries: extract application command router, narrow `DocumentSessionRuntime`, move secondary-page loading out of presentation, and move thumbnail infrastructure into a neutral thumbnail module.
5. Improve error semantics and observability: introduce typed foreground load failures first, then navigation listing diagnostics, then background thumbnail/predecode/refinement diagnostics.
6. Remove or simplify premature abstractions: replace payload-facing `ImageTileSource` with whole-image refinement source, adopt or delete typed source-key families, and consolidate predecode runtime ownership.

## Things Not To Change Yet

- Do not rewrite the Rust/C++ boundary wholesale. The current policy/reducer direction is useful; the problem is incomplete ownership, not the language split itself.
- Do not move authoritative runtime state into QML. QML should become thinner for shared decisions, not more stateful.
- Do not change user-visible supported formats or archive behavior while centralizing extension and archive policies.
- Do not add retries before typed errors and retryability semantics exist.
- Do not remove thumbnail, opened-collection, predecode, or two-page features as part of this design cleanup. The first goal is to make each feature removable and testable, not to remove it.
- Do not rebuild the rendering pipeline around visual tiles. The architecture direction is whole-image/provider display; the tile-era interface should shrink, not expand.
- Do not introduce a large generic service framework. Prefer small ports with production and test adapters.

## Appendix: Subagent Reports

### Single Source of Truth / Duplication Agent

Retained findings: display image cache budget defaults, raster/SVG bucket safety duplication, rotation duplication, extension parsing duplication, opened-collection thumbnail eligibility duplication, RTL navigation ordering duplication, and wheel zoom duplication. The display bucket and rotation items were merged because they are one rendering-policy ownership problem. Wheel zoom was kept as P3 cleanup. No finding was added for user-facing spec extension lists because specs are external behavior documentation and not themselves competing production policy.

### Invariant / Correctness Agent

Retained findings: image document open state partial mutation, QML-owned stale revision counters, video status/error separation, and presentation secondary visibility invalid snapshots. The video status/error finding is marked uncertain because more evidence is needed about whether supported Qt media backends can emit recoverable non-error status after an error for the same source.

### Cohesion / Coupling / Ownership Agent

Retained findings: `DocumentSessionRuntime` broad ownership, decoding/rendering/session thumbnail coupling, two-page presentation owning secondary-page loading, `KiriViewApplication` facade command routing, and image removal fallback policy placement. The command-routing finding was merged with the deletion/modularity action-removability report.

### Logic Placement / Flow Readability Agent

Retained findings: shared action availability depending on QML-computed presentation facts, video seek shortcuts bypassing application shortcut runtime, broad facade command routing, fit mode toolbar mirror state, and reflective menu access-key routing. The action availability, video shortcut, and facade routing findings were consolidated into the action/command architecture findings.

### Testability Agent

Retained findings: hidden global async worker execution, missing timer/clock scheduler seam, directory listing tied to real `KCoreDirLister`, KIOFuse resolver only integration-testable, thumbnail generation hidden inside effectful providers, and broad C++ test target structure. The thumbnail testability finding was merged into the thumbnail subsystem boundary finding, with test-specific acceptance criteria preserved.

### Error Handling / Observability Agent

Retained findings: raw string failure model, backend diagnostics displayed directly, swallowed adjacent container navigation errors, lost background diagnostics for thumbnail/predecode/refinement, and implicit retry semantics. Retry semantics are marked uncertain and intentionally sequenced after typed error modeling.

### Deletion / Modularity / Abstraction Agent

Retained findings: thumbnail infrastructure not self-contained, two-page presentation owning secondary-page loading, opened-collection support leaking across boundaries, tile-era static display refinement abstraction, application actions not removable at one boundary, menu access-key reflective contract, split predecode coordination, and typed source-key families not enforced. Findings that overlapped with cohesion, logic placement, and testability reports were merged rather than repeated.
