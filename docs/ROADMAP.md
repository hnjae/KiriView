# KiriView Architecture Roadmap

This roadmap moves KiriView from a mostly sound layered pipeline with several mirrored public states and QML-owned workflow decisions to a single-owner architecture where each durable cross-mode state has one named authority, Qt Quick keeps ownership of physical item state and UI-local transients, async completions are accepted only by their lifecycle owner, and new media, navigation, rendering, and UI extensions plug into explicit contracts instead of cross-cutting through facades.

**Diagnostic anchors**

- Resolved viewport bridge: canonical logical viewport state now flows through `src/presentation/imagepresentationruntime.cpp`, while QML retains only physical Qt Quick item state such as `Flickable.contentX/Y` and acknowledges revisioned presentation commands instead of owning visible rects or logical viewport frames.
- Resolved presentation ownership: `ImagePresentationRuntime` is the single mutable owner of image zoom, rotation, viewport frame, visible source rect, mode, reading direction, transition state, render projections, and tile-decode demand; page surface slots own decoded surfaces and resource lifetimes only.
- Multiple sources of truth: action availability is computed partly in Rust/C++ availability projection, partly in `src/qml/ImageActions.qml` and `src/qml/ImageShortcuts.qml`, and finally written into `QAction.enabled` and `QAction.checked` by `src/qml/ManagedAction.qml`, even though application actions are a C++ ownership boundary; active image readiness is likewise recomputed across `src/qml/Main.qml`, `src/qml/MediaViewportHost.qml`, and `src/qml/ImageViewport.qml`.
- Stale async updates or race conditions: core decode, predecode, thumbnail, direct-media navigation, video source, and tile decode flows already use operation ids, cursor generations, or scoped jobs, but viewport updates in `src/qml/ImageViewport.qml` still rely on delayed QML callbacks that can race user input, zoom changes, and frame publication.
- Unclear authoritative ownership: the top-level session routes media, but leaf documents still expose writable `sourceUrl` properties in `src/facade/kiriimagedocument.h` and `src/facade/kirivideodocument.h`; this leaves a public path around the session route owner.
- State-synchronization inconsistencies: the session stores active zoom and public navigation projections while leaf documents own source, status, zoom, page navigation, and media size; `src/session/documentsessionruntime.cpp` keeps those projections synchronized by signal fan-out rather than a single session projection transaction.
- Architectural ownership-boundary violations: QML performs shared command policy for actions, shortcuts, zoom anchoring, scan navigation, and direct action state binding in `src/qml/ImageActions.qml`, `src/qml/ImageShortcuts.qml`, `src/qml/ImageViewport.qml`, and `src/qml/ManagedAction.qml`; QML should compose and display these values, not own cross-mode behavior policy.
- Pipeline judgment: the high-level source routing -> document load -> decode -> presentation -> render-frame -> scene-graph pipeline is structurally sound, and the codebase already has strong foundations in plan objects, stale-completion rejection, and Rust policy boundaries; extension is resisted where public QML APIs and leaf documents still bypass those plan boundaries.

## Target Architecture

Top-level media session ownership is singular. The session owns the requested source identity, active document kind, direct-media cursor, direct-media scope generation, active-navigation projection, active zoom readout, displayed-media operation availability, window title subject, session-scoped errors, and the public batch of mixed-media signals. Image and video documents may expose snapshots to the session, but they do not own public cross-mode state and they cannot be directly selected as a top-level route by QML.

Image document ownership is scoped below the session. The image document owns image-document source loading, the current displayed image location, opened-collection scope, image load status, image load session identity, decoded image payloads, image-document page navigation, image-document container navigation, image metadata storage, and image-document-only deletion fallback. It accepts route commands from the session or internal page-navigation commands from its own controller, then publishes snapshots and completion events upward.

Video document ownership is scoped below the session. The video document owns video source resolution, backend source attachment, playback status, playback position and duration, seekability, media-ended state, audio/video availability, mute state, video output attachment, video zoom projection, and video metadata storage. QML may own the visual output item instance, but backend attachment and public video state belong to the video document runtime.

Presentation ownership is one active logical owner at a time. The active presentation owner holds zoom mode, zoom percent, rotation, logical viewport frame, visible source rect, scan-start handoff, spread mode, reading direction, primary/secondary page visibility, and render snapshots. Single-page and two-page modes may keep restoration snapshots, but inactive mode state is a cache, not a public owner. Mode transitions are explicit prepare, commit, or abort operations, and the public projection exposes whether it is previous-active, transitioning-placeholder, or committed-active so consumers do not infer state from partially updated properties.

The QML viewport owns Qt Quick item instances, physical applied item state, high-frequency gesture sampling, and UI-local transient state. QML reports viewport dimensions, user scroll deltas, pointer anchors, gesture commit events, actual applied positions, and demand visibility. C++ owns canonical viewport policy and publishes a revisioned viewport frame projection containing a command revision, requested content position, content size, image rect, visible rect, pannability, command outcome, and superseded or rejected status. Observation and acknowledgement revisions are distinct from command revisions, and observations identify their origin as command-origin, user-origin, inertia-origin, overshoot-origin, resize-origin, rotation-origin, device-pixel-ratio-origin, or system-origin. The viewport contract defines pending, applying, applied, acknowledged, settled, superseded, and rejected states, and fixes coordinate spaces for source, image, item, content, viewport, device-pixel-ratio, rotation anchors, zoom anchors, logical pixels, physical pixels, and fractional scroll values. QML may apply physical item movement immediately during gesture sampling, kinetic scrolling, bounds rebound, and animation settle, but only gesture commit, inertia settle, overshoot settle, resize commit, rotation commit, device-pixel-ratio commit, or explicit command acknowledgement can promote observed physical state into the canonical logical frame. QML applies only the latest non-superseded command revision, records an applied revision, and acknowledges actual item state when needed; delayed or stale UI callbacks cannot overwrite newer presentation state.

Action ownership stays in the application runtime. Application-global command identity is separate from window-local action instances: the command registry owns stable action names, standard action mapping, user shortcut override storage, retranslation policy, and command routing, while each window or main-controller action collection owns its own QAction instances, shortcut context, action groups, action contexts, focused window/view gating, enabled state, checked state, menu text, toolbar text, shortcut text, and trigger forwarding. A single global QAction is not shared across windows or QML placements. Shortcuts are registered and resolved in the owning window-local action collection; QML does not create duplicate shortcut handlers for the same command, and triggers flow through the canonical runtime action path. The action runtime consumes session, document, viewport, and UI-local gate snapshots through explicit availability inputs. QML owns UI-local gate facts such as text-input focus, dialog modality, hovered or active view, and transient tool focus, reports them as revisioned snapshots, receives action proxies for placement only, and may add visual layout state, but it does not write action state or recompute shared cross-mode availability. Trigger forwarding is reentrancy-safe: disabled triggers no-op, checked toggles have one source of truth, action-group exclusivity is enforced by the action owner, repeated shortcut activation is serialized, and modal suppression is handled in the runtime gate.

Public derived state has projection owners arranged as a directed acyclic graph. Leaf document snapshots feed the session snapshot; the session snapshot feeds named projections; named projections feed QML and platform action objects. Projection owners do not query each other or arbitrary leaf properties while applying state. Each projection keeps its immutable current snapshot as a C++ value and exposes one stable QObject facade with a revision plus readonly scalar and model APIs for QML binding; QML does not retain or observe replaceable snapshot subobjects. Snapshot-backed models define whether each update is a reset or diff, use begin/end model notifications correctly, define persistent index invalidation, keep role names stable, define row identity for delegate reuse, and keep the previous snapshot alive through any binding or model notification that can still observe it. Individual property and model notifications may exist for binding ergonomics, but they are valid only as notifications that the current snapshot revision changed or that a model view of that snapshot was replaced; batch notification ordering alone is not treated as atomicity. Named projection owners are used for duplicated computation or multi-consumer public state, while small owner-local derived fields may remain fields of the owning snapshot. Projections expose availability and target identity only; side effects such as deletion, open-with execution, clipboard writes, or file-manager actions are executed only by command owners. Derived state such as active navigation, active zoom readout, title subject, action availability, thumbnail rows, and media information must not be recomputed independently in QML or in multiple facades.

Async lifecycle ownership is local and explicit. Every out-of-order workflow owns a scoped operation identity: a stable source key, operation id, generation, demand object, or render-frame revision. Every completion carries that identity back to the owner, which accepts, rejects, cancels, or no-ops it before mutating public state. The shared lifecycle vocabulary is a contract checklist, not a shared base class or universal state machine. Rust and worker-thread tasks return plain payloads that are safe to cross the worker boundary, Rust payloads that cross threads satisfy the required `Send` and lifetime constraints, Rust futures do not hold Qt object pointers, QObject mutation happens only on the GUI-thread owner through a queued acceptance path, and owner destruction is guarded by both owner tokens and weak Qt references before callbacks can mutate state. Queued callbacks that arrive after owner destruction or token invalidation must no-op. Every async owner contract names the owner token, QObject thread affinity, queued acceptance method, destruction invalidation point, and post-destruction best-effort cancellation or no-op behavior. Destructor and disconnect behavior is part of each owner contract, and each workflow states whether cancellation is guaranteed or best-effort. Callbacks never carry an alternate authoritative URL, page, frame, or status beside the owner's current scope.

Stable key families are part of every extensibility boundary. Media source keys identify ordinary files, opened-collection entries, archive roots, directories, videos, image-document pages, thumbnails, predecode candidates, and render surfaces using family-specific equality and freshness rules. Durable identity is kept separate from freshness generation, and each family defines normalization, collision, symlink or alias handling, case sensitivity, metadata freshness, and sandboxed URL behavior where those concepts apply. Demand objects describe requested work, including key, size bucket, priority, render context, visible area, or decode window. Result objects report ready, pending, unsupported, failed, canceled, and stale outcomes without changing ownership.

Rendering ownership is downstream and projection-only. Presentation owns the displayed surface and render snapshot. The tile-decode scheduler owns pending and failed tile keys for the active surface generation. Render-frame projection chooses draw entries and missing tile requests from one snapshot. Scene-graph nodes receive prepared immutable render entries with a bounded lifetime at the GUI-to-render handoff, own graphics resources only, follow the Qt scene graph thread and disposal lifecycle, define texture creation and destruction thread rules for updatePaintNode, releaseResources, scene-graph invalidation, QSGNode destruction, window changes, and device loss, define QSGTexture or QRhi resource ownership, invalidate surface generations on window changes or device loss, and never read GUI/domain state or recompute source selection, cache policy, tile priority, or fallback behavior.

Extension points are adapter contracts, not state backdoors. New decoders, opened-collection backends, thumbnail sources, media navigation sources, predecode policies, and render sources plug in by returning typed capabilities, plans, demands, or payloads to the owning runtime. They do not publish QML state directly, mutate action state, store a second current source, or bypass stale-completion acceptance.

## Progress

- [x] Rust/C++ ownership direction documented under `docs/architecture/`.
- [x] Image open workflow uses transition plans and runtime operation plans.
- [x] Document session route plans classify empty, direct video, direct image, and image-document routes.
- [x] Active navigation is projected through the session for shared toolbar, menu, shortcut, and thumbnail consumers.
- [x] Core async flows have operation ids or generations for image decode, image load sessions, direct-media navigation, predecode, thumbnails, video source resolution, and tile decode scheduling.
- [x] Thumbnail rows, foreground demand, background fill, cache lookup, generation, and image retention are owned by the session thumbnail runtime.
- [x] Direct-media predecode has a session-owned still-image-only coordinator separate from image-document predecode.
- [x] Define the minimum source key contract needed for route sealing, route equality, route freshness, session snapshots, and direct-media scope identity, including top-level URL and local-file normalization.
- [x] Seal top-level source routing so QML can request media only through the session route owner.
- [x] Move `QAction` enabled, checked, label, and trigger state out of QML and into the application action runtime.
- [x] Replace viewport scroll/frame synchronization with a revisioned presentation command and projection contract that separates canonical logical state from QML-owned physical item state.
- [x] Collapse spread zoom, per-page zoom, and spread transition state into one active presentation owner with restoration snapshots.
- [x] Convert session public projection updates into revisioned snapshot transactions sourced from leaf document snapshots.
- [x] Normalize remaining async workflows onto one operation/demand/completion vocabulary with explicit disconnect and cancellation semantics, and remove ad hoc delayed UI reconciliation.
- [x] Consolidate media information, action availability, active zoom, title, deletion, open-with, and active navigation as named projection owners.
- [x] Publish stable extension contracts for media sources, thumbnails, predecode, decoders, and render sources, including key-family equality and freshness rules.
- [x] Final acceptance: no QML writes shared `QAction` state, no public leaf-document route setter bypasses the session, no durable cross-mode state has two mutable owners, Qt Quick physical item state is bridged by revision and acknowledgement rather than mirrored authority, and every out-of-order completion is rejected by an owner-held identity before public mutation.

## Stage 1: Seal The Session Route Boundary

**Goal:** Make the session the only public owner of top-level media selection.

**Work:**

- Apply the commit-intent order for public behavior and ownership changes: spec commit if behavior changes, architecture commit if the boundary changes, failing test commit where practical, then implementation commit.
- Define the minimum source key contract for top-level route identity, route equality, route freshness, direct-media scope identity, and session snapshot equality before removing alternate public route setters; this minimum covers URL normalization, local file path normalization, relative and absolute path handling, percent encoding, trailing slash handling, route case-sensitivity policy, and whether symlink or alias paths are the same route. Defer archive, render, cache-specific, and sandboxed URL semantics to the extension-contract stage unless they are needed for route sealing.
- Add tests proving QML-facing media opens route through the session and cannot leave session source, document kind, direct-media cursor, and leaf document source out of sync.
- Remove or make internal the writable public route setters on leaf image and video documents; keep internal route ports for the session and document controllers.
- Keep leaf document standalone construction only if it is compile-time test-only or routed through a private controller fixture that cannot become a production alternate route.
- Update QML to use only session-level open methods for top-level media selection, drag-and-drop, file dialogs, direct-media navigation, and deletion follow-up routes.

## Stage 2: Own Actions In The Application Runtime

**Goal:** Stop QML from owning shared action state while preserving QML as the action layout layer.

**Work:**

- Apply the commit-intent order for action behavior, architecture, tests, and implementation.
- Separate application-global command identity from window-local action instances: key commands by stable registry name, then create window or main-controller action-collection instances for standard actions, custom actions, action groups, action contexts, focused window/view gates, user shortcut overrides, retranslation, menu text, toolbar text, checked state, enabled state, and trigger routing.
- Define a C++ action availability input snapshot that combines session public projection, mode-specific document snapshots, active viewport gates, and a QML-owned UI-local gate snapshot for help-dialog state, text-input focus, modality, hovered or active view, and transient tool focus.
- Include a UI gate revision or event sequence in the action availability input so stale focus, modality, hover, or transient-tool facts cannot reopen or close actions after newer gates arrive.
- Use a temporary compatibility input snapshot if Stage 5 or Stage 7 projection owners are not migrated yet, and remove that adapter once the session snapshot publishes active navigation, active zoom, deletion, open-with, operation availability, and active media readiness.
- Move enabled and checked decisions first, then menu text, toolbar text, shortcut text, standard action metadata, and first/last navigation label decisions from QML action wrappers into the application action runtime.
- Replace QML `QAction.enabled`, `QAction.checked`, duplicate `Shortcut`, duplicate `Keys.onPressed`, and duplicate Kirigami shortcut bindings with placement-only action proxies and trigger forwarding while preserving runtime-owned canonical Qt/KDE action objects and platform shortcut semantics; do not share a single global QAction across windows.
- Add tests that action state changes when session projection, deletion progress, text input focus, video mode, spread mode, right-to-left reading state, focused window/view gate, UI gate revision, modal suppression, action-group exclusivity, disabled trigger no-op, checked toggle source, and repeated shortcut activation change.

## Stage 3: Revision The Viewport Contract

**Goal:** Replace split-brain viewport state with an explicit C++ presentation projection and stale-safe QML application.

**Work:**

- Apply the commit-intent order for viewport behavior, architecture, tests, and implementation.
- Define a revisioned viewport contract with separate command revisions and observation or acknowledgement revisions, including requested content position, applied command revision, actual item position observation, observation origin, content size, image rect, visible item rect, pannability, command outcome, and pending, applying, applied, settled, acknowledged, superseded, or rejected status.
- Define the coordinate-space contract for source, image, item, content, viewport, device-pixel-ratio, rotation before and after transforms, zoom anchor, logical pixel, physical pixel, rounding, and fractional scroll values before moving viewport commands.
- Move zoom policy, scan policy, initial/final scan handoff, and content-position clamping into presentation commands that return command revisions, while leaving high-frequency gesture sampling and immediate physical item application in QML.
- Change QML from setting both `Flickable.contentX/Y` and `imageDocument.viewportContentPosition` to reporting user input, applying only the latest non-superseded command revision, and promoting actual physical item state to canonical logical state only at gesture commit, inertia settle, overshoot settle, resize commit, rotation commit, device-pixel-ratio commit, or explicit acknowledgement points.
- Remove `Qt.callLater` generation reconciliation once unit tests and a QML interaction harness cover command-origin observations, user-origin observations, inertia-origin observations, overshoot-origin observations, delayed frame publication, rapid wheel zoom, pinch or flick sampling, bounds rebound, animation settle, scan-then-navigation, resize relayout, device-pixel-ratio changes, rotation, and mode switch scenarios.

## Stage 4: Collapse Presentation Mode Ownership

**Goal:** Ensure single-page and two-page presentation expose exactly one active owner for zoom, viewport frame, and public presentation properties.

**Work:**

- Apply the commit-intent order for presentation behavior, architecture, tests, and implementation.
- Define an active presentation state shape with public transition state, mode, primary page, optional secondary page, zoom, rotation, reading direction, viewport frame, and restoration snapshot.
- Migrate in internal substeps: introduce the active state shape, switch read projections to that shape, move writes to the single active owner, then remove the old owner and synchronization paths.
- Replace spread-owned zoom plus per-page zoom synchronization with one active zoom owner and inactive restoration snapshots.
- Make spread transitions transactional: prepare next mode, load secondary page if needed, commit the new active mode, or abort while the public transition state remains previous-active, transitioning-placeholder, or committed-active.
- Add tests for entering spread mode, leaving spread mode, failed secondary page load, page navigation during spread transition, aborted transition, transition-state projection, rotation restrictions, old render-frame retention, and zoom preservation without cross-owner copying.

## Stage 5: Revision Session Projection

**Goal:** Publish mixed-media public state from one revisioned snapshot instead of signal-by-signal synchronization.

**Work:**

- Apply the commit-intent order for projection behavior, architecture, tests, and implementation.
- Define leaf document snapshots for image, video, viewport, navigation, deletion, source scope, metadata availability, and operation availability, each tied to an input revision.
- Change session recomputation to consume a complete snapshot and publish one immutable current public projection snapshot through a stable QObject facade with revisioned readonly scalar and model APIs for active navigation, active zoom, title subject, open-with availability, deletion availability, source identity, and error exposure.
- Define snapshot-backed model behavior: reset versus diff update, begin/end model notifications, persistent index invalidation, role-name stability, row identity, delegate lifetime, facade copy or shared immutable storage ownership, and previous snapshot lifetime during notifications.
- Remove partial recomputation paths that update active zoom, public navigation, and title independently when the same leaf transition changes several inputs; QML consumers must read the current facade snapshot, and individual property or model notifications must only signal that the snapshot revision or snapshot-backed model changed.
- Add tests for direct image replacement, video-to-image switches, failed replacement, page navigation refresh, deletion in progress, and mode switches producing no stale title, zoom, or active-navigation readout.

## Stage 6: Normalize Async Completion Contracts

**Goal:** Give every out-of-order workflow the same lifecycle vocabulary and acceptance discipline.

**Work:**

- Apply the commit-intent order for lifecycle architecture, tests, and implementation.
- Inventory every async owner and classify its identity as operation id, source key, generation, demand key, or frame revision.
- Introduce a shared C++ lifecycle checklist for start, cancel, accept completion, reject stale completion, disconnect on owner destruction, and publish result without forcing unrelated workflows into one base class or state machine.
- Classify each owner cancellation contract as guaranteed or best-effort, especially across Qt signals, jobs, CXX-Qt bridges, Rust async work, and scene-graph lifetimes.
- Require each owner contract to name its owner token, QObject affinity, queued acceptance method, destruction invalidation point, and post-destruction best-effort cancellation or no-op behavior.
- Require Rust and worker-thread workflows to return plain boundary-safe payloads only; payloads crossing worker or Rust async boundaries must satisfy the required `Send` and lifetime constraints, Rust futures must not hold Qt object pointers, QObject mutation must pass through a GUI-thread owner acceptance path guarded by owner identity, disconnect state, and weak Qt references where needed, and queued callbacks after drop or token invalidation must no-op.
- Convert remaining local boolean or delayed-callback guards to owner-held identities, starting with viewport updates and any facade-level signal bridges that mutate public state after a mode change.
- Add targeted stale-completion tests for every converted owner, then keep the existing full stale-completion suites passing.

## Stage 7: Consolidate Public Projections

**Goal:** Make derived public models explicit owners instead of incidental facade recomputations.

**Work:**

- Apply the commit-intent order for public projection behavior, architecture, tests, and implementation.
- Define named projection owners for active navigation, active zoom, title subject, action availability, media information, thumbnail rows, and deletion/open-with availability where state has duplicated computation or multiple public consumers, with a projection DAG rule of leaf snapshots to session snapshot to named immutable projection snapshots to QML.
- Move media information target selection and row projection behind a session-fed projection owner while keeping Qt clipboard and file-manager side effects in C++.
- Ensure projection owners receive snapshots, consume the same input revision for one recompute, replace a C++ current snapshot, and expose that snapshot through a stable QObject facade with revisioned readonly scalar and model APIs; snapshot-backed models must define reset or diff semantics, persistent index invalidation, role stability, row identity, delegate lifetime, and previous snapshot lifetime, and projection owners must not query arbitrary leaf properties or other projection owners while applying their own public state.
- Add tests that projection state clears or becomes unknown during empty startup, loading gaps, direct-image replacement, video errors, unsupported archive videos, and failed opened-collection metadata access.

## Stage 8: Publish Extension Contracts

**Goal:** Make future media and rendering features extend through adapters and demands instead of ownership shortcuts.

**Work:**

- Apply the commit-intent order for architecture, tests, and implementation when changing extension behavior.
- Expand the Stage 1 minimum source key contract into stable source key families for ordinary files, direct videos, image-document pages, opened-collection entries, archive roots, directory collections, thumbnail sources, predecode candidates, and render surfaces.
- For each key family, define durable identity, freshness generation, equality, normalization, alias or symlink behavior, case sensitivity, metadata freshness, and sandboxed URL handling where applicable.
- Define capability and result contracts for decoders, thumbnail source adapters, opened-collection backends, media navigation providers, predecode planners, and tile/render sources.
- Define the render-source and scene-graph resource contract, including immutable render entry lifetime, GUI-to-render-thread handoff, updatePaintNode, releaseResources, QSGNode destruction, scene-graph invalidation, texture creation and destruction thread, QSGTexture or QRhi ownership, window-change invalidation, and device-loss invalidation.
- Require every adapter to return plain capabilities, plans, demands, payloads, or completion results to the owning runtime; adapters cannot mutate QML state, physical item state, public session state, or platform action state directly.
- Add contract tests for unsupported media, stale source keys, cache identity, opened-collection entry freshness, video-vs-image eligibility, render-source fallback behavior, and surface-generation invalidation after window or device changes.

## Stage 9: Remove Backdoors And Enforce The Boundary

**Goal:** Lock in the target architecture so new work cannot reintroduce mirrored owners.

**Work:**

- Apply the commit-intent order for any final behavior or architecture cleanup.
- Remove obsolete QML properties, facade setters, compatibility helpers, and test hooks that bypass the named owners.
- Add lint or focused tests that fail when QML writes shared `QAction` state, directly sets leaf document source routes, recomputes shared active navigation availability, or bypasses placement-only action proxies.
- Add architecture tests, include dependency checks, compile-time boundary checks, and focused API tests for forbidden ownership directions: QML direct mutation of durable domain state instead of typed commands or fact snapshots to the named owner, render nodes to policy decisions or GUI/domain reads, adapters to public projection state, Rust async work to Qt/KDE runtime objects, duplicate shortcut ownership outside the runtime action collection, and C++ facade bypasses around session routing or projection owners.
- Finish with `just test`; run `just check` as well if source manifests, QML structure, lint rules, or cross-language integration changed.
