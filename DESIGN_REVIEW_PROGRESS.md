# Design Review Progress

This file is an agent-facing status index for `DESIGN_REVIEW_CORRECT_END_STATE.md`. Use it to avoid reopening completed findings and to pick the next design-review task.

## Completed

- P1 HEIF/BMFF brand policy: centralized HEIF still/sequence brand kind and family membership in Rust policy and updated HEIF/input classification tests. Verified with `devenv shell -- devenv tasks run --mode single ci`. Commit: a841f33a.
- P1 typed video source-load failures: preserved source URL, kind, diagnostic detail, severity, and retryability through video document state while keeping public error text unchanged; added video state/runtime regression tests and trimmed the remaining design-review backlog to backend playback errors. Verified with devenv shell -- devenv tasks run --mode single ci. Commit: 5cd4b34b.
- P1 typed video backend failures: preserved backend playback source URL, kind, diagnostic detail, severity, and retryability through video document state while keeping public backend error text unchanged; added video state/runtime regression tests and updated architecture/design-review docs. Verified with devenv shell -- devenv tasks run --mode single ci. Commit: 2f9a4ced.
- P1 thumbnail source identity: replaced the active-navigation thumbnail runtime's local source-key type with canonical `ThumbnailSourceKey` and explicit generation freshness checks; added source-key, runtime, and architecture-boundary coverage and removed the completed backlog item. Verified with devenv shell -- devenv tasks run --mode single ci. Commit: bdb3b78e.
- P1 predecode runtime fact injection: injected timer scheduler and thread-count providers through image and media predecode dependency paths, replaced coordinator/controller wall-clock waits with manual timer tests, and updated architecture/design-review docs. Verified with devenv shell -- devenv tasks run --mode single ci. Commit: a22b26ac.

## Current Status

Resolved findings:

- Image deletion readiness at the command boundary is resolved. `ImageDocumentDeletionController::deleteDisplayedFile()` now requires `ImageDocumentStatus::Ready` before invoking `FileDeletionProvider`, and retained-image `Loading`/`Error` states are covered by focused tests.
- Active-navigation numbered dispatch validation is resolved. Rust active-navigation policy clamps numbered `Number`, `First`, and `Last` requests before C++ receives `OpenDirectMediaNavigationAtNumberOperation` or `OpenImageDocumentPageAtNumberOperation` payloads, and Rust/C++ tests cover out-of-range requests.

Do not reopen these as design-review findings unless the implementation regresses. Future work may still refactor nearby ownership or naming, but the original correctness gaps are closed.

## Next High-Value Work

P1 candidates:

- Split workflow ownership. `DocumentSessionRuntime`, session leaf ports, `ImageDocumentRuntimeControllers`, and `KiriImageDocument` still concentrate broad workflow orchestration.
- Preserve typed failures. Image, KIO/file-operation, collection-source, tile-decode, and thumbnail failures still lose structured diagnostic fields or collapse to raw strings.
- Move viewport command planning out of `KiriImageDocument`. The facade still owns `ImageViewportInteraction`, anchored zoom positioning, scan-start handoff, and related command calculations.
- Add a provider seam for live directory watching. `ImageDocumentPageCandidateDirectoryEntry` still owns `KCoreDirLister` directly.

P2/P3 candidates:

- Align image format capability metadata with decode classification and routing.
- Centralize zoom preset descriptors and `ImageShortcutScope` validity.
- Move `MediaEntrySourceStore` away from document load-planning types.
- Split `ApplicationCommandRouterPorts` by command owner.
- Move localized open-dialog filter text out of `navigation/mediaformatregistry`.
- Either wire `ActiveNavigationThumbnailDemandTracker` into production demand flow or remove/rename it as a test helper.
- Narrow `ImageDocumentRuntimePlan` operation families now that explicit `ImageDocumentRuntimePlanExecutor` dispatch exists.

## Verification History

Deletion readiness checkpoint:

- New tests initially failed because file deletion started in retained-image `Loading` and `Error` states.
- After the guard, `test_imagedocumentdeletioncontroller` passed.
- Final checks passed: `devenv tasks run --mode single ci:test:cpp` and `devenv tasks run --mode single ci:lint:cpp`.

Active-navigation numbered dispatch checkpoint:

- New Rust test initially failed with operation number `0`.
- After policy clamping, focused Rust `activenavigation` tests and focused C++ `test_activenavigationprojection` passed.
- Final checks passed: `devenv tasks run --mode single ci:test`, `devenv tasks run --mode single ci:lint:rust`, and `devenv tasks run --mode single ci:lint:cpp`.

HEIF/BMFF brand policy checkpoint:

- New Rust coverage verifies that every HEIF still-image and image-sequence brand routes as AVIF-compatible HEIF-family input.
- The production HEIF brand table now lives in one Rust policy module consumed by both input routing and container kind analysis.
- Focused Rust `heif` tests and full CI passed.

## Spec Notes

- No product spec update was needed for deletion readiness because `docs/spec/file-access.md` already says deletion applies when an image or direct video is ready.
- The user-visible page-number-entry rule remains: open the nearest valid page or media item.
