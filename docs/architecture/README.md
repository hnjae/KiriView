# Architecture

This directory records the long-term architecture direction for KiriView. It is not a product behavior specification; user-visible behavior belongs in the files under `../spec/`.

Use these documents when changing module ownership, crossing the Rust/C++/QML boundary, introducing new runtime workflow shape, or adjusting test strategy. Architecture decision records with dated rationale live in `../adr/`.

- [Overview](overview.md)
- [Layer Ownership](layer-ownership.md)
- [Language Boundary](language-boundary.md)
- [State Ownership](state-ownership.md)
- [FFI Design](ffi-design.md)
- [Workflow Shape](workflow-shape.md)
- [Target Direction](target-direction.md)
- [Testing Strategy](testing-strategy.md)
- [Evolution Rules](evolution-rules.md)
- [Architecture Decisions](architecture-decisions.md)
