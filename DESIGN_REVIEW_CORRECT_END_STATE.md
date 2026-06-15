# Design Review: Correct End State

## Executive Summary

KiriView already has a sound architectural direction: `QML -> facade -> C++ runtime/effect executor -> Rust policy`. The main issue is not the absence of layers. The issue is that several boundaries still allow the same domain rule or state concept to be defined more than once, allow external effects to rely on UI/projection availability checks instead of command-boundary validation, and place too much workflow assembly inside facade or runtime composition objects.

The highest-risk findings are around rules that affect real user behavior and state consistency: image-open state transitions and failure representation. The next major risk is that `DocumentSessionRuntime`, `KiriImageDocument`, `KiriViewApplication`, and `ImageDocumentRuntimeControllers` still act as broad workflow owners even though the codebase already has useful lower-level objects.

The correct end state should be precise and conservative, not clever. Rust policy should own duplicated-free domain decisions and typed plans. C++ runtime should own Qt/KDE objects and external effects behind typed adapters. Facades should expose QML-friendly types and forward commands. QML should report geometry/input facts and render projections. Errors should flow internally as typed failures with source, stage, diagnostic detail, severity, and retryability, while the UI receives only user-facing messages.

## Top Design Risks

1. P1: `DocumentSessionRuntime`, session leaf ports, `ImageDocumentRuntimeControllers`, and `KiriImageDocument` concentrate too many feature workflows and make control flow hard to remove or reason about.
2. P1: Lower-level image, collection-source, and thumbnail failures are represented as raw strings or discarded metadata, which weakens diagnostics, retry semantics, and user/internal error separation.
3. P2: Image open state transitions are not enforced by one central state-machine boundary.

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

- Evidence: `src/session/documentsessionruntime.h:42-47` groups direct-media navigation, file deletion, thumbnail runtime, and predecode dependencies. `src/session/documentsessionruntime.h:118-177` declares methods for routing, leaf snapshot refresh, thumbnails, video-output claims, direct-media navigation, deletion, predecode, and public projection. `src/session/documentsessionruntime.cpp:389-421`, `:578-621`, `:862-990`, `:1054-1159`, and `:1162-1194` handle video-output claims, deletion, route execution, direct-media navigation, and predecode.
- Current state: `DocumentSessionRuntime` is the public mixed-media session owner and directly executes routing, active navigation, thumbnail scheduling, video output attachment, deletion, predecode, and projection publication.
- Design concern: Independent feature changes or removals converge on the same runtime and require revalidating routing/projection side effects.
- Correct end state: `DocumentSessionState` should remain the public projection owner. `DocumentSessionRuntime` should orchestrate named subowners through typed inputs/results: route executor, direct-media navigation coordinator, displayed-media deletion coordinator, video-output claim runtime, active-navigation thumbnail runtime, and direct-media predecode adapter.
- Suggested migration: Continue extracting small workflows such as video-output claims or displayed-media deletion. Keep public projection updates centralized in `DocumentSessionState` and move lifecycle execution/cancellation into subowners.
- Acceptance criteria: Feature-specific coordinators can be tested without full session runtime construction; removing thumbnail strip or displayed-media deletion does not require edits to direct-media routing logic.
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
- Current state: QML events enter the facade; the facade samples presentation state, computes viewport content position, owns scan-start state, calls runtime mutation, and then QML applies the projection. Focused facade-boundary characterization now covers centered anchored zoom, pan/final scan commands, and next-displayed-image final scan start handoff.
- Design concern: `KiriImageDocument` acts as a stateful presentation command planner instead of a thin facade. The hidden scan-state mutation in `handleDocumentChanges()` makes the flow especially surprising.
- Correct end state: Viewport command planning, anchored zoom positioning, pan/scan positioning, and displayed-image scan-start handoff should be owned by the presentation runtime/controller layer. The facade should only convert QML-friendly inputs and forward commands.
- Suggested migration: Keep the QML API while moving calculations into runtime-level methods and moving `ImageViewportInteraction` ownership into presentation runtime/controller.
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
- Current state: A route plan is linear, and focused coverage now characterizes mutation, publication, and follow-up operation ordering across empty, direct image, direct video, image-document, and deletion-fallback route plans. Execution phases are still not separated.
- Design concern: Observable publication and external follow-up effects interleave with internal mutation, so correctness depends on operation order and an implicit `m_routingSource` flag.
- Correct end state: Session routing should remain owned by `DocumentSessionRuntime`, but execution should be phase-oriented: apply state/leaf mutations, publish the accepted public projection once, then execute typed follow-up effects such as direct-navigation refresh, predecode scheduling, or follow-up media routing.
- Suggested migration: Introduce an internal `RouteExecutionResult` or executor that separates mutation results from follow-up effects.
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

### Finding: Lower-level image decode failures still lose backend-specific diagnostics before document wrapping

- Evidence: `src/decoding/decodedimageresult.h` has `DecodedImageFailure` with only `QString errorString`; `src/decoding/staticimagedecode.cpp` and decoder adapters still return final strings before `ImageLoader` wraps them into document-level `ImageLoadFailure`; `src/rendering/qimagereadertilesource.cpp` still exposes a compatibility string path even though tile decode now preserves ordered attempt diagnostics internally.
- Current state: The image document boundary stores typed `ImageLoadFailure` for data-load, decode, opened-collection candidate-load, empty opened-collection, and presentation failures while preserving QML-facing user messages. `QImageReaderTileSource::decodeTileWithDiagnostics()` preserves ordered reader-clip, source-clip, scaled-level, and full-image fallback failures, but lower decoder, first-display, blocking-preview, backend, route, severity, and retryability detail still collapse before document wrapping.
- Design concern: Document-level tests can distinguish failure kind and source identity, but deeper production diagnostics still cannot reliably distinguish decoder route, backend, operation, raw backend code/detail, severity, or retryability.
- Correct end state: Decode failure results and remaining tile/refinement failure paths should preserve backend/route, operation, diagnostics, severity, and retryability before projecting a document-level user message.
- Suggested migration: Extend `DecodedImageFailure` and static decode results to typed failure payloads, then map those into `ImageLoadFailure` without changing current QML error text.
- Acceptance criteria: Decoder and refinement tests can assert route/backend/operation diagnostics separately from the document-level user message.
- Priority: P2

### Finding: Collection source errors are inconsistent across backends

- Evidence: `src/archive/mediaentrysourcebackend.h` defines string-oriented `MediaEntrySourceError`; directory backend behavior does not consistently preserve `QFile` error detail; KArchive and libarchive paths preserve different levels of detail; `src/async/imageiojobs.cpp` forwards opened-collection read/candidate-load errors as strings.
- Current state: Directory, KArchive, and libarchive source failures do not consistently preserve backend, operation, and source identity.
- Design concern: Opened-collection failures are hard to compare, reproduce, and log across backends.
- Correct end state: Typed `MediaEntrySourceFailure` should include backend, operation, collection URL, entry path, user text, and diagnostic detail.
- Suggested migration: Extend backend result types to typed failures first, then update `imageiojobs` forwarding to project typed failures.
- Acceptance criteria: Backend-specific failure tests assert operation, source identity, and diagnostic detail through common fields.
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
- Current state: Thumbnail strip, deletion, video-output, predecode, and direct-media routing are tied to the same session runtime and broad leaf port.
- Design concern: Removing a feature becomes a session orchestration/projection/leaf-sampling change rather than a feature-owner change.
- Correct end state: Feature lifecycle should move into removable subowners, while `DocumentSessionState` remains the public projection owner.
- Suggested migration: Continue extracting small effect-boundary features such as video-output or deletion, then proceed to thumbnails, predecode, and direct navigation.
- Acceptance criteria: Removing a feature does not require unrelated route/projection logic changes; feature subowner tests run without the full session runtime.
- Priority: P1

### Finding: `ImageDocumentRuntimePlan` should remain, but its operation vocabulary is still too broad

- Evidence: `src/document/imagedocumentruntimeplan.h:107-123` defines `ImageDocumentRuntimeOperation` variants covering lifecycle, media entry source, predecode, spread, navigation, open, and source-load operations. `src/document/imagedocumentruntimeplanexecutor.h` groups runtime operations by family and `src/document/imagedocumentruntimeplanexecutor.cpp` owns explicit dispatch, but `src/document/imagedocumentruntimecontrollers.cpp:170-278` still builds one broad operation table that reaches across sibling controllers.
- Current state: The runtime plan is useful as a C++ side-effect boundary, and dispatch has moved out of the composition root into `ImageDocumentRuntimePlanExecutor`. The remaining issue is that the plan vocabulary and bound operation table still span most image-document workflows at once.
- Design concern: Removing the plan concept would conflict with architecture docs; the problem is broad vocabulary and cross-feature operation ownership.
- Correct end state: Keep the plan as the canonical side-effect boundary, but split operation families and executor ownership by phase/feature.
- Suggested migration: Gradually split source-load, open, navigation, and predecode operation families into named executors, and shrink the broad variant list through compatibility wrappers.
- Acceptance criteria: Plan consumers are split by feature family; feature-specific executors can be tested without binding unrelated lifecycle, predecode, spread, navigation, open, and source-load callbacks.
- Priority: P2

## Recommended Correct End-State Architecture

- Ownership boundaries: `DocumentSessionState` owns public mixed-media projection and snapshot publication. `DocumentSessionRuntime` orchestrates named subowners for route, direct-media navigation, deletion, video-output, thumbnails, and predecode through typed results/effects.
- Domain rules: The image-open state machine lives in one Rust policy or clearly named C++ domain-policy boundary. UI and downstream executors consume validated plans.
- State definition: Image document state changes pass through named transitions or a validating final-state boundary.
- Validation: External side-effect commands validate eligibility at the command owner, not only at UI/projection availability. Source-load planning should pass through typed plans before providers run.
- External effects: `QFileInfo`, `sysconf`, KIO jobs, and display-store budget facts are isolated behind providers, resolvers, or dependency adapters. Core policy consumes resolved facts and explicit dependencies.
- Error representation: Image decoder, remaining tile/refinement, media-entry source, and thumbnail generation failures use typed failures. Internal paths preserve source identity, stage/kind, backend/raw code, severity, and retryability. QML receives user-facing projections.
- Facade/QML: `KiriImageDocument` and `KiriViewApplication` expose QML-friendly types, invokables, and signals. Viewport command planning and action routing input assembly move into presentation/application runtime. QML continues to report geometry/input facts and render projections.
- Tests: Characterization tests lock current behavior first. Rust policy and C++ domain helpers are tested with pure/fake dependencies. Qt/KDE/filesystem adapter tests remain small. Architecture boundary tests should verify abstractions used by production code.

## Suggested Refactoring Sequence

1. Add characterization tests around current image/video failure messages.
2. Isolate core domain logic from external effects: inject system memory facts for cache budget resolution.
3. Clarify ownership boundaries: split small `DocumentSessionRuntime` workflows first, introduce cohesive leaf session snapshots, move viewport command planning into presentation runtime, and move application action input/port assembly into application runtime/coordinator.
4. Improve error semantics and observability: extend lower-level image decoder and remaining refinement diagnostics, then media-entry source failures, then thumbnail failure diagnostics. Preserve UI text while internal diagnostics become structured.
5. Remove or simplify premature/parallel abstractions: phase `ImageDocumentRuntimeOperation` vocabulary by workflow family and remove compatibility wrappers after tests prove behavior preservation.

## Things Not To Change Yet

- Do not move `src/qml/NavigationPresentationOrder.qml` out of QML just because it contains logic. Existing architecture tests intentionally expect QML to own RTL-aware presentation order for visual projection.
- Do not collapse all Rust/C++ mirror enums or bridge conversions. Some duplication at the FFI boundary is intentional; the target is duplicated domain policy, not exhaustive conversion code.
- Do not rewrite `DocumentSessionRuntime` or `ImageDocumentRuntimeControllers` wholesale. Extract one workflow at a time behind characterization tests.
- Do not remove `ImageDocumentRuntimePlan` as a concept. It matches the documented C++ side-effect boundary; the problem is broad operation vocabulary and hidden dispatch ownership.
- Do not introduce a generic logging/observability framework before typed failure values exist. Without typed failures, logging would mostly preserve the current string ambiguity.
- Do not alter user-visible error text as part of the first migration unless current text is clearly a bug. Internal typed diagnostics can be added while preserving existing UI messages.
