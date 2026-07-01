# Architecture

This directory defines KiriView's durable implementation contracts. It is not a product behavior specification; user-visible behavior belongs in the files under `../spec/`.

Architecture documents define ownership boundaries, dependency direction, data ownership, state-transition enforcement, async lifecycle rules, failure-handling contracts, and boundary-test ownership. Milestone acceptance criteria, deferred-work lists, implementation progress notes, and temporary migration rationale do not belong here. Architecture decision records with dated rationale live in `../adr/`.

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
