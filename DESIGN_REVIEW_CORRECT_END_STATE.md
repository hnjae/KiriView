# Design Review: Correct End State

## Executive Summary

KiriView already has a sound architectural direction: `QML -> facade -> C++ runtime/effect executor -> Rust policy`. The remaining open design-review issue is lower-level image decoder and refinement failure representation.

The highest-risk remaining finding affects diagnostics, retry semantics, and user/internal error separation: lower-level image decode and refinement failures still have compatibility paths that can collapse typed diagnostics into string-only results.

The correct end state should be precise and conservative, not clever. Rust policy should own duplicated-free domain decisions and typed plans. C++ runtime should own Qt/KDE objects and external effects behind typed adapters. Facades should expose QML-friendly types and forward commands. QML should report geometry/input facts and render projections. Errors should flow internally as typed failures with source, stage, diagnostic detail, severity, and retryability, while the UI receives only user-facing messages.

## Top Design Risks

1. P2: Lower-level image decoder and remaining refinement failures still lose backend-specific diagnostics before document wrapping, which weakens diagnostics, retry semantics, and user/internal error separation.

## Error Handling and Observability Problems

### Finding: Lower-level image decode failures still lose backend-specific diagnostics before document wrapping

- Evidence: `src/decoding/decodedimageresult.h` now carries route, operation, diagnostic detail, severity, and retryability on `DecodedImageFailure`; `src/rendering/qimagereadertilesource.cpp` preserves ordered tile attempt diagnostics with severity and retryability, but still exposes a compatibility string path for older consumers.
- Current state: The image document boundary stores typed `ImageLoadFailure` for data-load, decode, opened-collection candidate-load, empty opened-collection, and presentation failures while preserving QML-facing user messages. `QImageReaderTileSource::decodeTileWithDiagnostics()` preserves ordered reader-clip, source-clip, scaled-level, and full-image fallback failures with severity and retryability, selected decoder route now stamps `DecodedImageFailure`, static-image decode failures preserve first-display/blocking-preview operation, diagnostic detail, severity, and retryability, the Qt raster adapter now stamps route/operation plus backend detail onto direct adapter failures, Qt raster animation-open adapter failures now preserve operation and adapter detail, raw decode failures now preserve raw route, operation, and LibRaw stage/code/detail, HEIF still/sequence failures now preserve HEIF route, operation, and backend diagnostic detail, and SVG/APNG adapter failures now preserve route, operation, and adapter detail. Remaining refinement and compatibility paths still collapse typed diagnostics into string-only error paths for some consumers.
- Design concern: Document-level tests can distinguish failure kind and source identity, but deeper production diagnostics still cannot reliably distinguish remaining refinement operation/detail, severity, or retryability across all paths.
- Correct end state: Decode failure results and remaining tile/refinement failure paths should preserve backend/route, operation, diagnostics, severity, and retryability before projecting a document-level user message.
- Suggested migration: Extend `DecodedImageFailure` and static decode results to typed failure payloads, then map those into `ImageLoadFailure` without changing current QML error text.
- Acceptance criteria: Decoder and refinement tests can assert route/backend/operation diagnostics separately from the document-level user message.
- Priority: P2

## Recommended Correct End-State Architecture

- Error representation: Image decoder, remaining tile/refinement, media-entry source, and thumbnail generation failures use typed failures. Internal paths preserve source identity, stage/kind, backend/raw code, severity, and retryability. QML receives user-facing projections.
- Tests: Decoder and refinement tests assert route/backend/operation diagnostics separately from document-level user messages.

## Suggested Refactoring Sequence

1. Improve error semantics and observability: extend lower-level image decoder and remaining refinement diagnostics. Preserve UI text while internal diagnostics become structured.

## Things Not To Change Yet

- Do not introduce a generic logging/observability framework before typed failure values exist. Without typed failures, logging would mostly preserve the current string ambiguity.
- Do not alter user-visible error text as part of the first migration unless current text is clearly a bug. Internal typed diagnostics can be added while preserving existing UI messages.
