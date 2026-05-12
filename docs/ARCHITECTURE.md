# Architecture

This document records the long-term architecture direction for KiriView. It is
not a product behavior specification; user-visible behavior belongs in
`SPEC.md`.

KiriView is a KDE Kirigami image viewer built from three cooperating layers:

```text
QML / Kirigami
    -> C++ QObject and QQuickItem facade
        -> C++ Qt/KDE runtime and effect executor
            -> Rust Qt-independent policy core
        <- typed plans, state deltas, and effect descriptions
```

The main maintenance goal is to keep product policy testable without making Rust
own Qt runtime concerns. Rust should decide what should happen. C++ should know
how to make it happen in Qt and KDE.

## Layer Ownership

QML and Kirigami own declarative UI composition:

- Page structure, actions, menus, toolbars, overlays, and shortcuts.
- Bindings to documented QObject properties and invokable methods.
- UI-only state that does not affect application behavior.

C++ QObject and QQuickItem facade classes own the public QML surface:

- `Q_OBJECT`, `Q_PROPERTY`, `Q_INVOKABLE`, signals, and QML registration.
- Conversion between QML-friendly types and internal controller APIs.
- Thin forwarding to controllers and render items.

C++ Qt/KDE runtime code owns platform integration and side effects:

- `QObject` lifetime, signal delivery, thread affinity, and cancellation.
- `QUrl`, `QImage`, `QAction`, `QQuickItem`, `QSGRenderNode`, and other Qt
  objects.
- KIO jobs, KDE settings, dialogs, notifications, file operations, and runtime
  integration.
- Image presentation, rendering, and async job orchestration.

Rust owns Qt-independent policy and algorithms:

- State transitions, workflow plans, and reducer-like decisions.
- Zoom, viewport, tile, spread, navigation, deletion, and cache policy.
- Parsing and byte-level format inspection that is independent of Qt objects.
- Pure calculations where the same input should produce the same output.

## Language Boundary

The Rust/C++ boundary should be a policy boundary, not a mechanical split by
feature name. A module belongs in Rust when its inputs and outputs can be
expressed as plain values and it does not need to know about Qt object lifetime
or side effects.

Prefer Rust for logic that:

- Can be represented as `State + Event -> StateDelta + Effects`.
- Is reused by multiple controllers or presentation paths.
- Is complex enough that Rust unit tests materially improve confidence.
- Would make a C++ controller hard to read if left inline.
- Avoids direct dependency on `QObject`, `QImage`, `QUrl`, KIO, or rendering
  APIs.

Prefer C++ for logic that:

- Is mostly `QObject`, Qt property, signal, or QML API plumbing.
- Depends on `QImage`, `QUrl`, `QAction`, `KIO::Job`, `QQuickItem`, or
  `QSGRenderNode`.
- Exists to manage async lifetime, cancellation, ownership, or thread affinity.
- Immediately executes Qt/KDE side effects.
- Is only a small local branch whose Rust bridge would be larger than the
  policy itself.

Rust should not call back into Qt/KDE adapters directly. It should return typed
plans, state deltas, or effect descriptions. C++ should execute those effects
and feed completion events back into the workflow.

## FFI Design

FFI code should be intentionally boring. A good bridge is explicit, typed, and
easy to audit.

Use small bridge structs and enums for:

- Stable policy inputs.
- State snapshots.
- Change sets.
- Effect plans.

Avoid bridges that expose:

- Raw Qt object ownership.
- Long-lived Rust references to C++ objects.
- Implicit side effects hidden behind policy functions.
- One-off wrappers whose only purpose is to move a local C++ `switch` into
  Rust.

When a Rust module starts to look like glue, either move the branch back to C++
or absorb it into a larger Rust workflow reducer where it becomes part of a
coherent policy decision.

## Workflow Shape

The preferred long-term shape for product workflows is event-driven:

```text
C++ receives UI/runtime event
    -> converts it into a plain workflow event
        -> Rust computes state delta and effects
            -> C++ applies state, executes effects, and reports completions
```

For example, opening an image can eventually converge on:

```text
OpenRequested(url)
ImageDataLoaded(bytes)
ImageDecoded(result_summary)
ImageLoadFailed(error)
PresentationFinished
```

Rust can decide loading status, error recovery, navigation updates, cache
policy, and follow-up effects. C++ keeps the actual KIO job, decoder job,
presentation controller, image object, and render update mechanics.

This does not require every existing controller to be rewritten at once. Move
logic when a workflow is already being changed and when the new boundary reduces
complexity.

## Target Direction

KiriView should evolve toward coherent Rust policy units rather than many small
FFI helpers. A Rust module should represent a meaningful workflow or algorithmic
domain, such as opening, navigation, deletion, zoom, spread layout, tile
selection, parsing, or cache policy.

Small local branches should stay in C++ unless they are part of a larger
Qt-independent policy decision. Moving a branch to Rust is useful when it
clarifies ownership, strengthens tests, or lets multiple C++ runtime paths share
one policy result. It is not useful when the bridge types and conversions become
larger than the decision being moved.

The preferred direction is fewer, larger policy boundaries:

- Workflow reducers that accept plain events and return state deltas plus
  effects.
- Geometry and rendering policy modules that compute values without owning
  renderer objects.
- Parsing modules that inspect bytes and return plain metadata or decoded
  domain values.
- Cache and navigation policies that can be tested without Qt event loops.

C++ should remain the place where those plans are executed through Qt/KDE APIs.
This keeps the application adaptable while the pre-release codebase continues to
change.

## Testing Strategy

Test Rust policy in Rust unit tests when the logic is independent of Qt. These
tests should cover state transitions, edge cases, and policy tables.

Test C++ runtime code with Qt tests when behavior depends on:

- Qt object lifetime or signals.
- `QImage`, `QUrl`, or rendering data.
- KIO or file-operation adapters.
- Controller integration across async boundaries.

Do not duplicate every Rust policy test in C++. C++ tests should verify that the
runtime layer applies plans correctly and preserves integration behavior.

## Evolution Rules

When adding or moving logic:

1. Start from ownership: policy in Rust, Qt/KDE execution in C++.
1. Keep FFI value-based and explicit.
1. Move whole policy decisions, not isolated boolean branches.
1. Keep QObject/QML API stable inside C++ facade classes.
1. Avoid adding compatibility layers for pre-release internal formats unless
   explicitly requested.
1. Document meaningful architecture decisions here or in an ADR.

## Architecture Decisions

Use `docs/adr/NNNN-title.md` for decisions that are too specific for this
overview but important enough to preserve. Keep ADRs short:

- Context
- Decision
- Consequences

Examples of ADR-worthy decisions include changing the Rust/C++ ownership model
for image opening, replacing a Qt image path with a Rust decoder path, or moving
workflow state ownership across the FFI boundary.
