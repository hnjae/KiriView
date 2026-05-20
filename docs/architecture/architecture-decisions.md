# Architecture Decisions

Use `docs/adr/NNNN-title.md` for decisions that are too specific for this overview but important enough to preserve. Keep ADRs short:

- Context
- Decision
- Consequences

Examples of ADR-worthy decisions include changing the Rust/C++ ownership model for image opening, replacing a Qt image path with a Rust decoder path, or moving workflow state ownership across the FFI boundary.

Existing ADRs:

- `../adr/0001-single-open-archive-document-session.md`: directly opened archive document sessions are owned by the C++ document runtime, including the archive location, sorted candidate list, and serialized archive image reads.
- `../adr/0002-libpng-apng-streaming-decoder.md`: APNG playback is owned by the C++ runtime through APNG-patched libpng so frames can be decoded sequentially instead of materializing the full animation in memory.
- `../adr/0003-resvg-svg-rendering.md`: static SVG parsing and rasterization are owned by Rust through resvg, while C++ keeps Qt image objects and tile-source integration.
- `../adr/0004-explicit-image-input-classification.md`: image byte and file-name classification is owned by Rust, while C++ executes the selected decoder path and reports final decoder failures.
