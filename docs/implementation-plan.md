# Implementation Plan

This document records an execution queue for bringing the current implementation into conformance with the authoritative end-state documents under `docs/spec/` and `docs/architecture/`.

This document is not a source of truth. If this plan conflicts with `docs/spec/**` or `docs/architecture/**`, the spec or architecture document wins.

Implementation work should preserve valuable existing implementation by replacing behavior deliberately and proving the documented end state with tests. The repository does not require backward compatibility unless `AGENTS.md` or the user explicitly says otherwise.

## Conformance Audit

### Documented Requirements

- KiriView opens one selected image, direct video, supported archive, or local directory, then displays and navigates adjacent media or collection entries in the active scope; references: `docs/spec/scope.md`, `docs/spec/file-access.md`, `docs/spec/navigation.md`.
- Direct images include local and KDE-supported remote URLs with the documented image extension set; direct videos include MP4, M4V, and MOV local, file, and KDE-supported remote URLs; KDE archive-entry URLs remain direct media URLs rather than opened collection scopes; references: `docs/spec/file-access.md`, `docs/spec/video-playback.md`.
- Directly opened comic archives, directly opened general archives, and directly opened local directories become opened collection scopes with recursive supported media ordering, first supported item selection, empty collection handling, and collection-specific deletion/current-media rules; references: `docs/spec/file-access.md`, `docs/spec/comic-archives.md`, `docs/spec/navigation.md`.
- Opened collection video playback is limited to directory files, stored ZIP entries, and plain TAR entries; ineligible collection videos remain navigation rows and show the documented unsupported-video placeholder; references: `docs/spec/file-access.md`, `docs/spec/video-playback.md`, `docs/spec/navigation.md`.
- The main UI has exactly one toolbar, app-menu and menubar parity, viewer context menu rules, fullscreen overlay behavior, shortcut help, configurable shortcuts, info panel, thumbnail panel, and video playback controls; references: `docs/spec/main-window.md`, `docs/spec/video-playback.md`.
- Static images, animated images, SVG, HEIF, RAW, previews, refinement, zoom, fit, pan, spread, RTL, rotation, and stale-load retention follow the documented image-display behavior; references: `docs/spec/image-display.md`, `docs/spec/comic-archives.md`.
- The thumbnail strip has one row per supported active navigation item and only generates thumbnails for documented eligible source kinds; references: `docs/spec/navigation.md`, `docs/architecture/thumbnail-source-adapters.md`.
- Session owns mixed-media routing, source identity, active navigation, public projections, current media/deletion/open-with planning, thumbnails, and action availability inputs; references: `docs/architecture/state-ownership.md`, `docs/architecture/workflow-shape.md`.
- QML owns composition and placement only; C++ owns QObject, Qt/KDE/KIO/QAction, display provider, async, and side effects; Rust owns plain-value policy and reducers; references: `docs/architecture/layer-ownership.md`, `docs/architecture/language-boundary.md`, `docs/architecture/ffi-design.md`.
- Image presentation must use provider-backed whole-image display/refinement through Qt Quick image/provider items and must not expose custom render nodes, visual tile surfaces, tile scheduling, decoded-tile caches, render-frame APIs, or low-level rendering resources; references: `docs/architecture/provider-rendering.md`, `docs/architecture/extension-contracts.md`.

### Current Implementation Status

- The repository already contains substantial end-state structure: `src/session/`, `src/navigation/`, `src/archive/`, `src/video/`, `src/thumbnail/`, `src/predecode/`, `src/presentation/`, `src/application/`, `src/facade/`, and `src/qml/` map closely to the documented ownership areas.
- `KiriDocumentSession` and `DocumentSessionRuntime` already centralize mixed-media routing, active navigation projection, public snapshots, direct media navigation, deletion/open-with planning, thumbnail demand, predecode coordination, and video output attachment.
- The QML shell already uses a single main toolbar, `MediaWorkspaceHost`, `MediaViewportHost`, `ImageViewport`, `VideoViewport`, `InfoPanel`, `ThumbnailPanel`, `ApplicationMenuBar`, `ShortcutHelpDialog`, and action proxies.
- Open dialog filters currently advertise ordinary media and comic archives, not general archives or directories; this matches the documented advertised entry points.
- The desktop file currently advertises images, direct videos, and comic archive MIME types; this matches the documented desktop file scope.
- The Flatpak manifest currently exposes home, `/media`, `/mnt`, `/run/media`, XDG thumbnails, PipeWire, and GVfs paths; this broadly matches the documented access model but should still be verified against video and portal workflows.
- The provider-backed QML display path exists through `DisplayImagePage`, `KiriImageDisplaySource`, `ImagePageSurfaceController`, and `DisplayImageStore`.
- Architecture boundary tests already cover many durable rules, including leaf source setter privacy, QML action-state restrictions, provider path expectations, source-key families, thumbnail contracts, and test/build source ownership.

### Gaps

- The rendering pipeline still exposes tile-era source contracts: `ImageTileSource::decodeTile`, `imagetile*`, `qimagereadertilesource`, `svgtilesource`, `heiftilesource`, `qimagereaderscaledlevelcache`, and Rust `imagetilegeometry` remain in production source manifests.
- Boundary tests do not yet forbid the current `decodeTile` public contract or all tile-era public vocabulary, even though provider-rendering architecture excludes visual tile scheduling and tile-surface contracts.
- `KiriImageDocument` still exposes shared navigation and deletion operations as public QML invokables, including `openPreviousPage`, `openNextPage`, `openImageAtPage`, and `deleteDisplayedFile`; production QML mostly routes through session/actions, but the public leaf surface still permits owner bypass.
- Direct directory collection behavior, general archive entry behavior, direct archive-entry versus opened collection routing, collection video eligibility, and current-media/deletion target rules need full conformance coverage.
- Live directory updates need verification that they apply only to ordinary local direct media parents and not remote URLs, archive-entry URLs, opened archive scopes, or recursive directory collection scopes.
- Thumbnail generation needs full eligibility coverage for direct local images, direct local videos, CBZ/direct ZIP image pages, placeholders for unsupported or ineligible sources, DPR buckets, stale completions, and source-key identity.
- Image-display behavior needs full coverage for same-scope stale retention, image/video mode replacement, trusted previews, RAW previews, SVG security/refinement, animation playback, oversize/unsafe errors, zoom/fit state transitions, spread pairing, and rotation reset behavior.
- Video behavior needs full coverage for direct-video routing, collection-video source identity, playback preparation failure messages, action availability, fixed seek shortcuts, timeline gates, deletion follow-up, and Flatpak multimedia access.
- Info panel and Open With behavior need coverage for current media target differences, CBR/RAR unavailable states, collection-relative paths, direct video metadata, and no collection-internal video metadata parsing.
- Startup and input handling need integration coverage for first argument only, `--verbose`/`-v`, missing local startup path exit code 2, file chooser single selection, and drop-first-item behavior.

### Conflicting Existing Behavior

- Public tile source APIs conflict with the provider-only rendering architecture even where those classes now also provide whole-image decode/refinement helpers.
- Public image-leaf navigation/deletion invokables conflict with the durable session ownership boundary for shared navigation and displayed-media operations.
- Existing source-key naming includes `RenderSurfaceKey`; provider-rendering docs reject render-frame public APIs and tile-era rendering vocabulary, so this naming should be clarified before renaming or preserving it as-is.
- `ApplicationMenuHost.qml` appears unreferenced by production QML and may be legacy UI structure; confirm generated QML module and tests before removal.

### Likely Removal Or Replacement

- Replace `ImageTileSource` with whole-image display/refinement source contracts that expose first-display, raster refinement, blocking display, diagnostics, byte cost, image transform, and resolution-independence without a public tile decode method.
- Replace or delete `src/rendering/imagetile.*`, `src/rendering/imagetilegeometrypolicy.*`, `src/rendering/imagetilevisibility.*`, `src/rendering/qimagereadertilesource.*`, `src/rendering/svgtilesource.*`, `src/rendering/heiftilesource.*`, `src/rendering/qimagereaderscaledlevelcache.*`, and `src/policy/imagetilegeometry.rs` after replacement contracts and tests are in place.
- Update `src/cpp_core_sources.txt`, `src/rust_policy_sources.txt`, `src/rust_bridge_sources.txt`, `src/lib.rs`, and build/test source expectations when rendering contracts move.
- Remove public QML-invokable image-leaf shared navigation/deletion methods after session routes and tests no longer need them as public API.
- Remove unused QML artifacts such as `ApplicationMenuHost.qml` only after reference audits, QML lint, and C++ QML tests prove they are unused.
- Do not delete `ImageShortcuts.qml` blindly; it is intentionally referenced by shortcut and architecture tests even though runtime shortcut installation is centralized.

### Existing Tests To Preserve, Update, Or Delete

- Preserve architecture boundary tests and extend them as boundaries become stricter.
- Preserve session route, public projection, active navigation, direct media navigation, deletion/open-with, video sync, thumbnail runtime, predecode, action, shortcut, menu, main-window integration, info panel, thumbnail panel, decoder, metadata, animation, RAW, HEIF, SVG, and localization tests where they assert documented behavior.
- Update tile-source tests that actually validate first-display/refinement diagnostics so they target the replacement display/refinement source contracts.
- Delete tests that only assert public visual tile geometry, tile visibility, tile request scheduling, or `decodeTile` behavior after the replacement architecture removes those concepts.
- Add focused tests for direct directory collections, general archive entry points, direct archive-entry routing, collection-video eligibility/source identity, info panel target rules, deletion targets/follow-up, Flatpak manifest behavior if packaging changes, and provider-only rendering boundaries.

### Ambiguities Or Conflicts In Authoritative Docs

- `RenderSurfaceKey` appears in the architecture source-key contract, while provider-rendering rejects render-frame public APIs and tile-era rendering vocabulary; clarify whether this key should be renamed to display-source terminology or kept as a non-rendering presentation identity.
- Remote direct-video playback is documented behaviorally, but the implementation strategy for KIO/GVfs streaming, temporary local devices, and Flatpak access is not specified; do not broaden permissions or add dependencies without confirmation.
- Context menu rules explicitly keep image-only actions visible but disabled in video mode; app-menu visibility is less explicit, so avoid changing app-menu visibility solely by inference.
- Thumbnail wording distinguishes CBZ and direct ZIP as generated-thumbnail-eligible while CB7/7z and other non-ZIP-backed collections are ineligible; treat CBZ as ZIP-backed unless the specs are clarified otherwise.

### Risks

- Rendering contract replacement can regress large images, SVG, HEIF, RAW, preview quality, refinement latency, memory ceilings, and animation behavior.
- Directory deletion targets can remove whole directories; behavior must be proven through KDE-operation mocks and confirmation/cancel/failure paths before changing destructive flows.
- Action ID, shortcut scope, or persisted config key churn can reset user shortcut/menu preferences; backward compatibility is not required, but churn should be deliberate and tested.
- Flatpak/video changes can accidentally broaden filesystem or multimedia permissions beyond the documented model.
- External dependency, version, upstream patch, or vendored workaround changes are blocked by repository policy unless explicitly requested.
- Translation changes must not hand-edit `po/*.po`; string changes require template/check updates and translator-supplied catalog changes only.

## Milestone Plan

### Milestone 1: Conformance Characterization Tests

- Status: Not started.
- Purpose: Convert the most important documented gaps into executable tests before changing behavior.
- Dependencies: None.
- Relevant references: `docs/spec/scope.md`, `docs/spec/file-access.md`, `docs/spec/navigation.md`, `docs/spec/video-playback.md`, `docs/architecture/state-ownership.md`, `docs/architecture/provider-rendering.md`, `docs/architecture/testing-strategy.md`.
- Likely code areas: `tests/cpp/`, `src/policy/*`, `src/session/*`, `src/navigation/*`, `src/archive/*`, `src/video/*`, `src/rendering/*`.
- Acceptance criteria: Tests or test TODOs with explicit failing expectations exist for routing matrix gaps, active navigation invariants, collection video eligibility, current-media/deletion target rules, thumbnail eligibility, and provider-only rendering boundaries.
- Expected tests/checks: Focused Rust policy tests for plain-value planning; focused C++ tests for session/navigation/rendering boundaries; no full suite required for intent-only test commits unless changes are broad.
- Suggested commit boundaries: Commit route/navigation characterization tests separately from rendering boundary characterization tests.
- Stop conditions: Stop if a failing expectation depends on an ambiguity in authoritative docs or would require changing `docs/spec/**` or `docs/architecture/**`.

### Milestone 2: Session Routing And Source Identity

- Status: Not started.
- Purpose: Make session routing and public source identity match the documented direct media, opened collection, and mode-switch semantics.
- Dependencies: Milestone 1 characterization tests for routing and source identity.
- Relevant references: `docs/spec/file-access.md`, `docs/spec/navigation.md`, `docs/spec/video-playback.md`, `docs/architecture/state-ownership.md`, `docs/architecture/workflow-shape.md`, `docs/architecture/language-boundary.md`.
- Likely code areas: `src/session/documentsession*`, `src/navigation/directmedia*`, `src/navigation/imagedocumentpagecandidate*`, `src/archive/*`, `src/video/*`, `src/policy/imageopenworkflow.rs`, `src/policy/applicationruntime.rs`.
- Acceptance criteria: Direct images, direct videos, direct KDE archive-entry URLs, directly opened archives, and directly opened directories route to the correct document kind and active navigation scope; image-to-image same-scope navigation preserves committed presentation until target readiness; image-to-video and video-to-image switches clear stale mode state; failed targets remain active errors; collection videos keep collection entry source identity and never synthesize ordinary direct media scope.
- Expected tests/checks: Focused session route/projection tests, direct media navigation tests, image open workflow Rust tests, video document sync tests, then `devenv tasks run --mode single ci:test:cpp` if C++ session behavior changed broadly.
- Suggested commit boundaries: Commit Rust route policy changes, C++ runtime application changes, and test updates separately when practical.
- Stop conditions: Stop before changing documented source identity behavior, preserving compatibility with an old route model, or introducing broad local workarounds for KIO/video limitations.

### Milestone 3: Archive And Directory Collection Semantics

- Status: Not started.
- Purpose: Complete opened collection behavior for comic archives, general archives, and directly opened local directories.
- Dependencies: Milestone 2.
- Relevant references: `docs/spec/file-access.md`, `docs/spec/comic-archives.md`, `docs/spec/navigation.md`, `docs/architecture/extension-contracts.md`, `docs/architecture/async-lifecycle.md`.
- Likely code areas: `src/archive/archiveformat*`, `src/archive/mediaentrysource*`, `src/navigation/imagecontainer*`, `src/navigation/imagedocumentpagecandidate*`, `src/session/*`, `tests/cpp/test_mediaentrysourcebackend.cpp`, `tests/cpp/test_archiveformat.cpp`.
- Acceptance criteria: Directly opened CBZ/CBT/CB7/CBR and ZIP/TAR/7Z/RAR files open first supported media; directly opened local directories open recursive supported media; empty collections clear displayed media and use collection-neutral error wording; previous/next archive works only for local sibling comic archives; general archives and directories are not advertised in the default open filter or desktop file; opened collection snapshots remain stable rather than live-watched.
- Expected tests/checks: Media entry source backend tests, archive format tests, image container navigation tests, document session integration tests; run `devenv tasks run --mode single ci:test:cpp`.
- Suggested commit boundaries: Commit archive format/routing changes separately from directory collection behavior and sibling archive navigation.
- Stop conditions: Stop before adding archive libraries, changing dependency versions, patching upstream archive behavior, or deleting archive support that has not been deliberately replaced.

### Milestone 4: Video End State

- Status: Not started.
- Purpose: Bring direct video and eligible opened collection video behavior into spec conformance.
- Dependencies: Milestone 2 and Milestone 3.
- Relevant references: `docs/spec/video-playback.md`, `docs/spec/file-access.md`, `docs/spec/navigation.md`, `docs/spec/main-window.md`, `docs/architecture/state-ownership.md`, `docs/architecture/thumbnail-source-adapters.md`.
- Likely code areas: `src/video/*`, `src/session/documentsessionvideo*`, `src/session/documentsessionruntime*`, `src/qml/VideoViewport.qml`, `src/qml/VideoFloatingControls.qml`, `src/application/*`, `org.hnjae.kiriview.json`, `tests/cpp/test_mainwindowvideointegration.cpp`, `tests/cpp/test_videodocument*`.
- Acceptance criteria: Direct MP4/M4V/MOV startup, drop, open, and adjacent navigation are direct media; eligible directory/stored-ZIP/plain-TAR collection videos play from collection devices; ineligible collection videos show `KiriView can't play this video from the selected collection.` and remain navigation rows; video controls, seek gates, unsupported action toasts, autoplay, end-of-stream behavior, mute persistence, and deletion follow-up match the spec.
- Expected tests/checks: Video source/load/control tests, session video sync tests, shortcut/action tests, main-window video integration tests; run `devenv tasks run --mode single ci:test:cpp`; run `just build-with-tests` only if Flatpak-only codec, manifest, or runtime dependency behavior changes.
- Suggested commit boundaries: Commit video source eligibility separately from UI/action control behavior and Flatpak/package changes.
- Stop conditions: Stop before adding codecs, adding dependencies, changing Flatpak permissions beyond documented access, patching Qt/KDE/FFmpeg/libheif behavior, or exposing collection-internal video metadata.

### Milestone 5: Actions, Menus, Panels, Startup, And Operations

- Status: Not started.
- Purpose: Finish visible shell behavior and user operations that depend on shared session/action ownership.
- Dependencies: Milestone 2; Milestone 4 for video-specific action states.
- Relevant references: `docs/spec/main-window.md`, `docs/spec/file-access.md`, `docs/spec/video-playback.md`, `docs/architecture/state-ownership.md`, `docs/architecture/layer-ownership.md`, `docs/architecture/async-lifecycle.md`.
- Likely code areas: `src/application/*`, `src/facade/kiriviewapplication*`, `src/facade/kiridocumentsession*`, `src/facade/kirimediainformation*`, `src/qml/Main.qml`, `src/qml/ImageToolBar.qml`, `src/qml/ApplicationMenuBar.qml`, `src/qml/ContextActionMenu.qml`, `src/qml/InfoPanel.qml`, `src/qml/ShortcutHelpDialog.qml`, `src/policy/startup_arguments.rs`.
- Acceptance criteria: Exactly one toolbar remains visible outside fullscreen; menu and shortcut surfaces share action identity, text, shortcuts, enabled state, and routing; Open With and Delete target the documented current media/deletion targets; fullscreen/Escape/pointer/toolbar timing follows the spec; info panel copy/open-folder availability matches current media target rules; startup handles first arg, `--verbose`/`-v`, missing local path exit 2, single file chooser selection, and drop-first-item behavior.
- Expected tests/checks: Action registry/runtime tests, shortcut policy tests, menu access key tests, main-window toolbar tests, info panel tests, startup Rust tests, `devenv tasks run --mode single ci:lint:qml`, and focused Qt/QML CTest targets.
- Suggested commit boundaries: Commit startup policy, action/menu availability, operation targets, and panel/fullscreen UI behavior in separate commits where practical.
- Stop conditions: Stop before changing persistent action IDs or shortcut names without deliberate acceptance, before changing destructive deletion targets without test coverage, or before modifying `po/*.po`.

### Milestone 6: Provider-Only Rendering Migration

- Status: Not started.
- Purpose: Remove tile-era public rendering contracts and complete the documented provider-backed whole-image display/refinement model.
- Dependencies: Milestone 1 rendering boundary tests; Milestone 2 for stable source identity; image-display characterization tests for previews/refinement.
- Relevant references: `docs/spec/image-display.md`, `docs/architecture/provider-rendering.md`, `docs/architecture/extension-contracts.md`, `docs/architecture/state-ownership.md`, `docs/architecture/testing-strategy.md`.
- Likely code areas: `src/rendering/staticimage*`, `src/rendering/imagetile*`, `src/rendering/qimagereadertilesource*`, `src/rendering/svgtilesource*`, `src/rendering/heiftilesource*`, `src/rendering/rasterdisplaybucketpolicy*`, `src/decoding/staticimagedecode*`, `src/decoding/qimagereaderdecoder*`, `src/decoding/rawdecoder*`, `src/decoding/heifdecoder*`, `src/presentation/imagepagesurfacecontroller*`, `src/qml/DisplayImagePage.qml`, `src/policy/imagetilegeometry.rs`.
- Acceptance criteria: Production source no longer exposes public visual tile decode/scheduling/cache contracts; display/refinement sources publish immutable whole-image payloads through provider entries; SVG requests do not rasterize in the provider; unsafe oversized images produce documented errors instead of tile fallback; XDG/RAW previews, refinement replacement, and animation routes remain correct.
- Expected tests/checks: Updated display source/refinement tests, image page surface controller tests, display store tests, SVG/HEIF/RAW/QImageReader tests, architecture boundary tests, Rust tests if policy manifests change, `devenv tasks run --mode single ci:test:cpp`, and `devenv tasks run --mode single ci:lint:cpp`.
- Suggested commit boundaries: Commit replacement display/refinement interfaces first, migrate decoders/presentation second, remove tile-era files/manifests third, update boundary tests last if needed.
- Stop conditions: Stop before deleting tile-era code that still carries unreplaced preview/refinement functionality, before adding an image/rendering dependency, or before resolving the `RenderSurfaceKey` naming ambiguity without confirmation.

### Milestone 7: Thumbnail And Predecode Finalization

- Status: Not started.
- Purpose: Align thumbnail generation, placeholder behavior, demand buckets, and still-image predecode with navigation and provider boundaries.
- Dependencies: Milestone 2; Milestone 3; Milestone 4 for video rows; Milestone 6 for rendering source contracts if thumbnail generation currently depends on tile-era APIs.
- Relevant references: `docs/spec/navigation.md`, `docs/spec/video-playback.md`, `docs/architecture/thumbnail-source-adapters.md`, `docs/architecture/state-ownership.md`, `docs/architecture/async-lifecycle.md`.
- Likely code areas: `src/session/activenavigationthumbnail*`, `src/session/documentsessionthumbnail*`, `src/thumbnail/*`, `src/predecode/*`, `src/navigation/*`, `src/qml/ThumbnailPanel.qml`, `tests/cpp/test_activenavigationthumbnailruntime.cpp`, `tests/cpp/test_thumbnailgeneration.cpp`, `tests/cpp/test_thumbnailpanel.cpp`.
- Acceptance criteria: The thumbnail strip has one row per supported active navigation item; generated thumbnails are attempted only for documented eligible source kinds; placeholders are used for unsupported, pending, failed, non-local, direct archive-entry, directory collection, CB7/7z, and non-ZIP collection rows; stale thumbnail completions are rejected; direct videos are cursor positions but not still-image predecode targets; power saver suppresses only new background work.
- Expected tests/checks: Active navigation thumbnail runtime tests, thumbnail generation tests, thumbnail panel Qt/QML tests, predecode schedule/cache tests; run `devenv tasks run --mode single ci:test:cpp`.
- Suggested commit boundaries: Commit thumbnail eligibility/source-key behavior separately from predecode scheduling and QML panel behavior.
- Stop conditions: Stop before adding a video thumbnail backend dependency or broadening thumbnail generation beyond documented eligible sources.

### Milestone 8: Cleanup, Public API Narrowing, And Boundary Enforcement

- Status: Not started.
- Purpose: Remove replaced code, narrow bypass-capable public surfaces, and make architecture tests enforce the end-state boundaries.
- Dependencies: Milestones 2 through 7.
- Relevant references: `docs/architecture/state-ownership.md`, `docs/architecture/provider-rendering.md`, `docs/architecture/evolution-rules.md`, `docs/architecture/testing-strategy.md`, `docs/architecture/layer-ownership.md`.
- Likely code areas: `src/facade/kiriimagedocument*`, `src/facade/kiridocumentsession*`, `src/qml/*`, `tests/cpp/test_architectureboundaries.cpp`, `tests/cpp/test_imageshortcuts.cpp`, source manifest files, removed rendering tests.
- Acceptance criteria: QML cannot bypass session-owned navigation/deletion through leaf image document invokables; unused QML files and dead rendering files are removed; production manifests contain no replaced tile-era sources; architecture boundary tests reject owner bypasses and provider-rendering regressions; generated/build metadata remains consistent.
- Expected tests/checks: `rg` audits for removed APIs/vocabulary, architecture boundary tests, shortcut tests, qmllint, `devenv tasks run --mode single ci:lint`, and final `devenv tasks run --mode single ci:test`.
- Suggested commit boundaries: Commit public API narrowing separately from dead-file removal and test-boundary tightening.
- Stop conditions: Stop before removing public APIs still used by production QML or required test fixtures, before deleting files whose behavior has not been replaced, or before changing specs/architecture to fit implementation.

## Global Stop Conditions

- Stop before changing documented end-state behavior in `docs/spec/**` or durable boundaries in `docs/architecture/**`.
- Stop when authoritative docs conflict and implementation would require choosing one interpretation without clarification.
- Stop before destructive data behavior changes, especially deletion of directories, archive files, or remote targets, unless tests and KDE confirmation behavior prove the documented flow.
- Stop before adding, replacing, vendoring, patching, or upgrading external dependencies.
- Stop before broadening Flatpak permissions, requiring external credentials, or changing host access beyond the documented model.
- Stop before deleting valuable existing implementation unless replacement behavior and tests already prove the documented end state.
- Stop before authoring `po/*.po` content manually.
- Stop if a milestone would require changing `docs/spec/**` or `docs/architecture/**`; those changes require explicit user direction and must remain separate from this execution plan.
