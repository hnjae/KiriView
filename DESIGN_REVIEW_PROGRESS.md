# Design Review Progress

This file is an agent-facing status index for `DESIGN_REVIEW_CORRECT_END_STATE.md`. Use it to avoid reopening completed findings and to pick the next design-review task.

## Current Status

Resolved findings:

- Image deletion readiness at the command boundary is resolved. `ImageDocumentDeletionController::deleteDisplayedFile()` now requires `ImageDocumentStatus::Ready` before invoking `FileDeletionProvider`, and retained-image `Loading`/`Error` states are covered by focused tests.
- Active-navigation numbered dispatch validation is resolved. Rust active-navigation policy clamps numbered `Number`, `First`, and `Last` requests before C++ receives `OpenDirectMediaNavigationAtNumberOperation` or `OpenImageDocumentPageAtNumberOperation` payloads, and Rust/C++ tests cover out-of-range requests.

Do not reopen these as design-review findings unless the implementation regresses. Future work may still refactor nearby ownership or naming, but the original correctness gaps are closed.

## Next High-Value Work

P1 candidates:

- Unify thumbnail source identity. `ThumbnailSourceKey` and `ActiveNavigationThumbnailSourceKey` still define production-like identity semantics separately; separate durable identity from freshness generation.
- Split workflow ownership. `DocumentSessionRuntime`, session leaf ports, `ImageDocumentRuntimeControllers`, and `KiriImageDocument` still concentrate broad workflow orchestration.
- Preserve typed failures. Image, video, KIO/file-operation, collection-source, tile-decode, and thumbnail failures still lose structured diagnostic fields or collapse to raw strings.
- Extract HEIF/BMFF brand knowledge. `imageinputclassification.rs` and `heifcontainer.rs` still carry separate brand classification policy.
- Move viewport command planning out of `KiriImageDocument`. The facade still owns `ImageViewportInteraction`, anchored zoom positioning, scan-start handoff, and related command calculations.
- Add provider seams for live directory watching and predecode timing. `ImageDocumentPageCandidateDirectoryEntry` still owns `KCoreDirLister` directly, and high-level predecode coordinator tests still rely on wall-clock waits.

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

## Spec Notes

- No product spec update was needed for deletion readiness because `docs/spec/file-access.md` already says deletion applies when an image or direct video is ready.
- The user-visible page-number-entry rule remains: open the nearest valid page or media item.
