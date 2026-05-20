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

C++ Qt/KDE runtime code owns platform integration and side effects:

- `QObject` lifetime, signal delivery, thread affinity, and cancellation.
- `QUrl`, `QImage`, `QAction`, `QQuickItem`, `QSGRenderNode`, and other Qt objects.
- OS and desktop-environment observers such as system-memory discovery and power-saver portal state.
- `QAction` identity, KDE action collections, configured shortcut state, and shortcut persistence.
- KIO jobs, KDE settings, dialogs, notifications, file operations, and runtime integration.
- Image presentation, rendering, and async job orchestration.
- View-owned render context discovery, such as device pixel ratio and maximum texture size. A document may consume one render context provider, but page render items must not compete to install independent providers for the same document.

Rust owns Qt-independent policy and algorithms:

- State transitions, workflow plans, and reducer-like decisions.
- Zoom, viewport, tile, spread, navigation, deletion-target, follow-up, and cache decisions when they are computed from plain value snapshots.
- Parsing and byte-level format inspection that is independent of Qt objects.
- Pure calculations where the same input should produce the same output.
