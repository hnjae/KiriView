# Layer Ownership

QML and Kirigami own declarative UI composition:

- Page structure, menus, toolbars, overlays, and shortcut attachment points.
- Visual placement of action objects supplied by the C++ facade.
- Bindings to documented QObject properties and invokable methods.
- UI-only state that does not affect application behavior.

C++ QObject and QQuickItem facade classes own the public QML surface:

- `Q_OBJECT`, `Q_PROPERTY`, `Q_INVOKABLE`, signals, and QML registration.
- Conversion between QML-friendly types and internal controller APIs.
- Thin forwarding to controllers and render items.

These facade classes live under `src/facade/`. Keep QML-facing API shape there and move domain behavior into the appropriate runtime, presentation, rendering, or policy module instead of growing facade-owned workflow state.

C++ Qt/KDE runtime code owns platform integration and side effects:

- `QObject` lifetime, signal delivery, thread affinity, and cancellation.
- `QUrl`, `QImage`, `QAction`, `QQuickItem`, image-provider entries, and other Qt objects.
- OS and desktop-environment observers such as system-memory discovery and power-saver portal state.
- `QAction` identity, KDE action collections, configured shortcut state, and shortcut persistence.
- KIO jobs, KDE settings, dialogs, notifications, file operations, and runtime integration.
- Image presentation, provider-backed display publication, and async job orchestration.
- The image presentation runtime as the single owner of active image presentation state: mode, reading direction, transition phase, zoom, rotation, logical viewport frame, visible source rect, display-source projections, page visibility, restoration snapshots, and display refinement demand.
- Image page surface owners as resource owners for display entries, animation playback, image revision, display-image pin leases, previous-frame retention, predecode facts, and load lifetimes only.
- Render context discovery, such as device pixel ratio and maximum safe display texture size, through one document-owned provider. The provider-rendering architecture uses a non-rendering bridge and stable public Qt/Qt Quick capability inputs or conservative fallbacks rather than direct QRhi ownership.

Rust owns Qt-independent policy and algorithms:

- State transitions, workflow plans, and reducer-like decisions.
- Zoom, viewport, provider display bucket, compatibility tile, spread, navigation, deletion-target, follow-up, and cache decisions when they are computed from plain value snapshots.
- Parsing and byte-level format inspection that is independent of Qt objects.
- Pure calculations where the same input should produce the same output.
