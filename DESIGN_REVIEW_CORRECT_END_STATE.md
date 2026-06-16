# Design Review: Correct End State

## Executive Summary

KiriView already has a sound architectural direction: `QML -> facade -> C++ runtime/effect executor -> Rust policy`. The main issue is not the absence of layers. The issue is that several boundaries still allow the same domain rule or state concept to be defined more than once, allow external effects to rely on UI/projection availability checks instead of command-boundary validation, and place too much workflow assembly inside facade or runtime composition objects.

The highest-risk findings are around rules that affect real user behavior and state consistency: failure representation and broad workflow ownership. The next major risk is that `DocumentSessionRuntime`, `KiriImageDocument`, `KiriViewApplication`, and `ImageDocumentRuntimeControllers` still act as broad workflow owners even though the codebase already has useful lower-level objects.

The correct end state should be precise and conservative, not clever. Rust policy should own duplicated-free domain decisions and typed plans. C++ runtime should own Qt/KDE objects and external effects behind typed adapters. Facades should expose QML-friendly types and forward commands. QML should report geometry/input facts and render projections. Errors should flow internally as typed failures with source, stage, diagnostic detail, severity, and retryability, while the UI receives only user-facing messages.

## Top Design Risks

1. P1: `DocumentSessionRuntime`, `ImageDocumentRuntimeControllers`, and `KiriImageDocument` concentrate too many feature workflows and make control flow hard to remove or reason about.
2. P1: Lower-level image decoder and remaining refinement failures still lose backend-specific diagnostics before document wrapping, which weakens diagnostics, retry semantics, and user/internal error separation.

## Invariant and Correctness Risks

### Finding: Uncertain - image presentation can expose visible image state without a provider-ready display source

- Evidence: `src/presentation/imagepagesurfacecontroller.cpp:155` returns `hasImage` and `m_displaySource` independently. `src/presentation/imagepagesurfacecontroller.cpp:246` creates a display source even when `DisplayImageStore::insert` returns an empty entry id. `src/rendering/displayimagestore.cpp:237` returns empty for null images or images over budget. `src/presentation/imagepresentationruntime.cpp:724` marks the projection visible when `slot.hasImage` is true and copies the provider URL. `src/qml/DisplayImagePage.qml:28` shows the page when the projection is visible and uses `providerUrl` for the inner image.
- Current state: `ImagePageSurfaceController::setImage()` now preserves the invariant by publishing a display-error source slot with image dimensions when no provider entry exists, display-store insertion failure already projects as display-error, and `ImagePresentationRuntime` downgrades malformed provider-ready empty-URL slots to explicit display-error projections. The page-slot type still represents `hasImage`, provider readiness, and display-error as independent fields rather than explicit variants.
- Design concern: The visible provider-backed image invariant is not encoded in the type, so a blank page or unnamed display failure may be possible.
- Correct end state: `ImagePresentationPageSlotSnapshot` should use explicit variants such as empty, provider-ready, display-error, and retained/stale presentation. A visible provider-backed state should always include either a valid provider URL or an explicit display-error state consumed by UI/document failure handling.
- Suggested migration: Replace independent `ImagePresentationPageSlotSnapshot` fields with explicit empty, provider-ready, display-error, and retained/stale variants, then project QML-visible state from those variants.
- Acceptance criteria: No production `ImageDisplaySourceProjection` can be `visible == true` with an empty provider URL unless it carries an explicit display-error state.
- Priority: P2, uncertain

## Cohesion, Coupling, and Ownership Problems

### Finding: `DocumentSessionRuntime` is a mixed-media feature convergence point

- Evidence: `src/session/documentsessionruntime.h:42-47` groups direct-media navigation, file deletion, thumbnail runtime, and predecode dependencies. `src/session/documentsessionruntime.h:118-177` declares methods for routing, leaf snapshot refresh, thumbnails, video-output claims, direct-media navigation, deletion, predecode, and public projection. `src/session/documentsessionruntime.cpp:578-621`, `:862-990`, `:1054-1159`, and `:1162-1194` handle deletion, route execution, direct-media navigation, and predecode.
- Current state: `DocumentSessionRuntime` is the public mixed-media session owner and directly owns route selection/logging and projection publication. Image document source assignment, page navigation, and image-document deletion command effects now live in `DocumentSessionImageDocumentCommandRuntime`. Video document source assignment, stop, and source clearing on session mode exit now live in `DocumentSessionVideoDocumentCommandRuntime`. Video-output surface token issuance, owner freshness, detach authorization, clear-on-mode-exit state, and accepted video-output attach/detach effects now live in `DocumentSessionVideoOutputRuntime`. Direct-media deletion candidate lookup, KDE deletion invocation, cancellation, and stale-completion rejection now live in `DocumentSessionMediaDeletionRuntime`. Direct-media predecode schedule/cache eligibility, coordinator lifecycle, clear/cancel behavior, and cached predecoded-image lookup now live in `DocumentSessionMediaPredecodeRuntime`. Active-navigation thumbnail runtime construction, session-specific source adaptation, demand bucket mapping/reporting, and row publication now live in `DocumentSessionThumbnailRuntime`. Direct-media navigation async candidate loading, cancellation, and stale-completion rejection now live in `DocumentSessionDirectMediaNavigationRuntime`; refresh/open completion interpretation now lives in `DocumentSessionDirectMediaNavigationWorkflow`. Active-navigation dispatch operation execution and pending reveal context now live in `DocumentSessionActiveNavigationRuntime`. Route-operation dispatch and route-local execution facts now live in `DocumentSessionRouteRuntime`. Leaf read/change events now cross as cohesive image/video snapshot ports that are separate from leaf command/effect ports. The session runtime remains the adapter that gates commands by public projection/document kind, supplies committed predecode facts, publishes progress, applies deletion completion routes and direct-media navigation workflow applications, delegates thumbnail row projection from committed active-navigation facts, and supplies source/snapshot facts to active-navigation dispatch.
- Design concern: Independent feature changes or removals converge on the same runtime and require revalidating routing/projection side effects.
- Correct end state: `DocumentSessionState` should remain the public projection owner. `DocumentSessionRuntime` should orchestrate named subowners through typed inputs/results: route executor, active-navigation dispatch runtime, direct-media navigation coordinator, displayed-media deletion coordinator, video-output claim runtime, active-navigation thumbnail runtime, and direct-media predecode adapter.
- Suggested migration: Continue extracting small workflows and narrowing remaining leaf command/effect callbacks. Keep public projection updates centralized in `DocumentSessionState` and move lifecycle execution/cancellation into subowners.
- Acceptance criteria: Feature-specific coordinators can be tested without full session runtime construction; removing thumbnail strip or direct-media routing does not require edits to unrelated session projection logic.
- Priority: P1

### Finding: `ImageDocumentRuntimeControllers` hides a peer-controller callback mesh

- Evidence: `src/document/imagedocumentruntimecontrollers.h:59-69` stores source store, deletion, page surface, presentation, open, navigation, predecode, spread, navigation controller, and runtime workflow in one composition object. `src/document/imagedocumentruntimecontrollers.cpp:47-123` wires animation errors, open callbacks, predecode, spread, and navigation through sibling-capturing lambdas. `src/document/imagedocumentruntimecontrollers.cpp:124-136` wires `ImageDocumentRuntimeWorkflowPorts`, and `src/document/imagedocumentruntimeworkflow.cpp:23-253` builds and dispatches `ImageDocumentRuntimeOperations` across sibling controllers.
- Current state: `ImageDocumentRuntimeWorkflow` now owns `ImageDocumentRuntimePlan` dispatch and operation binding through fakeable workflow ports. Controllers still construct the sibling controllers, wire sibling-capturing callbacks for open/predecode/spread/navigation interactions, and build the workflow port table.
- Design concern: Constructor signatures do not reveal dependencies, and removing workflows such as predecode, spread, or source-load requires auditing the callback mesh.
- Correct end state: Cross-controller choreography should live in a named workflow executor/coordinator. Individual controllers should depend on narrow typed ports, and construction should expose dependencies without hidden `this` captures.
- Suggested migration: Continue replacing sibling-capturing controller callbacks with narrow typed ports for open, spread, navigation, predecode, and page-surface interactions.
- Acceptance criteria: Controllers no longer call siblings through hidden captures; construction order is not semantically important for callback validity; the workflow executor can be tested with fake ports.
- Priority: P1

## Logic Placement and Flow Predictability

### Finding: Application action routing input assembly lives in the facade

- Evidence: `src/facade/kiriviewapplication.h:7-10` includes action state policy, command router, application types, and image action availability policy. `src/facade/kiriviewapplication.h:177-220` owns router input/port builders, `ApplicationCommandRouter`, `ImageActionAvailabilityProjection`, `ApplicationActionStateInput`, and UI gate fields. `src/facade/kiriviewapplication.cpp:331-360` rebuilds action state and subscribes to sources; `:395-497` assembles `ImageActionAvailabilityInput`, `ApplicationActionStateInput`, and `ApplicationCommandRouterInput`; `:499-631` builds concrete command router ports.
- Current state: `ApplicationActionRuntime` and `ApplicationCommandRouter` exist, but the QML-facing `KiriViewApplication` facade still owns state gathering, policy input assembly, command port construction, and dispatch.
- Design concern: Adding or changing an action affects the facade even when the behavior is application-domain routing rather than QML API shape.
- Correct end state: An application-layer coordinator/runtime should own action-state input assembly, source subscriptions, command-router input, and command ports. `KiriViewApplication` should expose QML APIs, attach the session, forward UI gate snapshots, and emit UI requests.
- Suggested migration: Create an application-shell coordinator or extend `ApplicationActionRuntime` to own session/UI-gate attachment. Move `imageActionAvailabilityInput()`, `actionStateInput()`, `commandRouterInput()`, and `commandRouterPorts()` into that application-layer object.
- Acceptance criteria: `src/facade/kiriviewapplication.h` no longer includes action policy/router headers; action-state rebuild tests target the application runtime/coordinator rather than the facade.
- Priority: P2

## Testability Problems

### Finding: Shortcut dispatch core is private inside the Qt event-filter runtime

- Evidence: `src/application/applicationshortcutruntime.h:59` keeps `handleShortcutEvent`, `handleFixedShortcutEvent`, `shortcutBindingEnabled`, and `handleShortcutActivated` private. `src/application/applicationshortcutruntime.cpp:272` installs event filters, and `:317`, `:343`, and `:422` still handle focus gating, generic binding dispatch, unsupported-video interception, and QAction triggering. `src/application/applicationshortcutpolicy.cpp:216` now exposes `fixedShortcutDispatchOutcome()` for fixed video seek, horizontal arrow, single-page arrow, and vertical pan decisions, with direct coverage in `tests/cpp/test_applicationshortcutpolicy.cpp`; many generic shortcut paths are still exercised through `tests/cpp/test_imageshortcuts.cpp` using `QQuickView`, `QTest::keyClick`, focus, and window state.
- Current state: Fixed viewer shortcut mapping and availability are isolated in pure policy, but registered binding precedence, unsupported media interception, action-enabled facts, and focus/window gating remain reachable only through Qt event filters and UI key events.
- Design concern: Reproducing precedence bugs requires app/window bootstrapping, and failures do not clearly point to policy, focus state, event filtering, or QAction invocation.
- Correct end state: A small dispatch policy/service should accept key sequence, focus applicability, action-state snapshot, registered bindings, and action-enabled facts, then return explicit outcomes such as ignore, trigger action, fixed navigation, or unsupported-video action. `ApplicationShortcutRuntime` should adapt Qt events to outcomes.
- Suggested migration: Extract registered binding dispatch and shortcut activation outcome next, including unsupported media interception and action-enabled facts, then leave focus/window integration in a small runtime adapter test.
- Acceptance criteria: Generic shortcut binding precedence can be tested without `QQuickView` or `QTest::keyClick`; focus/window integration remains covered by a small adapter test.
- Priority: P2

## Error Handling and Observability Problems

### Finding: Lower-level image decode failures still lose backend-specific diagnostics before document wrapping

- Evidence: `src/decoding/decodedimageresult.h` now carries route, operation, diagnostic detail, severity, and retryability on `DecodedImageFailure`; `src/rendering/qimagereadertilesource.cpp` preserves ordered tile attempt diagnostics with severity and retryability, but still exposes a compatibility string path for older consumers.
- Current state: The image document boundary stores typed `ImageLoadFailure` for data-load, decode, opened-collection candidate-load, empty opened-collection, and presentation failures while preserving QML-facing user messages. `QImageReaderTileSource::decodeTileWithDiagnostics()` preserves ordered reader-clip, source-clip, scaled-level, and full-image fallback failures with severity and retryability, selected decoder route now stamps `DecodedImageFailure`, static-image decode failures preserve first-display/blocking-preview operation, diagnostic detail, severity, and retryability, the Qt raster adapter now stamps route/operation plus backend detail onto direct adapter failures, Qt raster animation-open adapter failures now preserve operation and adapter detail, raw decode failures now preserve raw route, operation, and LibRaw stage/code/detail, HEIF still/sequence failures now preserve HEIF route, operation, and backend diagnostic detail, and SVG/APNG adapter failures now preserve route, operation, and adapter detail. Remaining refinement and compatibility paths still collapse typed diagnostics into string-only error paths for some consumers.
- Design concern: Document-level tests can distinguish failure kind and source identity, but deeper production diagnostics still cannot reliably distinguish remaining refinement operation/detail, severity, or retryability across all paths.
- Correct end state: Decode failure results and remaining tile/refinement failure paths should preserve backend/route, operation, diagnostics, severity, and retryability before projecting a document-level user message.
- Suggested migration: Extend `DecodedImageFailure` and static decode results to typed failure payloads, then map those into `ImageLoadFailure` without changing current QML error text.
- Acceptance criteria: Decoder and refinement tests can assert route/backend/operation diagnostics separately from the document-level user message.
- Priority: P2

## Deletion, Modularity, and Abstraction Problems

### Finding: Feature removal cost is concentrated in `DocumentSessionRuntime` and wide ports

- Evidence: Same evidence as the `DocumentSessionRuntime` convergence finding above in `src/session/documentsessionruntime.*`, plus the remaining leaf command/effect callbacks in `src/session/documentsessiondocumentports.h`.
- Current state: Image document command effects, video document source/stop effects, video-output claim lifecycle and attachment effects, direct-media deletion lifecycle, direct-media navigation completion decisions, direct-media predecode coordinator lifecycle, active-navigation thumbnail runtime/source-adaptation lifecycle, active-navigation dispatch operation execution and pending reveal context, route-operation dispatch, and session-facing image/video leaf snapshot-change publication now have focused boundaries. Leaf snapshot ports are separate from leaf command/effect ports. Route selection/logging and session projection/routing remain tied to the same session runtime.
- Design concern: Removing a feature becomes a session orchestration/projection/leaf-event change rather than a feature-owner change.
- Correct end state: Feature lifecycle should move into removable subowners, while `DocumentSessionState` remains the public projection owner.
- Suggested migration: Continue extracting small effect-boundary features, proceeding next to remaining leaf command/effect callbacks or projection/routing narrowing.
- Acceptance criteria: Removing a feature does not require unrelated route/projection logic changes; feature subowner tests run without the full session runtime.
- Priority: P1

### Finding: `ImageDocumentRuntimePlan` should remain, but its operation vocabulary is still too broad

- Evidence: `src/document/imagedocumentruntimeplan.h:107-123` defines `ImageDocumentRuntimeOperation` variants covering lifecycle, media entry source, predecode, spread, navigation, open, and source-load operations. `src/document/imagedocumentruntimeplanexecutor.h` groups runtime operations by family and `src/document/imagedocumentruntimeplanexecutor.cpp` owns explicit dispatch. `src/document/imagedocumentruntimeworkflow.h:25-48` exposes the workflow ports and owns an executor, while `src/document/imagedocumentruntimeworkflow.cpp:23-232` still builds one broad operation table that reaches across sibling controllers.
- Current state: The runtime plan is useful as a C++ side-effect boundary, dispatch has moved out of the composition root into `ImageDocumentRuntimePlanExecutor`, and operation binding/dispatch ownership now has the named `ImageDocumentRuntimeWorkflow` owner. The remaining issue is that the plan vocabulary and bound operation table still span most image-document workflows at once.
- Design concern: Removing the plan concept would conflict with architecture docs; the problem is broad vocabulary and cross-feature operation ownership.
- Correct end state: Keep the plan as the canonical side-effect boundary, but split operation families and executor ownership by phase/feature.
- Suggested migration: Gradually split source-load, open, navigation, and predecode operation families into named executors, and shrink the broad variant list through compatibility wrappers.
- Acceptance criteria: Plan consumers are split by feature family; feature-specific executors can be tested without binding unrelated lifecycle, predecode, spread, navigation, open, and source-load callbacks.
- Priority: P2

## Recommended Correct End-State Architecture

- Ownership boundaries: `DocumentSessionState` owns public mixed-media projection and snapshot publication. `DocumentSessionRuntime` orchestrates named subowners for route, active-navigation dispatch, direct-media navigation, deletion, image commands, video commands, video-output, thumbnails, and predecode through typed results/effects, and reads leaf document facts through cohesive session snapshots.
- Domain rules: The image-open state machine lives in one Rust policy or clearly named C++ domain-policy boundary. UI and downstream executors consume validated plans.
- State definition: Image document state changes pass through named transitions or a validating final-state boundary.
- Validation: External side-effect commands validate eligibility at the command owner, not only at UI/projection availability. Source-load planning should pass through typed plans before providers run.
- External effects: `QFileInfo`, `sysconf`, KIO jobs, and display-store budget facts are isolated behind providers, resolvers, or dependency adapters. Core policy consumes resolved facts and explicit dependencies.
- Error representation: Image decoder, remaining tile/refinement, media-entry source, and thumbnail generation failures use typed failures. Internal paths preserve source identity, stage/kind, backend/raw code, severity, and retryability. QML receives user-facing projections.
- Facade/QML: `KiriImageDocument` and `KiriViewApplication` expose QML-friendly types, invokables, and signals. Viewport command planning and projection application are runtime/C++ bridge-owned; action routing input assembly still moves into application runtime. QML continues to report geometry/input facts and render projections.
- Tests: Characterization tests lock current behavior first. Rust policy and C++ domain helpers are tested with pure/fake dependencies. Qt/KDE/filesystem adapter tests remain small. Architecture boundary tests should verify abstractions used by production code.

## Suggested Refactoring Sequence

1. Clarify ownership boundaries: split small `DocumentSessionRuntime` workflows first, and move application action input/port assembly into application runtime/coordinator.
2. Improve error semantics and observability: extend lower-level image decoder and remaining refinement diagnostics. Preserve UI text while internal diagnostics become structured.
3. Remove or simplify premature/parallel abstractions: phase `ImageDocumentRuntimeOperation` vocabulary by workflow family and remove compatibility wrappers after tests prove behavior preservation.

## Things Not To Change Yet

- Do not move `src/qml/NavigationPresentationOrder.qml` out of QML just because it contains logic. Existing architecture tests intentionally expect QML to own RTL-aware presentation order for visual projection.
- Do not collapse all Rust/C++ mirror enums or bridge conversions. Some duplication at the FFI boundary is intentional; the target is duplicated domain policy, not exhaustive conversion code.
- Do not rewrite `DocumentSessionRuntime` or `ImageDocumentRuntimeControllers` wholesale. Extract one workflow at a time behind characterization tests.
- Do not remove `ImageDocumentRuntimePlan` as a concept. It matches the documented C++ side-effect boundary; the problem is broad operation vocabulary and hidden dispatch ownership.
- Do not introduce a generic logging/observability framework before typed failure values exist. Without typed failures, logging would mostly preserve the current string ambiguity.
- Do not alter user-visible error text as part of the first migration unless current text is clearly a bug. Internal typed diagnostics can be added while preserving existing UI messages.
