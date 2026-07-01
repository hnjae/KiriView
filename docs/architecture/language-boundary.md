# Language Boundary

The Rust/C++ boundary is a policy boundary, not a mechanical split by feature name. A module belongs in Rust when its inputs and outputs can be expressed as plain values and it does not need to know about Qt object lifetime or side effects.

Prefer Rust for logic that:

- Can be represented as `State + Event -> StateDelta + Effects`.
- Is reused by multiple controllers or presentation paths.
- Is complex enough that Rust unit tests materially improve confidence.
- Parses media bytes or file paths into owned, Qt-independent metadata values.
- Would make a C++ controller hard to read if left inline.
- Avoids direct dependency on `QObject`, `QImage`, `QUrl`, KIO, or rendering APIs.

Prefer C++ for logic that:

- Is mostly `QObject`, Qt property, signal, or QML API plumbing.
- Depends on `QImage`, `QUrl`, `QAction`, `KIO::Job`, `QQuickItem`, or Qt Quick provider and presenter APIs.
- Exists to manage async lifetime, cancellation, ownership, or thread affinity.
- Immediately executes Qt/KDE side effects.
- Is only a small local branch whose Rust bridge would be larger than the policy itself.

Rust must not call back into Qt/KDE adapters directly. It returns typed plans, state deltas, or effect descriptions. C++ executes those effects and feeds completion events back into the workflow.
