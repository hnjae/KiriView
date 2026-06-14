# Design Review: Correct End State

## Executive Summary

KiriView already has a sound architectural direction: `QML -> facade -> C++ runtime/effect executor -> Rust policy`. The main issue is not the absence of layers. The issue is that several boundaries still allow the same domain rule or state concept to be defined more than once, allow external effects to rely on UI/projection availability checks instead of command-boundary validation, and place too much workflow assembly inside facade or runtime composition objects.

The highest-risk findings are around rules that affect real user behavior and state consistency: image-open state transitions, format capability catalogs, and failure representation. The next major risk is that `DocumentSessionRuntime`, `KiriImageDocument`, `KiriViewApplication`, and `ImageDocumentRuntimeControllers` still act as broad workflow owners even though the codebase already has useful lower-level objects.

The correct end state should be precise and conservative, not clever. Rust policy should own duplicated-free domain decisions and typed plans. C++ runtime should own Qt/KDE objects and external effects behind typed adapters. Facades should expose QML-friendly types and forward commands. QML should report geometry/input facts and render projections. Errors should flow internally as typed failures with source, stage, diagnostic detail, severity, and retryability, while the UI receives only user-facing messages.

## Top Design Risks

1. P1: `DocumentSessionRuntime`, session leaf ports, `ImageDocumentRuntimeControllers`, and `KiriImageDocument` concentrate too many feature workflows and make control flow hard to remove or reason about.
2. P1: Image and file-operation failures are represented as raw strings or discarded metadata, which weakens diagnostics, retry semantics, and user/internal error separation.
3. P2: Image format capabilities and image-open state transitions are not enforced by one central catalog or state-machine boundary.

## Single Source of Truth Violations

### Finding: Supported image format metadata and decode classification are competing catalogs

- Evidence: `src/policy/imageformatregistry.rs` defines `SUPPORTED_IMAGE_FORMATS`, `RAW_IMAGE_EXTENSIONS`, `supported_image_extensions`, and `supported_image_mime_types`; `src/policy/imageinputclassification.rs` defines `RustQtRasterFormat` and `classify_image_input`; `src/decoding/imageinputclassification.h`, `src/bridge/imageinputclassificationconversion.cpp`, and `src/decoding/imagedecodepipeline.cpp` mirror and route those values through `imageDecodeRouteForClassification` and `handlerForRoute`.
- Current state: Openable extension/MIME metadata, byte-signature and extension-fallback classification, FFI enum conversion, and concrete decode routing are separate partial definitions.
- Design concern: Adding or removing a format requires matching edits across the registry, classifier, bridge, and decode route. A format can be advertised by UI/desktop metadata while lacking a decode route, or be decodable while missing from supported metadata.
- Correct end state: A policy-level image format capability catalog should own format family, extension/MIME metadata, and decoder-family capability. Byte-level classification may remain separate, but catalog/route alignment should be enforced by tests.
- Suggested migration: First add policy tests asserting that each meaningful `SUPPORTED_IMAGE_FORMATS` entry has an expected classifier/decoder family. Move `RAW_IMAGE_EXTENSIONS` ownership from the classifier into the capability catalog.
- Acceptance criteria: There is one obvious policy entry point for adding/removing formats; tests fail when registry-supported formats lack classifier/decoder mapping; C++ decode routing remains an execution layer.
- Priority: P2

### Finding: Zoom preset action values are duplicated between metadata and dispatch

- Evidence: `src/application/kiriviewapplicationactions.cpp` declares `ViewZoom50PercentAction`, `ViewZoom100PercentAction`, and `ViewZoom200PercentAction`; `src/application/applicationcommandrouter.cpp` dispatches them with `requestManualZoomPercent(..., 50.0/100.0/200.0)`; `src/application/applicationactionstatepolicy.cpp` repeats zoom labels; `src/qml/ImageActions.qml` exposes managed actions for those presets.
- Current state: Action IDs and labels say 50/100/200, while the router supplies the same values as separate literal payloads.
- Design concern: The visible label/action identity can drift from the actual executed percent. Adding a preset requires checking action declarations, menu text, router literals, and QML placement.
- Correct end state: Zoom presets should be represented by one descriptor table or helper keyed by `ActionId`, with the percent payload declared once and consumed by metadata and dispatch.
- Suggested migration: Introduce a small zoom preset descriptor and remove hard-coded double literals from the router. Add tests aligning action label/name and dispatched percent.
- Acceptance criteria: Each preset value is declared once, and adding a preset requires one descriptor plus intentional UI placement.
- Priority: P2

### Finding: `ImageShortcutScope` validity is enumerated more than once in C++

- Evidence: `src/application/applicationtypes.h` defines `ImageShortcutScope`; `src/policy/imageactionavailability.rs` mirrors it as `RustImageShortcutScope`; `src/bridge/imageactionavailabilityconversion.cpp` converts it; `src/application/imageactionavailabilitypolicy.cpp` defines `imageShortcutScopeKnown`; `src/application/applicationshortcutpolicy.cpp` defines `imageShortcutScopeFromValue`.
- Current state: Rust/C++ FFI mirroring is expected, but C++ also repeats the valid enum list in both known-scope validation and integer conversion.
- Design concern: Adding a scope can leave one path accepting it and another rejecting it.
- Correct end state: C++ should have one canonical validity helper or safe sentinel/range mechanism near `ImageShortcutScope`. Policy and conversion paths should share it.
- Suggested migration: Add a helper such as `knownImageShortcutScope(ImageShortcutScope)` and replace local validation switches. Keep Rust bridge conversion exhaustive, but do not make it a second source of C++ validity.
- Acceptance criteria: C++ scope validity is defined in one place, and a new scope requires one validity update plus intentional policy handling.
- Priority: P2

## Invariant and Correctness Risks

### Finding: Image open transitions can represent contradictory document state

- Evidence: `src/document/imagedocumentstate.h:44` exposes independent setters for `sourceUrl`, `sourceKind`, `displayedImageLocation`, `status`, `loading`, `errorString`, and navigation URLs. `src/policy/imageopenworkflow.rs:146` defines `RustImageOpenStateDelta` with independent field targets. `src/document/imageopentransitionapplier.cpp:270` applies optional fields independently. `tests/cpp/test_imageopenworkflow.cpp:376` constructs `loading == true` with `status == Ready`. `tests/cpp/test_imageopentransitionapplier.cpp:254` can request `status Ready` with `errorString Provided`.
- Current state: Existing workflow builders usually produce coherent transitions, but the types and applier allow combinations such as `Ready + loading`, `Ready + errorString`, or `Error + empty errorString`.
- Design concern: Correctness depends on every transition builder preserving implicit cross-field relationships. Release builds do not protect those relationships with `debug_assert!`.
- Correct end state: Image document state changes should pass through named transition variants or a validating state-machine boundary. The owner should define allowed combinations for `Null`, `Loading`, `Ready`, `Error`, and unsupported states, and validate final state before notifications or effects are emitted.
- Suggested migration: First document current invariants in tests. Add final-state validation to `applyImageOpenApplicationPlan` or a transition factory. Gradually replace open-ended field-target deltas with named transitions for source load begin, success, source error, container error, animation error, and empty source.
- Acceptance criteria: Contradictory transition outputs fail tests unless explicitly allowed; new image-open transitions cannot update status/loading/error/displayed-location independently without the invariant boundary.
- Priority: P2

### Finding: Uncertain - image presentation can expose visible image state without a provider-ready display source

- Evidence: `src/presentation/imagepagesurfacecontroller.cpp:155` returns `hasImage` and `m_displaySource` independently. `src/presentation/imagepagesurfacecontroller.cpp:246` creates a display source even when `DisplayImageStore::insert` returns an empty entry id. `src/rendering/displayimagestore.cpp:237` returns empty for null images or images over budget. `src/presentation/imagepresentationruntime.cpp:724` marks the projection visible when `slot.hasImage` is true and copies the provider URL. `src/qml/DisplayImagePage.qml:28` shows the page when the projection is visible and uses `providerUrl` for the inner image.
- Current state: `hasImage == true` with an empty provider URL or display-error source is representable. More evidence is needed to prove the oversize/budget path is reachable in production, and `setImage` may effectively be test-only.
- Design concern: The visible provider-backed image invariant is not encoded in the type, so a blank page or unnamed display failure may be possible.
- Correct end state: `ImagePresentationPageSlotSnapshot` should use explicit variants such as empty, provider-ready, display-error, and retained/stale presentation. A visible provider-backed state should always include either a valid provider URL or an explicit display-error state consumed by UI/document failure handling.
- Suggested migration: Add tests for `DisplayImageStore` insertion failure and `ImagePageSurfaceController::snapshot` after `setImage`. Decide whether `setImage` should be removed, made test-only, or made invariant-preserving.
- Acceptance criteria: No production `ImageDisplaySourceProjection` can be `visible == true` with an empty provider URL unless it carries an explicit display-error state.
- Priority: P2, uncertain

## Cohesion, Coupling, and Ownership Problems

### Finding: `DocumentSessionRuntime` is a mixed-media feature convergence point

- Evidence: `src/session/documentsessionruntime.h:42-47` groups direct-media navigation, file deletion, Open With, thumbnail runtime, and predecode dependencies. `src/session/documentsessionruntime.h:118-177` declares methods for routing, leaf snapshot refresh, thumbnails, video-output claims, direct-media navigation, deletion, Open With, predecode, and public projection. `src/session/documentsessionruntime.cpp:389-421`, `:578-621`, `:862-990`, `:1054-1159`, and `:1162-1194` handle video-output claims, deletion/Open With, route execution, direct-media navigation, and predecode.
- Current state: `DocumentSessionRuntime` is the public mixed-media session owner and directly executes routing, active navigation, thumbnail scheduling, video output attachment, deletion, Open With, predecode, and projection publication.
- Design concern: Independent feature changes or removals converge on the same runtime and require revalidating routing/projection side effects.
- Correct end state: `DocumentSessionState` should remain the public projection owner. `DocumentSessionRuntime` should orchestrate named subowners through typed inputs/results: route executor, direct-media navigation coordinator, displayed-media deletion coordinator, Open With controller, video-output claim runtime, active-navigation thumbnail runtime, and direct-media predecode adapter.
- Suggested migration: Extract small workflows first, such as Open With or video-output claims. Keep public projection updates centralized in `DocumentSessionState` and move lifecycle execution/cancellation into subowners.
- Acceptance criteria: Feature-specific coordinators can be tested without full session runtime construction; removing thumbnail strip or Open With does not require edits to direct-media routing logic.
- Priority: P1

### Finding: Session-to-leaf document ports expose leaf internals as wide callback bags

- Evidence: `src/session/documentsessiondocumentports.h:31-83` exposes 17 image signal connectors plus source/displayed URL/opened collection/status/deletion/navigation/zoom/presentation/metadata/predecode getters and commands. `src/session/documentsessiondocumentports.h:86-114` lists video getters and commands. `src/facade/kiridocumentsession.cpp:252-340` manually maps facade properties/signals. `src/session/documentsessionruntime.cpp:624-764` connects individual signals and samples individual getters to rebuild snapshots.
- Current state: The port is explicit, but it exposes leaf object shape rather than cohesive snapshot/events.
- Design concern: Adding or moving one projected fact requires coordinated edits in the leaf facade adapter, port struct, signal wiring, snapshot sampling, and projection code.
- Correct end state: Leaf documents should expose session-facing snapshot families plus narrow command ports. The session should consume values such as `ImageDocumentSessionSnapshot` and `VideoDocumentSessionSnapshot` and receive snapshot-changed events.
- Suggested migration: Add snapshot getters/events behind the existing port and gradually move fields into snapshot structs. Once covered by tests, replace individual signal connectors with snapshot-family events.
- Acceptance criteria: `refreshImagePublicSnapshot()` and `refreshVideoPublicSnapshot()` no longer sample every leaf getter directly; adding a projected leaf fact is localized to one snapshot type and one leaf owner.
- Priority: P1

### Finding: `ImageDocumentRuntimeControllers` hides a peer-controller callback mesh

- Evidence: `src/document/imagedocumentruntimecontrollers.h:62-72` stores source store, deletion, page surface, presentation, open, navigation, predecode, spread, navigation controller, and plan executor in one composition object. `src/document/imagedocumentruntimecontrollers.cpp:50-124` wires animation errors, open callbacks, predecode, spread, and navigation through sibling-capturing lambdas. `src/document/imagedocumentruntimecontrollers.cpp:170-278` builds `ImageDocumentRuntimeOperations`, which dispatches into many sibling controllers.
- Current state: Controllers are named, but actual choreography is hidden in composition-root lambdas and a broad runtime operation table.
- Design concern: Constructor signatures do not reveal dependencies, and removing workflows such as predecode, spread, or source-load requires auditing the callback mesh.
- Correct end state: Cross-controller choreography should live in a named workflow executor/coordinator. Individual controllers should depend on narrow typed ports, and construction should expose dependencies without hidden `this` captures.
- Suggested migration: Define small port structs for open, spread, navigation, predecode, and page-surface interactions. Move runtime-operation execution into a workflow executor with fakeable ports.
- Acceptance criteria: Controllers no longer call siblings through hidden captures; construction order is not semantically important for callback validity; the workflow executor can be tested with fake ports.
- Priority: P1

### Finding: `MediaEntrySourceStore` depends upward on document load planning

- Evidence: `src/archive/mediaentrysourcestore.h:16` forward-declares `ImageDocumentSourceLoadRequest`; `src/archive/mediaentrysourcestore.h:29-30` exposes `prepareForSourceLoad(const ImageDocumentSourceLoadRequest &, ...)`; `src/archive/mediaentrysourcestore.cpp:7-8` includes `document/imagedocumentsourceloadrequest.h` and `document/imageloadplan.h`; `src/archive/mediaentrysourcestore.cpp:15-27` calls `openedCollectionScopeLoadPlan(...)`; `src/document/imagedocumentruntimecontrollers.cpp:262-267` invokes it.
- Current state: A source lifetime store under `src/archive/` understands image document source-load requests and image load planning semantics.
- Design concern: The archive/source lifetime abstraction is coupled upward to image document workflow shape, so source-load request changes can force edits in `src/archive/`.
- Correct end state: The document/open workflow should compute the opened-collection scope or store command. `MediaEntrySourceStore` should own only media-entry source lifetime/loading for an already resolved `OpenedCollectionScopeLocation`.
- Suggested migration: Add a document-layer adapter that converts `ImageDocumentSourceLoadRequest` plus current scope into a store command. Move `openedCollectionScopeLoadPlan(...)` use out of `src/archive/`.
- Acceptance criteria: `src/archive/mediaentrysourcestore.*` no longer includes `document/*`; source-store reuse tests do not need document request types.
- Priority: P2

### Finding: `ApplicationCommandRouterPorts` is a flat cross-domain command surface

- Evidence: `src/application/applicationcommandrouter.h:23-62` defines one `ApplicationCommandRouterPorts` struct containing shell/window, session, image document, viewport, panel, and video callbacks. `src/application/applicationcommandrouter.cpp:38-137` and `:180-264` switch across file, navigation, zoom, presentation, panels, fullscreen, help, menus, and video seek through the same port object. `src/facade/kiriviewapplication.cpp:499-631` binds application signals, `KiriDocumentSession`, `KiriImageDocument`, panel toggles, and `KiriVideoDocument` calls in one method.
- Current state: Dispatch policy is centralized, but the target boundary is not cohesive.
- Design concern: Changing or removing video seek, thumbnail panel, image zoom, direct navigation, or shell menu commands touches the same broad port struct and facade binding.
- Correct end state: Keep the command router, but split ports by owner: shell/window, session media/navigation, image presentation/viewport, panel, and video playback.
- Suggested migration: Introduce grouped port structs while keeping existing callback bodies. Gradually move shortcut/action handling paths to narrower port groups, then split `KiriViewApplication::commandRouterPorts()` into owner-specific builders.
- Acceptance criteria: Video seek shortcuts can be tested with only video/session ports; image pan/zoom shortcuts can be tested with only image ports.
- Priority: P2

## Logic Placement and Flow Predictability

### Finding: Viewport command planning lives in the QML facade

- Evidence: `src/facade/kiriimagedocument.h:247-345` exposes many viewport commands as `Q_INVOKABLE`s and owns `KiriView::ImageViewportInteraction m_viewportInteraction`. `src/facade/kiriimagedocument.cpp:567-705` computes zoom step, pan, scan, and nearest-anchor logic in the facade. `src/facade/kiriimagedocument.cpp:823-868` computes anchored zoom content position and sequences `m_runtime->requestManualZoomPercent()` with `requestViewportInteractionContentPosition()`. `src/facade/kiriimagedocument.cpp:872-890` mutates scan-start state while handling public signals. `src/qml/ImageViewport.qml:126-236` calls facade commands and explicitly calls `applyViewportProjection()` after many commands.
- Current state: QML events enter the facade; the facade samples presentation state, computes viewport content position, owns scan-start state, calls runtime mutation, and then QML applies the projection.
- Design concern: `KiriImageDocument` acts as a stateful presentation command planner instead of a thin facade. The hidden scan-state mutation in `handleDocumentChanges()` makes the flow especially surprising.
- Correct end state: Viewport command planning, anchored zoom positioning, pan/scan positioning, and displayed-image scan-start handoff should be owned by the presentation runtime/controller layer. The facade should only convert QML-friendly inputs and forward commands.
- Suggested migration: Add characterization tests for anchored zoom, pan/scan, and “next displayed image starts at final scan position.” Keep the QML API while moving calculations into runtime-level methods and moving `ImageViewportInteraction` ownership into presentation runtime/controller.
- Acceptance criteria: `KiriImageDocument` no longer owns `ImageViewportInteraction`; `handleDocumentChanges()` does not mutate viewport interaction state; QML does not need to understand scan-start handoff.
- Priority: P1

### Finding: Application action routing input assembly lives in the facade

- Evidence: `src/facade/kiriviewapplication.h:7-10` includes action state policy, command router, application types, and image action availability policy. `src/facade/kiriviewapplication.h:177-220` owns router input/port builders, `ApplicationCommandRouter`, `ImageActionAvailabilityProjection`, `ApplicationActionStateInput`, and UI gate fields. `src/facade/kiriviewapplication.cpp:331-360` rebuilds action state and subscribes to sources; `:395-497` assembles `ImageActionAvailabilityInput`, `ApplicationActionStateInput`, and `ApplicationCommandRouterInput`; `:499-631` builds concrete command router ports.
- Current state: `ApplicationActionRuntime` and `ApplicationCommandRouter` exist, but the QML-facing `KiriViewApplication` facade still owns state gathering, policy input assembly, command port construction, and dispatch.
- Design concern: Adding or changing an action affects the facade even when the behavior is application-domain routing rather than QML API shape.
- Correct end state: An application-layer coordinator/runtime should own action-state input assembly, source subscriptions, command-router input, and command ports. `KiriViewApplication` should expose QML APIs, attach the session, forward UI gate snapshots, and emit UI requests.
- Suggested migration: Create an application-shell coordinator or extend `ApplicationActionRuntime` to own session/UI-gate attachment. Move `imageActionAvailabilityInput()`, `actionStateInput()`, `commandRouterInput()`, and `commandRouterPorts()` into that application-layer object.
- Acceptance criteria: `src/facade/kiriviewapplication.h` no longer includes action policy/router headers; action-state rebuild tests target the application runtime/coordinator rather than the facade.
- Priority: P2

### Finding: Document session route plans mix mutation, publication, and follow-up effects

- Evidence: `src/session/documentsessionrouteplan.h:22-98` defines `DocumentSessionRouteOperation` with state mutation, leaf document routing, `RecomputePublicProjectionRouteOperation`, `RefreshDirectMediaNavigationAfterRoutingRouteOperation`, and `ClearMediaPredecodeRouteOperation` in one variant list. `src/session/documentsessionruntime.cpp:862-990` applies state mutation, leaf mutation, snapshot refresh, projection recomputation, direct navigation refresh, and predecode clearing in one loop. `src/session/documentsessionruntime.cpp:874-877` relies on `m_routingSource` suppression. `src/session/documentsessionruntime.cpp:1091-1125` may recompute projection, schedule predecode, and call `openMediaUrl()` from direct media navigation completion.
- Current state: A route plan is linear, but execution phases are not separated.
- Design concern: Observable publication and external follow-up effects interleave with internal mutation, so correctness depends on operation order and an implicit `m_routingSource` flag.
- Correct end state: Session routing should remain owned by `DocumentSessionRuntime`, but execution should be phase-oriented: apply state/leaf mutations, publish the accepted public projection once, then execute typed follow-up effects such as direct-navigation refresh, predecode scheduling, or follow-up media routing.
- Suggested migration: Add characterization tests for empty, direct image, direct video, image-document, and deletion-fallback routes. Introduce an internal `RouteExecutionResult` or executor that separates mutation results from follow-up effects.
- Acceptance criteria: Route plan types distinguish mutation operations from follow-up effects; the mutation visitor does not call `recomputePublicProjection()` or start direct media refresh.
- Priority: P2

### Finding: `async/imageiojobs` mixes generic async infrastructure with domain loaders

- Evidence: `src/async/imageiojobs.h:7-14` imports directory listing, callbacks, worker scheduler, decode request, image location, and navigation candidate types. `src/async/imageiojobs.cpp:86-190` implements directory candidates, opened-collection candidates, opened-collection reads, and `KIO::storedGet` image data loading. `src/navigation/imagedocumentpagecandidateprovider.cpp` and `src/decoding/imagedecodedependencies.cpp` import this header for default domain providers.
- Current state: `src/async/` appears to own generic job/callback/scheduler infrastructure, but `imageiojobs` also owns navigation/archive/decoding/KIO domain defaults.
- Design concern: Default domain IO behavior is hard to locate, and archive/navigation/decoding features are harder to remove independently.
- Correct end state: `src/async/` should own cancelable job wrappers, callback delivery, schedulers, and generic worker primitives. Directory candidate jobs should live in navigation, opened-collection jobs in archive, and stored image data loading in decoding or a clearly named IO infrastructure module.
- Suggested migration: Move one function group at a time and keep compatibility wrappers temporarily if needed.
- Acceptance criteria: `src/async/imageiojobs.*` is removed or contains only generic IO primitives; removing opened-collection support does not require editing a generic async module.
- Priority: P2

### Finding: Uncertain - `navigation/mediaformatregistry` owns both navigation predicates and localized open-dialog text

- Evidence: `src/navigation/mediaformatregistry.cpp:6-12` imports archive format, Rust conversion, decoding image registry, Rust media policy, direct navigation model, and `KLocalizedString`. `src/navigation/mediaformatregistry.h:15-23` exposes support predicates and `ordinaryMediaOpenDialogNameFilters()`. `src/decoding/imageformatregistry.cpp:37-46` also builds image/comic-book open-dialog filters.
- Current state: A navigation module owns media support predicates used by routing/predecode/session code and also builds localized file-dialog filter strings.
- Design concern: Navigation may legitimately own media target decisions, but localized UI text should probably live outside navigation.
- Correct end state: A neutral media-format capability wrapper should expose predicates and extension lists. Open-dialog filter text should be generated by facade/application/localization code. Navigation should not depend on `KLocalizedString`.
- Suggested migration: Move `ordinaryMediaOpenDialogNameFilters()` to an application/facade formatting helper first. Decide the long-term location of generic media predicates after confirming mixed-media session ownership intent.
- Acceptance criteria: `src/navigation/mediaformatregistry.cpp` no longer includes or uses `KLocalizedString`; image-only and mixed-media open-dialog builders share extension-list inputs.
- Priority: P3, uncertain

## Testability Problems

### Finding: Live directory candidate watching bypasses the provider seam

- Evidence: `src/navigation/imagedocumentpagecandidatedirectoryentry.cpp:18` constructs `KCoreDirLister` directly; `:76` uses `QTimer::singleShot(0, ...)`; `:200` connects `KCoreDirLister` signals directly. `src/navigation/imagedocumentpagecandidatestore.cpp:60` and `:93` watch through directory entries. In contrast, `tests/cpp/test_directorylistingjob.cpp:48` and `:75` use fake `DirectoryItemListProvider` for one-shot listing. `tests/cpp/test_imagedocumentpagecandidatedirectoryentry.cpp:53` and `:82` use `QTemporaryDir`, real files, KDirNotify DBus emission, and `QTRY_VERIFY_WITH_TIMEOUT(..., 10000)`.
- Current state: One-shot directory listing has an injectable provider seam, but live local-directory candidate watching directly owns KDE, filesystem, and timer effects.
- Design concern: Candidate-store state transitions, stale results, cancellation, and failure paths are hard to test deterministically without KDE directory notification timing and filesystem setup.
- Correct end state: A directory candidate/watch provider should own external directory effects. `ImageDocumentPageCandidateDirectoryEntry` should consume events such as initial snapshot, add/remove/change, deleted directory, error, and cancellation.
- Suggested migration: Extract a watch-oriented provider interface or extend `DirectoryItemListProvider` with live events. Move `KCoreDirLister` creation into the production adapter and add fake-provider tests.
- Acceptance criteria: Candidate-store and directory-entry transitions can be tested without `QTemporaryDir`, KDirNotify DBus, or `QTRY_VERIFY_WITH_TIMEOUT`; `KCoreDirLister` appears only in the production adapter.
- Priority: P1

### Finding: Predecode coordinators hide the timer seam

- Evidence: `src/predecode/predecodescheduleruntime.cpp:25` accepts `TimerScheduler`, and `tests/cpp/test_predecodescheduleruntime.cpp:84` uses `ManualTimerScheduler`. However, `src/predecode/imagepredecodecoordinator.cpp:16` and `src/predecode/mediapredecodecoordinator.cpp:16` construct `m_scheduleRuntime` without taking a scheduler dependency. `src/predecode/imagepredecodecoordinator.cpp:50` calls `QThread::idealThreadCount()` directly. `tests/cpp/test_imagepredecodecoordinator.cpp`, `tests/cpp/test_mediapredecodecoordinator.cpp`, and `tests/cpp/test_imagedocumentpredecodecontroller.cpp` rely on `QTest::qWait(...)`.
- Current state: The lower-level scheduler runtime is manually testable, but higher-level coordinators/controllers use wall-clock waits.
- Design concern: Debounce, momentum, stale-work suppression, and power-saver transitions are scheduling policy, but tests depend on elapsed time and host thread count.
- Correct end state: Predecode dependency structs should carry `TimerScheduler` and, where needed, a system profile/thread-count provider. Production should provide Qt timers and real facts; tests should provide manual timers and fixed facts.
- Suggested migration: Add optional timer scheduler and thread-count/system facts to image/media predecode dependency overrides. Convert qWait-based tests to manual timer advancement.
- Acceptance criteria: Predecode coordinator/controller tests do not need `QTest::qWait` for debounce or stale behavior; parallelism tests can set thread count deterministically.
- Priority: P1

### Finding: Image load planning performs filesystem directory probes

- Evidence: `src/document/imageloadplan.cpp:51` calls `openedCollectionScopeLocationForDirectlyOpenedLocalUrl(request.sourceUrl())`; `src/location/imagedocumentlocation.cpp:86` uses `QFileInfo(localPath).isDir()` to identify directories. `tests/cpp/test_imageloadplan.cpp:68` and `tests/cpp/test_imageloader.cpp:388` use `QTemporaryDir` for directory planning tests.
- Current state: The planner that chooses direct image load versus collection-scope directory load performs real filesystem existence/type checks.
- Design concern: Core routing logic cannot be tested with synthetic URLs only, and planning tests become coupled to filesystem state.
- Correct end state: Filesystem source resolution should live outside `ImageLoadPlan`. The planner should consume resolved facts such as opened collection scope or source classification.
- Suggested migration: Introduce an `OpenedCollectionScopeResolver` or source-classification value and move `QFileInfo::isDir()` into the production resolver.
- Acceptance criteria: `imageLoadPlan(...)` can be tested without creating directories; filesystem adapter tests are small and separate.
- Priority: P2

### Finding: Navigation-source identity mixes URL policy with process and OS probes

- Evidence: `src/location/imageurl.cpp:23` reads `qgetenv("XDG_RUNTIME_DIR")`; `:47` reads document-portal host paths with `getxattr(...)`; `:176` combines xattr lookup, KIOFuse runtime-dir parsing, and fallback in `navigationSourceUrl(...)`. `src/location/sourcekey.cpp:61` and `:66` build source keys from `navigationSourceUrl(url)`. `tests/cpp/test_imageurl.cpp:151` uses `setxattr`, `:182` uses `qputenv`/`qunsetenv`, and `tests/cpp/test_sourcekey.cpp:111` depends on symlink availability.
- Current state: Navigation URL normalization and source-key construction depend on host filesystem capabilities and process-wide environment mutation.
- Design concern: Source identity is correctness-sensitive, but important branches can be skipped or made flaky by OS feature availability and global environment changes.
- Correct end state: Platform fact collection should be separated from pure URL identity derivation. A production `NavigationSourceResolver` can read xattrs and environment, while pure helpers accept facts such as optional portal host path and runtime dir.
- Suggested migration: Extract a helper such as `navigationSourceUrlForFacts(url, portalHostPath, runtimeDir)` and make `navigationSourceUrl(...)` a production wrapper.
- Acceptance criteria: KIOFuse and document-portal URL cases can be tested without environment mutation or xattrs; OS adapter tests remain guarded and small.
- Priority: P2

### Finding: Default cache budgets are resolved from host memory outside dependency injection

- Evidence: `src/system/systemmemory.cpp:17` reads physical memory through `sysconf`; `src/document/imagedocumentruntimedependencies.cpp:32`, `src/predecode/mediapredecodedependencies.cpp:14`, `src/rendering/displayimagestore.cpp:35` and `:429`, and `src/session/thumbnailimagestore.cpp:38` call production `systemMemorySnapshot()` directly. `tests/cpp/test_systemmemory.cpp:21` tests lower-level injected readers, but owner-level budget tests assert only broad host-dependent properties.
- Current state: The low-level memory seam exists, but dependency resolution and default store construction capture host-derived values directly.
- Design concern: Low-memory, missing-memory, and invalid-memory budget behavior cannot be tested deterministically at the owner level.
- Correct end state: Runtime dependency resolution should accept `SystemMemorySnapshot` or `SystemMemoryRuntime`, and stores should receive explicit budgets from the owning dependency layer.
- Suggested migration: Add optional system-memory snapshot/runtime fields to the relevant dependency overrides and resolve image document, media predecode, display, and thumbnail budgets from supplied facts.
- Acceptance criteria: Dependency-resolution tests can set physical memory to known values and verify low/missing-memory behavior at owner level.
- Priority: P2

### Finding: Shortcut dispatch core is private inside the Qt event-filter runtime

- Evidence: `src/application/applicationshortcutruntime.h:59` keeps `handleShortcutEvent`, `handleFixedShortcutEvent`, `shortcutBindingEnabled`, and `handleShortcutActivated` private. `src/application/applicationshortcutruntime.cpp:272` installs event filters, and `:317`, `:343`, and `:422` handle focus gating, fixed shortcut mapping, unsupported-video interception, and QAction triggering. Pure adjacent tests exist in `tests/cpp/test_applicationshortcutpolicy.cpp` and `tests/cpp/test_applicationcommandrouter.cpp`, while many shortcut paths are exercised through `tests/cpp/test_imageshortcuts.cpp` using `QQuickView`, `QTest::keyClick`, focus, and window state.
- Current state: Shortcut filtering and adjacent policies are partly isolated, but full dispatch precedence is reachable only through Qt event filters and UI key events.
- Design concern: Reproducing precedence bugs requires app/window bootstrapping, and failures do not clearly point to policy, focus state, event filtering, or QAction invocation.
- Correct end state: A small dispatch policy/service should accept key sequence, focus applicability, action-state snapshot, registered bindings, and action-enabled facts, then return explicit outcomes such as ignore, trigger action, fixed navigation, or unsupported-video action. `ApplicationShortcutRuntime` should adapt Qt events to outcomes.
- Suggested migration: Add characterization coverage for untested runtime shortcut paths, then extract fixed shortcut mapping and binding dispatch into a helper.
- Acceptance criteria: Shortcut dispatch precedence can be tested without `QQuickView` or `QTest::keyClick`; focus/window integration remains covered by a small adapter test.
- Priority: P2

## Error Handling and Observability Problems

### Finding: Primary image load failures collapse user-facing errors and diagnostics into raw strings

- Evidence: `src/decoding/decodedimageresult.h` has `DecodedImageFailure` with only `QString errorString`; `src/document/imageloadtypes.h` defines `ImageLoadError` mostly around `Generic` and `EmptyOpenedCollection`; `src/document/imageloader.cpp` forwards decode/data failures as `Generic` and string; `src/document/imageopencontroller.cpp` maps to raw strings; `src/rendering/qimagereadertilesource.cpp` forwards `reader.errorString()`; `src/qml/ImageStateOverlay.qml` displays `imageDocument.errorString`.
- Current state: Primary image load failures do not preserve stage, source identity, backend, retryability, severity, or diagnostic detail as typed values.
- Design concern: UI copy and internal diagnostics are mixed in one string. Tests, logs, and retry policy cannot reliably distinguish failure kinds.
- Correct end state: The image/document loading boundary should use typed `ImageLoadFailure` with source URL/session id, stage/kind, user message, diagnostic detail, backend, severity, and retryability. Document state should expose only a user-message projection to QML.
- Suggested migration: Wrap existing decode/data/load strings into kinded typed failures first, while preserving current UI text.
- Acceptance criteria: Image load failure tests can assert stage/kind/retryability/diagnostic fields; QML does not depend on internal diagnostic strings.
- Priority: P1

### Finding: KIO file-operation failures lose error codes and duplicate cancellation policy

- Evidence: `src/system/filedeletion.h` and `.cpp` use result plus `QString` and local cancellation classification. `src/session/mediaopenwith.cpp` duplicates cancellation classification and string failures. `src/document/imagedocumentdeletioncontroller.cpp` and `src/session/documentsessionruntime.cpp` use generic fallback/user-message paths. `src/qml/Main.qml` displays toasts directly.
- Current state: File deletion and Open With KIO failures flow as raw strings, local cancellation decisions, and generic fallbacks.
- Design concern: Cancellation, retryability, and user/internal/fatal distinctions can drift by operation.
- Correct end state: A shared typed `KioOperationFailure` should contain operation kind, URL, raw KJob/KIO code, cancelled flag, user text, diagnostic detail, and retryability.
- Suggested migration: Normalize deletion and Open With provider results into `KioOperationFailure`; keep existing UI copy as a projection.
- Acceptance criteria: Cancellation policy is defined in one helper/type; deletion/Open With tests assert raw code and user message separately.
- Priority: P2

### Finding: Collection source errors are inconsistent across backends

- Evidence: `src/archive/mediaentrysourcebackend.h` defines string-oriented `MediaEntrySourceError`; directory backend behavior does not consistently preserve `QFile` error detail; KArchive and libarchive paths preserve different levels of detail; `src/async/imageiojobs.cpp` forwards opened-collection read/candidate-load errors as strings.
- Current state: Directory, KArchive, and libarchive source failures do not consistently preserve backend, operation, and source identity.
- Design concern: Opened-collection failures are hard to compare, reproduce, and log across backends.
- Correct end state: Typed `MediaEntrySourceFailure` should include backend, operation, collection URL, entry path, user text, and diagnostic detail.
- Suggested migration: Extend backend result types to typed failures first, then update `imageiojobs` forwarding to project typed failures.
- Acceptance criteria: Backend-specific failure tests assert operation, source identity, and diagnostic detail through common fields.
- Priority: P2

### Finding: QImageReader tile fallback can overwrite the original cause

- Evidence: `src/rendering/qimagereadertilesource.cpp` uses the same `QString*` error output through clip, scaled, and fallback attempts in `decodeTile()`. `src/decoding/staticimagedecode.cpp` returns only the final string, and `setTileSourceError` can overwrite previous causes.
- Current state: Multi-attempt tile decode failure does not distinguish primary failure from fallback diagnostics.
- Design concern: The first meaningful reader failure can be lost behind a later fallback message.
- Correct end state: Tile decode should return structured attempt results that preserve primary failure and fallback diagnostics separately.
- Suggested migration: Introduce a tile decode attempt list or primary/fallback failure struct. Keep user-message selection as a separate policy.
- Acceptance criteria: Fallback path tests verify both primary reader failure and fallback failure detail are preserved.
- Priority: P2

### Finding: Thumbnail failure diagnostics are produced but dropped by runtime observability

- Evidence: `src/thumbnail/thumbnailcachelookup.h` and `src/thumbnail/thumbnailgeneration.h`/`.cpp` carry error strings, but `src/session/activenavigationthumbnailruntime.cpp` primarily handles status/source and does not preserve errorString in structured logs or diagnostics.
- Current state: Cache lookup/generation failures are not user-visible, but they also do not leave enough internal diagnostics.
- Design concern: When thumbnails are blank or stale, production debugging lacks source, bucket, job, backend, and error detail.
- Correct end state: The public thumbnail model can remain unchanged, but runtime diagnostics/logging should include job id, source key, demand bucket, failure kind, and error detail.
- Suggested migration: Add typed diagnostic events or structured logs in active thumbnail runtime completion handling for failed/invalid lookup and generation.
- Acceptance criteria: Thumbnail failure tests or log probes verify preservation of source, bucket, kind, and error detail.
- Priority: P2

### Finding: Uncertain - async no-op paths can hide unexpected failures

- Evidence: `src/decoding/imagedecodejob.cpp` has empty/dependency-missing/no-delivery paths; `src/document/imageloader.cpp` quietly ignores stale no-current callbacks; `src/async/directorylistingjob.cpp` has an empty-string path for `openUrl` failure; `src/qml/ImageToolBar.qml` catches an exception without logging.
- Current state: Expected stale/cancel paths and unexpected missing-dependency/failure paths can both look like quiet no-ops.
- Design concern: Some no-ops are correct cancellation behavior, but unexpected no-ops can disappear without a debug signal.
- Correct end state: No-op reasons should be classified into expected stale/cancelled and unexpected invalid dependency/backend failure. Unexpected paths should produce debug/warn diagnostics.
- Suggested migration: Add a reason enum/log hook for callback suppression and stale completion. Investigate production reachability before changing user-visible behavior.
- Acceptance criteria: Expected stale completions remain quiet; missing dependency or `openUrl` failure paths leave structured diagnostics.
- Priority: P3, uncertain

## Deletion, Modularity, and Abstraction Problems

### Finding: Feature removal cost is concentrated in `DocumentSessionRuntime` and wide ports

- Evidence: Same evidence as the `DocumentSessionRuntime` convergence and session leaf port findings above in `src/session/documentsessionruntime.*` and `src/session/documentsessiondocumentports.h`.
- Current state: Thumbnail strip, Open With, deletion, video-output, predecode, and direct-media routing are tied to the same session runtime and broad leaf port.
- Design concern: Removing a feature becomes a session orchestration/projection/leaf-sampling change rather than a feature-owner change.
- Correct end state: Feature lifecycle should move into removable subowners, while `DocumentSessionState` remains the public projection owner.
- Suggested migration: Extract small effect-boundary features first, such as Open With or video-output, then proceed to thumbnails, predecode, and direct navigation.
- Acceptance criteria: Removing a feature does not require unrelated route/projection logic changes; feature subowner tests run without the full session runtime.
- Priority: P1

### Finding: Thumbnail demand tracker appears to be a parallel abstraction outside the production runtime path

- Evidence: `src/session/activenavigationthumbnaildemand.h` and `.cpp` define `ActiveNavigationThumbnailDemandTracker` and `record(...)`. `tests/cpp/test_activenavigationthumbnaildemand.cpp` tests the tracker directly. `rg` shows the production demand flow from `src/qml/ThumbnailPanel.qml:329` to `KiriDocumentSession::reportActiveNavigationThumbnailDemand`, `src/facade/kiridocumentsession.cpp:617`, and `src/session/documentsessionruntime.cpp:368`, while tracker usage appears limited to tests and definitions.
- Current state: A separate demand tracker abstraction exists, but the active runtime's production demand path does not appear to use it.
- Design concern: A test-only or abandoned abstraction makes it unclear where the real demand validation/bucketing rule lives.
- Correct end state: If the tracker is the intended abstraction, production demand flow should use it. Otherwise it should be removed or explicitly made a test helper.
- Suggested migration: Add a characterization comparison between `DocumentSessionRuntime::reportActiveNavigationThumbnailDemand` and `ActiveNavigationThumbnailDemandTracker` rules. If the rules match, wire the tracker into production; otherwise remove or rename it.
- Acceptance criteria: Active thumbnail demand validation/bucketing rules exist in one production-used type, and tests validate that type.
- Priority: P3

### Finding: `ImageDocumentRuntimePlan` should remain, but its operation vocabulary is still too broad

- Evidence: `src/document/imagedocumentruntimeplan.h:107-123` defines `ImageDocumentRuntimeOperation` variants covering lifecycle, media entry source, predecode, spread, navigation, open, and source-load operations. `src/document/imagedocumentruntimeplanexecutor.h` groups runtime operations by family and `src/document/imagedocumentruntimeplanexecutor.cpp` owns explicit dispatch, but `src/document/imagedocumentruntimecontrollers.cpp:170-278` still builds one broad operation table that reaches across sibling controllers.
- Current state: The runtime plan is useful as a C++ side-effect boundary, and dispatch has moved out of the composition root into `ImageDocumentRuntimePlanExecutor`. The remaining issue is that the plan vocabulary and bound operation table still span most image-document workflows at once.
- Design concern: Removing the plan concept would conflict with architecture docs; the problem is broad vocabulary and cross-feature operation ownership.
- Correct end state: Keep the plan as the canonical side-effect boundary, but split operation families and executor ownership by phase/feature.
- Suggested migration: Gradually split source-load, open, navigation, and predecode operation families into named executors, and shrink the broad variant list through compatibility wrappers.
- Acceptance criteria: Plan consumers are split by feature family; feature-specific executors can be tested without binding unrelated lifecycle, predecode, spread, navigation, open, and source-load callbacks.
- Priority: P2

## Recommended Correct End-State Architecture

- Ownership boundaries: `DocumentSessionState` owns public mixed-media projection and snapshot publication. `DocumentSessionRuntime` orchestrates named subowners for route, direct-media navigation, deletion, Open With, video-output, thumbnails, and predecode through typed results/effects.
- Domain rules: Image format capability and the image-open state machine live in one Rust policy or clearly named C++ domain-policy boundary. UI and downstream executors consume validated plans.
- State definition: Image document state changes pass through named transitions or a validating final-state boundary.
- Validation: External side-effect commands validate eligibility at the command owner, not only at UI/projection availability. Open With and source-load planning should pass through typed plans before providers run.
- External effects: `KCoreDirLister`, `QFileInfo`, xattrs, environment variables, `sysconf`, Qt timers, thread count, KIO jobs, and display-store budget facts are isolated behind providers, resolvers, or dependency adapters. Core policy consumes resolved facts and explicit dependencies.
- Error representation: Image, KIO operation, media-entry source, and thumbnail generation failures use typed failures. Internal paths preserve source identity, stage/kind, backend/raw code, severity, and retryability. QML receives user-facing projections.
- Facade/QML: `KiriImageDocument` and `KiriViewApplication` expose QML-friendly types, invokables, and signals. Viewport command planning and action routing input assembly move into presentation/application runtime. QML continues to report geometry/input facts and render projections.
- Tests: Characterization tests lock current behavior first. Rust policy and C++ domain helpers are tested with pure/fake dependencies. Qt/KDE/filesystem adapter tests remain small. Architecture boundary tests should verify abstractions used by production code.

## Suggested Refactoring Sequence

1. Add characterization tests around current behavior: route projection/follow-up ordering, viewport anchored zoom/scan-start behavior, and current image/video failure messages.
2. Centralize duplicated rules/state: add image format capability alignment tests, centralize zoom preset descriptors, and centralize `ImageShortcutScope` validity.
3. Isolate core domain logic from external effects: add a directory watch provider seam, thread timer scheduler/system facts into predecode coordinators, split filesystem source resolution from `ImageLoadPlan`, extract pure navigation-source URL helpers, and inject system memory facts for cache budget resolution.
4. Clarify ownership boundaries: split small `DocumentSessionRuntime` workflows first, introduce cohesive leaf session snapshots, move viewport command planning into presentation runtime, move application action input/port assembly into application runtime/coordinator, and move `MediaEntrySourceStore` document planning out of `src/archive/`.
5. Improve error semantics and observability: introduce typed image failures, then KIO and media-entry source failures, then tile decode attempt diagnostics and thumbnail failure diagnostics. Preserve UI text while internal diagnostics become structured.
6. Remove or simplify premature/parallel abstractions: either wire `ActiveNavigationThumbnailDemandTracker` into production or remove/test-helper it, phase `ImageDocumentRuntimeOperation` vocabulary by workflow family, and remove compatibility wrappers after tests prove behavior preservation.

## Things Not To Change Yet

- Do not move `src/qml/NavigationPresentationOrder.qml` out of QML just because it contains logic. Existing architecture tests intentionally expect QML to own RTL-aware presentation order for visual projection.
- Do not collapse all Rust/C++ mirror enums or bridge conversions. Some duplication at the FFI boundary is intentional; the target is duplicated domain policy, not exhaustive conversion code.
- Do not rewrite `DocumentSessionRuntime` or `ImageDocumentRuntimeControllers` wholesale. Extract one workflow at a time behind characterization tests.
- Do not remove `ImageDocumentRuntimePlan` as a concept. It matches the documented C++ side-effect boundary; the problem is broad operation vocabulary and hidden dispatch ownership.
- Do not introduce a generic logging/observability framework before typed failure values exist. Without typed failures, logging would mostly preserve the current string ambiguity.
- Do not alter user-visible error text as part of the first migration unless current text is clearly a bug. Internal typed diagnostics can be added while preserving existing UI messages.
