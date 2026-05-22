# Documentation

This directory contains KiriView's project documentation. Keep user-visible behavior, architecture direction, and packaging notes in the narrowest document set that owns the topic.

- [Product Specifications](spec/README.md): canonical user-visible behavior. Update the relevant subject files before implementing product behavior changes.
- [Architecture](architecture/README.md): long-term ownership, language-boundary, FFI, workflow, testing, and evolution guidance.
- [Architecture Decision Records](adr/): accepted design decisions that need historical context or tradeoff records.
- [Flatpak](flatpak.md): Flatpak build, packaging, and runtime notes.

When a change fits more than one area, prefer the document that future maintainers would read before touching the affected code. Product behavior belongs in `spec/`; implementation shape belongs in `architecture/`; one-time rationale with durable consequences belongs in `adr/`.
