# Target Direction

KiriView should evolve toward coherent Rust policy units rather than many small FFI helpers. A Rust module should represent a meaningful workflow or algorithmic domain, such as opening, navigation, deletion, zoom, spread layout, tile selection, parsing, or cache policy.

Small local branches should stay in C++ unless they are part of a larger Qt-independent policy decision. Moving a branch to Rust is useful when it clarifies ownership, strengthens tests, or lets multiple C++ runtime paths share one policy result. It is not useful when the bridge types and conversions become larger than the decision being moved.

The preferred direction is fewer, larger policy boundaries:

- Workflow reducers that accept plain events and return state deltas plus effects.
- Geometry and rendering policy modules that compute values without owning renderer objects.
- Parsing modules that inspect bytes and return plain metadata or decoded domain values.
- Cache prioritization and navigation policies that can be tested without Qt event loops and do not own Qt runtime objects.

The document-session refactor should extract coherent contracts before moving implementation. Useful boundaries include active navigation projection from session and image-document snapshots, direct media routing plans, ordinary direct media deletion fallback plans, action-availability projections, and a named still-image-only direct media predecode eligibility policy.

Session-to-leaf document ports should expose cohesive snapshot families and one snapshot-change event per family, not property-shaped signal bags. Leaf snapshot ports must stay separate from leaf command/effect ports: session projection refresh and thumbnail-source adaptation consume `DocumentSessionImageDocumentSnapshot` and `DocumentSessionVideoDocumentSnapshot` values, while routing, page navigation, deletion, and stop cross explicit command/effect ports. Image source assignment, image-page navigation, and image-document deletion effects should cross a focused session image-document command runtime. Video source assignment, stop, and clear-on-session-exit effects should cross a focused session video-document command runtime; video-output attachment effects should cross a focused video-output runtime attachment port after the session runtime has applied public-projection eligibility.

Active-navigation dispatch should be a named document-session subowner. The subowner may hold pending reveal context and execute typed direct-media or image-page navigation operations through narrow ports, while `DocumentSessionRuntime` supplies the current public active-navigation source/snapshot and `DocumentSessionState` remains the only public projection owner.

Session projection publication should be a named document-session subowner. The projection runtime should apply revisioned `DocumentSessionPublicSnapshotInput` values to `DocumentSessionState`, publish image-page-only active-navigation updates through the same state owner, and synchronize active-navigation thumbnail rows from the committed projection facts. `DocumentSessionRuntime` supplies snapshot inputs and leaf thumbnail rows, but it must not own the snapshot commit algorithm or thumbnail-row publication.

Session public-snapshot input assembly should be a named document-session subowner. The input builder should construct `DocumentSessionPublicSnapshotInput` from committed session state, cached image/video leaf snapshots, direct-media navigation facts, and operation-availability facts. `DocumentSessionRuntime` supplies those facts, but it must not inline the projection-input field mapping or query leaf facades while building an input.

Session public leaf-snapshot assembly should be a named document-session subowner. The leaf snapshot builder should translate cohesive `DocumentSessionImageDocumentSnapshot` and `DocumentSessionVideoDocumentSnapshot` values into `DocumentSessionPublicImageLeafSnapshot` and `DocumentSessionPublicVideoLeafSnapshot` values. `DocumentSessionRuntime` may cache the built public leaf snapshots, but it must not inline raw leaf-to-public field mapping.

Direct-media deletion completion application should be a named document-session subowner. The completion runtime should clear deletion progress, report typed file-operation failures through `DocumentSessionState`, and forward accepted fallback route plans to the route runtime. `DocumentSessionRuntime` supplies state and route ports, but it must not inline completion-result branching.

Route planning and execution should be a named document-session subowner. The route runtime should classify source/media URLs into route plans, own route-operation dispatch, and keep route-local execution facts such as direct-media scope changes. Route plan data should distinguish ordered mutation operations from typed follow-up effect requests instead of expressing publication, direct-media refresh, or predecode clearing as peers of state mutation. `DocumentSessionRuntime` supplies the current document kind plus typed mutation/effect ports for leaf routing, public projection publication, direct-media navigation refresh, predecode clearing, and route-completion cleanup. Route execution should collect mutation facts first, then publish accepted public projection changes once, then run follow-up effects such as direct-media navigation refresh and predecode clearing.

Direct-image cursor synchronization should be a named document-session planner. The planner should decide whether a pending direct-image cursor is confirmed, restored after failure, or left unchanged from committed cursor and image leaf facts. `DocumentSessionRuntime` applies the returned cursor operation to `DocumentSessionState`, but it must not inline pending/displayed/source/error branching.

Direct-media navigation application should be a named document-session subowner. The application runtime should translate refresh/open workflow applications into state replacement, reveal-context operations, projection publication, predecode scheduling, and follow-up media routes through narrow ports. `DocumentSessionRuntime` may supply current cursor and active-navigation facts, but it must not inline workflow-application branching or own the effect sequence.

Viewport scan-start state, pan/scan command planning, anchored zoom positioning, viewport point-query geometry, and viewport command projection application should belong to the image-document runtime/presentation layer, not QML. The facade may keep forwarding QML commands and a non-rendering QQuickItem bridge may adapt the physical `Flickable` position, but C++ owns command revision tracking, begin/complete/acknowledge ordering, and observation revision reconciliation.

Those boundaries should keep `DocumentSessionState` as the public C++ owner and keep Qt/KDE effects in C++. Rust may later host pure projection or plan computation from plain snapshots, but C++ applies the resulting state, executes document routing, publishes signal batches, and preserves cursor-generation rejection for stale async completions.

When adding C++ or Rust files for these boundaries, keep source ownership manifests in sync, including `src/rust_policy_sources.txt`, `src/rust_bridge_sources.txt`, and relevant REUSE coverage.

C++ should remain the place where those plans are executed through Qt/KDE APIs. This keeps the application adaptable while the pre-release codebase continues to change.
