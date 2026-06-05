# Testing Strategy

Test Rust policy in Rust unit tests when the logic is independent of Qt. These tests should cover state transitions, edge cases, and policy tables.

Test C++ runtime code with Qt tests when behavior depends on:

- Qt object lifetime or signals.
- `QImage`, `QUrl`, display-source data, or render-context capability data.
- KIO or file-operation adapters.
- Controller integration across async boundaries.

Do not duplicate every Rust policy test in C++. C++ tests should verify that the runtime layer applies plans correctly and preserves integration behavior.

## Boundary Enforcement Tests

Architecture boundary tests should fail when code reintroduces a second owner for durable public state. Prefer simple source-pattern checks for QML and facade API boundaries, and focused Qt tests for runtime behavior that depends on signal ordering or object lifetime.

Boundary tests must cover these forbidden directions: QML writing shared `QAction` state, QML action proxies overriding runtime-owned enabled or checked state, QML recomputing shared active media readiness or action availability from raw document properties, QML storing viewport revisions in numeric properties, QML creating duplicate shortcut handlers for runtime-owned commands, production QML or C++ setting leaf route state directly, production C++ calling public image-document mutators that bypass presentation owners, page presentation controllers retaining mutable active zoom or rotation owners, action runtime accepting stale UI gate revisions, custom image render nodes or visual tile fallbacks reading document or presentation policy state, generic source keys crossing adapter or display-source boundaries, provider requests mutating document state, and projection owners querying arbitrary leaf facades while applying a named projection snapshot.

Runtime tests should cover the accepted replacement paths for those boundaries: revisioned UI gate snapshots, typed presentation and viewport commands, document-session route commands, session projection snapshot replacement, fixed shortcut routes through the action runtime gate, provider request boundaries, display-source stale-completion rejection, and render-context freshness after window, DPR, scene-graph, or texture-capability changes.
