# Active Image Navigation Transition Implementation Plan

This implementation plan is an execution queue, not a source of truth. If this file conflicts with `docs/spec/**` or `docs/architecture/**`, the spec and architecture documents win.

## Conformance Audit

Recent documentation commits define the end state for retained image-to-image active navigation. The current implementation is not yet conformant with that end state.

Active navigation dispatch already chooses image-document page targets, but a selected page URL is still routed through the source-load workflow. When the selected image URL differs from the current source URL, source loading classifies the request as replacement and runs the full replacement sequence, including navigation cancellation, predecode cancellation, source URL mutation, and begin-open.

The current public image projection is tied to document readiness. During loading, the image document reports no displayed URL, the session reports the active image as not ready, and QML sizes the image presentation to zero. This means a prepared or nearly prepared target can still look slow because the previously committed image is hidden before the new provider-backed image has been accepted.

The current predecode scheduler is display-success-driven. It cancels background work on each schedule request, waits for a debounce before starting adjacent work, disables parallel work during scrubbing, and may reload opened-collection candidates before computing a window that active navigation already knows. This makes fast Home, End, Page Up, Page Down, page-number, and thumbnail navigation likely to fall back to foreground loading or to wait behind presentation acknowledgement.

## Relevant Spec And Architecture References

- `docs/spec/image-display.md`: retained image display during same-scope image navigation, readiness-dependent controls, and selected-target failure behavior.
- `docs/spec/navigation.md`: active navigation selection, displayed media identity, page readout during pending loads, and background loading behavior.
- `docs/spec/comic-archives.md`: Home and End behavior inside comic book archives.
- `docs/architecture/terminology.md`: committed image presentation and pending image navigation target terms.
- `docs/architecture/workflow-shape.md`: same-scope image navigation as target selection rather than source replacement.
- `docs/architecture/state-ownership.md`: C++ ownership of committed presentation, pending navigation target, active navigation projection, and image-document predecode.
- `docs/architecture/provider-rendering.md`: provider projection retention and commit acknowledgement rules.

## Milestone Plan

### Milestone 1: Split Image Navigation Target Selection From Source Replacement

Status: Completed

Completed notes:

- Same-scope image navigation now has an explicit source-load kind that updates the selected source and starts the image open without replacement-only navigation or predecode cancellation.
- Ordinary direct media image-to-image navigation from image mode routes through the same same-scope image navigation command; top-level source assignment and video-to-image routing continue to use normal source assignment.
- Conservative assumption: same-scope image target selection reuses the current opened-collection media-entry source and therefore does not run `PrepareSourceLoadOperation`; active scope changes, container navigation, and other replacement paths still prepare source lifetime through the replacement workflow.

Dependencies: None

Likely code areas:

- image-document source-load workflow and source-load request types
- image-document page navigation controller and runtime plan execution
- document-session active navigation dispatch and projection inputs

Acceptance criteria:

- Same-scope image-to-image active navigation does not classify a URL change as top-level source replacement.
- Previous, Next, First, Last, page-number, and thumbnail activation can publish a pending image navigation target for ordinary direct media scopes and opened collection scopes.
- Same-scope image navigation does not run `CancelAllNavigation`, does not clear active navigation rows, and does not clear provider-ready predecode/cache state solely because the selected image URL changed.
- Top-level source assignment, active scope changes, container navigation to another collection, image-to-video transitions, video-to-image transitions, empty source, and selected-target errors still use the appropriate replacement, routing, or clearing workflow.
- Existing archive video handoff and unsupported-video placeholder behavior remain unchanged.

Expected tests/checks:

- Add focused failing tests first when practical.
- Rust source-load policy tests for same-scope image navigation versus replacement source loading.
- C++ image-document runtime plan tests proving page navigation does not emit replacement-only operations.
- Session active-navigation tests for direct media and opened collection image targets.

### Milestone 2: Add Committed And Pending Image Presentation State

Status: Completed

Completed notes:

- Same-scope image navigation now retains the committed provider-backed page projection while the selected image target is loading, including previous-active two-page spread transitions.
- Newly published target display entries carry stale-retained status and `retainWhileLoading` eligibility until the matching display-load acknowledgement releases the prior retained entry.
- Adjacent page navigation effects now route through the page-navigation source-load operation so ordinary previous/next commands use the same retained same-scope presentation path as explicit page selection.

Dependencies: Milestone 1

Likely code areas:

- image-open transition state and application plan application
- image presentation runtime and page-surface display-source projection
- image loader predecode promotion and decoded-image completion handling
- display-load acknowledgement handling

Acceptance criteria:

- The currently committed image presentation remains visible while a same-scope selected image target is pending and no prepared target image is ready.
- A predecoded or otherwise prepared target image can be promoted and committed without first blanking the viewport.
- A newly decoded target image commits only after the provider-backed display entry is published and accepted for the current target identity.
- Stale decode, predecode, refinement, or QML acknowledgement completions cannot commit an older target after a newer navigation selection.
- If the selected target fails, the selected target's error state is shown and the prior committed image is no longer treated as the active media item.
- Top-level source replacement and image-to-video transitions still clear or leave image mode according to the specs.

Expected tests/checks:

- Add focused failing tests first when practical.
- Image open workflow tests for pending-target success, stale completion rejection, and selected-target failure.
- Provider/display-source tests for retained committed projection and acknowledged target commit.
- Integration tests covering rapid image-to-image navigation with a delayed foreground decode.

### Milestone 3: Align Public Projection, Readiness, And QML Retention

Status: Not started

Dependencies: Milestones 1 and 2

Likely code areas:

- document-session public leaf snapshot and public projection builders
- active image readiness, active zoom, action availability, and media information projections
- image viewport and display image QML bindings
- application action runtime inputs for image-only controls

Acceptance criteria:

- During pending same-scope image navigation, active navigation readout, selected thumbnail, and page-number entry follow the selected target.
- During the same interval, displayed image URL and media information for the committed image continue to identify the actually visible image until the target commits.
- Image-only controls, panning affordances, zoom editing, rotation, and ready-image action gates are disabled while the selected target is pending, even if a previous committed image remains visible.
- The retained image is visible through the normal provider-backed presentation path; QML does not invent a separate stale-image cache, duplicate source URL, or readiness policy.
- Mode switches, startup loading, unknown navigation, and replacement failures still clear or mark unknown public values before publishing a readout from a different scope.

Expected tests/checks:

- Add focused failing tests first when practical.
- Public projection tests for selected target versus displayed committed image.
- QML or Qt Quick viewport tests proving retained image dimensions do not collapse to zero during pending image navigation.
- Action availability tests proving image-only controls are disabled during pending navigation.
- `devenv tasks run --mode single ci:lint:qml` if QML files change.

### Milestone 4: Schedule Predecode From Navigation Selection

Status: Not started

Dependencies: Milestones 1 through 3

Likely code areas:

- image-document predecode controller and coordinator
- document-session direct media predecode runtime
- shared predecode schedule state and window planning
- active navigation candidate snapshot plumbing

Acceptance criteria:

- Selecting an image target in a known active navigation scope immediately computes a still-image predecode window around the selected target from the confirmed navigation rows.
- Same-scope target changes reprioritize the window without clearing provider-ready cached images that are still valid for the scope.
- Fast repeated navigation coalesces obsolete background work while preserving the most recent selected target as the priority center.
- Scrubbing may limit background throughput, but it must not prevent the selected target and its immediate still-image window from being considered for urgent preparation when power saver allows it.
- Power Saver mode continues to suppress new background work while allowing foreground display and already retained prepared images.
- Video rows remain cursor positions only; no video frame predecode is introduced.

Expected tests/checks:

- Add focused failing tests first when practical.
- Predecode schedule-state tests for immediate selected-target window updates, scrubbing behavior, and retained cache behavior.
- Image-document and direct-media predecode coordinator tests using deterministic timers and candidate snapshots.
- Session tests for rapid Page Up/Page Down and Home/End target changes.

### Milestone 5: Reuse Confirmed Candidate Snapshots For Opened Collection Planning

Status: Not started

Dependencies: Milestone 4

Likely code areas:

- image-document page candidate repositories and navigation services
- opened collection image loader planning
- image-document predecode window planning
- live candidate refresh integration

Acceptance criteria:

- Opened collection foreground image loads and predecode window planning can consume the confirmed active-navigation candidate snapshot when it is fresh for the current opened collection scope.
- Candidate listing is not repeated only to locate a target already present in the confirmed active navigation rows.
- Live refresh, deletion fallback, and container navigation still invalidate or replace candidate snapshots when the opened collection contents or scope changes.
- Empty collection, missing target, unsupported-video placeholder, and playable collection-video handoff behavior remain unchanged.

Expected tests/checks:

- Add focused failing tests first when practical.
- Page candidate repository/navigation tests for snapshot reuse and invalidation.
- Image loader tests proving known opened-collection targets do not require an extra candidate load before decode or predecode.
- Session tests for live candidate refresh after retained navigation changes.

### Milestone 6: Integration And Regression Verification

Status: Not started

Dependencies: Milestones 1 through 5

Likely code areas:

- cross-module tests and fixtures
- any CMake, QML lint, or Rust test manifests touched by prior milestones

Acceptance criteria:

- Ordinary direct image navigation, direct image-to-video navigation, opened archive image navigation, opened directory image navigation, playable collection video, unsupported collection video placeholders, two-page spread, right-to-left reading, page-number entry, thumbnail activation, and selected-target failure paths match the authoritative docs.
- Rapid Home, End, Page Up, Page Down, page-number, and thumbnail navigation does not blank the viewport for same-scope image-to-image pending loads when a committed image is available.
- Predecoded targets commit without running the replacement workflow or canceling same-scope navigation state.
- No tests assert unrelated repository artifacts outside the ownership boundary of the code under test.
- Final verification succeeds.

Expected tests/checks:

- Focused Rust and C++ tests affected by each milestone while iterating.
- `devenv tasks run --mode single ci:lint:qml` if QML files changed.
- `devenv tasks run --mode single ci:lint:cpp` if C++ lint surface changed.
- `devenv tasks run --mode single ci:lint:rust` if Rust source changed.
- `devenv tasks run --mode single ci:test`

## Cross-Milestone Dependencies

- Milestone 2 depends on Milestone 1 because committed/pending presentation needs a same-scope target-selection workflow.
- Milestone 3 depends on Milestones 1 and 2 because public projection must observe both selected target and committed presentation.
- Milestone 4 depends on Milestones 1 through 3 because predecode priority must consume the pending selected target without becoming a public-state owner.
- Milestone 5 depends on Milestone 4 because candidate snapshot reuse must serve both navigation target selection and predecode window planning.
- Milestone 6 depends on all prior milestones.

## Stop Conditions

Stop and ask before proceeding if any of these conditions occur:

- `docs/spec/**` and `docs/architecture/**` conflict on behavior that affects implementation.
- The implementation would require editing `docs/spec/**` or `docs/architecture/**`.
- The straightforward solution would require adding, replacing, vendoring, patching, or upgrading external dependencies.
- The straightforward solution would require private Qt/KDE APIs, a custom scene graph renderer, visual tile rendering, KIOFuse remounting for opened collection pages, or provider requests that perform decode or mutate public state.
- Work would modify upstream, third-party, generated, vendored, packaged dependency sources, or `po/*.po` files.
- Work would require destructive file or git operations, deleting real user files, or changing repository history.
- Verification would require external credentials, network-only samples, privileged portals, GUI permissions beyond normal local test execution, or host codec assumptions not already covered by the project test environment.
