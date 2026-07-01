# Architecture Decisions

Use `docs/adr/NNNN-title.md` for durable decisions that are too specific for the architecture contract files but important enough to preserve with dated rationale. Keep ADRs short:

- Context
- Decision
- Consequences

ADR-worthy decisions include changing a Rust/C++ ownership boundary, replacing a media decoding or rendering family, moving workflow state ownership across the FFI boundary, or choosing a durable dependency boundary whose rationale would otherwise be lost.

Architecture contract files remain the normative source for the current intended design. ADRs preserve rationale and consequences; they must not be used as deferred-work lists or implementation-status indexes.
