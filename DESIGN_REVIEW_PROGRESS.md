# Design Review Progress

## 2026-06-07

Completed checkpoint: P1 image deletion readiness at the command boundary.

What changed: added focused deletion-controller characterization tests for retained image surfaces while the image document state is `Loading` or `Error`, then guarded `ImageDocumentDeletionController::deleteDisplayedFile()` so `FileDeletionProvider` is reached only when the image document state is `Ready`.

Behavior kept unchanged: ready-image deletion still starts the file operation, uses the same deletion target resolution, reports progress the same way, preserves cancellation behavior, and uses the existing fallback selection after success.

Verification: the new characterization tests failed before the guard because a file deletion operation was started in both non-ready states. After the guard, `test_imagedocumentdeletioncontroller` passed, `devenv tasks run --mode single ci:test:cpp` passed, and `devenv tasks run --mode single ci:lint:cpp` passed.

Deviation: no product spec update was needed because `docs/spec/file-access.md` already states that deletion applies when an image or direct video is ready.

Remaining P1 follow-up work: centralize deletion eligibility into a named snapshot or plan shared by session projection, facade/runtime commands, and the deletion controller; validate active-navigation numbered dispatch before operations are created; unify thumbnail source identity and freshness generation; split small workflows out of `DocumentSessionRuntime` and the image runtime controller callback mesh; add typed image/video failure preservation; extract HEIF/BMFF brand knowledge to one policy source; move viewport command planning out of `KiriImageDocument`; add provider seams for live directory watching and predecode coordinator timers.
