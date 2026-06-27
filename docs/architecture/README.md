# Architecture

This directory records the long-term architecture direction for KiriView. It is not a product behavior specification; user-visible behavior belongs in the files under `../spec/`.

Use these documents when changing module ownership, crossing the Rust/C++/QML boundary, introducing new runtime workflow shape, or adjusting testing ownership and boundary enforcement. Keep milestone acceptance criteria and deferred-work lists in `../planning/`. Architecture decision records with dated rationale live in `../adr/`.

- [Overview](overview.md)
- [Terminology](terminology.md)
- [Layer Ownership](layer-ownership.md)
- [Language Boundary](language-boundary.md)
- [State Ownership](state-ownership.md)
- [Provider Rendering Architecture](provider-rendering.md)
- [Thumbnail Source Adapters](thumbnail-source-adapters.md)
- [Extension Contracts](extension-contracts.md)
- [FFI Design](ffi-design.md)
- [Workflow Shape](workflow-shape.md)
- [Async Lifecycle](async-lifecycle.md)
- [Policy Boundary Contracts](target-direction.md)
- [Testing Strategy](testing-strategy.md)
- [Evolution Rules](evolution-rules.md)
- [Architecture Decisions](architecture-decisions.md)
