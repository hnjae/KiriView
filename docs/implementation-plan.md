# Collection Video Playback Implementation Plan

This implementation plan is an execution queue, not a source of truth. If this file conflicts with `docs/spec/**` or `docs/architecture/**`, the spec and architecture documents win.

## Conformance Audit

Recent documentation commits define the end state for playing eligible video entries from directly opened archive collections. The current implementation is not yet conformant with that end state.

Direct video playback exists through the video source resolver and URL-based media backend. Archive and directory collection listing already classify video entries as media candidates. However, all opened-collection videos are still routed to the unsupported placeholder path, regardless of archive type, compression method, or entry eligibility.

The implementation currently lacks a media-entry-source-owned playback device contract for collection video, an eligibility boundary for stored ZIP and plain TAR video entries, a video document path that accepts an owned source device while preserving the public collection entry URL, and a session route that keeps opened-collection navigation active while the displayed document is video.

Session projection currently treats video mode as ordinary direct media. As a result, opened-collection video would not currently expose the documented archive navigation scope, archive title and page count, toolbar visibility, file-action behavior, or collection-aware Info Panel state.

Unsupported collection video behavior also needs alignment. Directory collection videos and ineligible archive videos must remain placeholders, but the current placeholder text and routing are based on the older rule that all collection videos are unsupported.

## Relevant Spec And Architecture References

- `docs/spec/video-playback.md`: direct video playback, eligible archive collection video playback, unsupported collection-video placeholders, public source URL identity, active navigation behavior, video seek shortcut boundaries, and metadata omissions.
- `docs/spec/navigation.md`: opened archive navigation membership, playable collection videos, unsupported placeholders, and background preparation limits.
- `docs/spec/file-access.md`: Delete, Open With, Copy Path, containing-folder behavior, archive KIO availability, and collection deletion fallback.
- `docs/spec/main-window.md`: toolbar visibility, Fit and zoom behavior, archive title/page count, Info Panel behavior, and thumbnail constraints.
- `docs/spec/comic-archives.md`: comic archive navigation, first supported item behavior, and Two-Page Spread boundaries around video.
- `docs/spec/scope.md`: current product boundaries for direct video, eligible archive collection video, and unsupported collection states.
- `docs/planning/current-product-boundaries.md`: current deferred scope and non-goals for collection video.
- `docs/architecture/extension-contracts.md`: media entry source adapter responsibilities, including eligible video playback source devices and lifetime boundaries.
- `docs/architecture/state-ownership.md`: C++ ownership of playback URL resolution, opened-collection source-device acquisition, source-device lifetime, metadata ownership, and supported row classes.
- `docs/architecture/workflow-shape.md`: opened collection video routing through media entry sources, separation from direct media routing, and active navigation preservation.
- `docs/architecture/terminology.md`: direct media scope, opened collection scope, video document source-device acceptance, and media entry source terminology.

## Milestone Plan

### Milestone 1: Media Entry Source Eligibility And Device Contract

Status: Complete

Dependencies: None

Likely code areas:

- `src/archive/mediaentrysourcebackend.h`
- `src/archive/mediaentrysourcebackend_karchive.cpp`
- `src/archive/mediaentrysourcebackend_libarchive.cpp`
- `src/archive/mediaentrysourcebackend_directory.cpp`
- media entry source runner, runtime, and store code
- `src/navigation/imagedocumentpagenavigationtypes.h`

Acceptance criteria:

- Media entry source APIs can represent playable collection video entries separately from unsupported video entries.
- Local CBZ/ZIP video entries are playable only when the entry is stored and uncompressed.
- Local CBT/TAR video entries are playable only when served from the plain TAR path.
- Directory collection videos, CBR/RAR, CB7/7Z, deflated ZIP video entries, and unsupported formats remain unsupported placeholders.
- The media entry source can return a playback source device for an eligible video entry without exposing that device as a direct media URL.
- The playback device lifetime remains tied to the media entry source until the video source owner clears or supersedes it.
- Existing image candidate listing and image byte loading behavior remain unchanged.

Expected tests/checks:

- Add focused failing tests first when practical.
- `test_mediaentrysourcebackend`
- `test_mediaentrysourcestore`
- Focused CTest target for media entry source behavior.

### Milestone 2: Video Document Source-Device Playback Path

Status: Complete

Dependencies: Milestone 1

Likely code areas:

- `src/video/videomediabackend.h`
- `src/video/videomediabackend.cpp`
- `src/video/videosourceloadplan*`
- `src/video/videosourceloadruntime*`
- `src/video/videodocumentruntime*`
- `src/video/kirivideodocument*`

Acceptance criteria:

- The video document can accept a media-entry-source-owned `QIODevice` plus the public collection entry URL.
- Direct video playback continues to use the existing URL resolver path.
- Collection video playback does not call the direct video resolver, does not synthesize direct media scope, and does not expose a helper URL.
- The media backend receives the source device using the Qt media source-device path.
- Collection video metadata parsing is skipped.
- Clearing, replacing, or destroying the video document releases the prior collection video device owner.
- Direct video behavior and tests remain unchanged.

Expected tests/checks:

- Add focused failing tests first when practical.
- `test_videomediabackend`
- `test_videosourceloadruntime`
- `test_videosourceloadplan`
- `test_videodocumentruntime`
- `test_kirivideodocument`

### Milestone 3: Opened Collection Video Handoff

Status: Pending

Dependencies: Milestones 1 and 2

Likely code areas:

- `src/document/imageloadsessiontracker*`
- `src/document/imageloader*`
- `src/document/imageopenworkflow*`
- `src/document/imageopencontroller*`
- `src/session/documentsessionrouteplan*`
- `src/session/documentsessionrouteruntime*`
- `src/session/documentsessionruntime*`
- session document ports and source attachment code

Acceptance criteria:

- Opening an archive whose first supported item is an eligible collection video enters video mode and starts playback.
- Navigating from an archive image to an eligible collection video enters video mode and starts playback.
- The public video source URL remains the collection entry URL.
- The opened collection scope and page row remain active after the handoff.
- The handoff does not clear or replace the opened collection with an ordinary direct media scope.
- Ineligible archive videos and directory collection videos still route to the unsupported placeholder path.
- Existing direct image, archive image, and direct video workflows remain unchanged.

Expected tests/checks:

- Add focused failing tests first when practical.
- `test_imageloader`
- `test_imageopenworkflow`
- `test_documentsessionrouteplan`
- `test_documentsessionrouteruntime`
- `test_kiridocumentsession`

### Milestone 4: Session Projection, Navigation, Toolbar, And Title State

Status: Pending

Dependencies: Milestone 3

Likely code areas:

- `src/session/documentsessionpublicprojection*`
- `src/session/documentsessionpublicleafsnapshotbuilder*`
- `src/session/windowtitleprojection*`
- `src/session/documentsessionactivezoom*`
- `src/application/applicationactionstatepolicy*`
- `src/application/applicationcommandrouter*`
- `src/qml/Main.qml`
- toolbar and video viewport QML bindings

Acceptance criteria:

- While an eligible collection video is active, active navigation remains the opened archive collection.
- Previous, next, first, last, page-number, and thumbnail activation use the opened collection row set.
- Direct media navigation is not active for collection video.
- The window title uses the archive title and current/total position when the active page position is known.
- Right-to-left reading and Two-Page Spread control visibility follows opened archive toolbar rules.
- Two-Page Spread never pairs a video page.
- Fit is disabled and zoom is read-only for video mode.
- Direct video session projection remains ordinary direct media.

Expected tests/checks:

- Add focused failing tests first when practical.
- `test_documentsessionpublicprojection`
- `test_kiridocumentsession`
- `test_mainwindowtoolbar`
- `test_mainwindowvideointegration`
- `devenv tasks run --mode single ci:lint:qml` if QML files change.

### Milestone 5: Unsupported Collection Video Placeholder Conformance

Status: Pending

Dependencies: Milestone 3

Likely code areas:

- `src/document/imageopencontroller*`
- `src/document/imageopenworkflow*`
- `src/document/imageloader*`
- session public projection and placeholder state code
- `src/qml/MediaViewportHost.qml`

Acceptance criteria:

- Directory collection videos show the documented unsupported placeholder.
- Ineligible archive videos show the documented unsupported placeholder.
- The placeholder text is exactly `KiriView can’t play this video from the selected collection.`
- Unsupported collection video entries remain navigation positions inside the opened collection.
- Unsupported collection video entries do not trigger video frame preparation or metadata parsing.
- Existing unsupported placeholder behavior for non-video unsupported media remains unchanged.

Expected tests/checks:

- Add focused failing tests first when practical.
- `test_imageloader`
- `test_imageopenworkflow`
- `test_kiridocumentsession`
- `test_mainwindowvideointegration`

### Milestone 6: File Actions, Info Panel, And Deletion

Status: Pending

Dependencies: Milestones 4 and 5

Likely code areas:

- `src/session/mediaopenwithplan*`
- `src/session/mediainformationprojection*`
- `src/session/documentsessionmediadeletionplan*`
- `src/session/documentsessionruntime*`
- session public leaf snapshot and file-action projection code

Acceptance criteria:

- Open With targets the collection entry URL for playable collection video when the archive scheme is supported.
- Open With remains unavailable for unsupported archive schemes and unsupported placeholders according to the file-access spec.
- Copy Path uses the collection entry URL for playable collection video and unsupported collection video placeholders.
- Info Panel keeps video identity for playable collection video but omits collection-internal video metadata.
- Info Panel path presentation is collection-aware rather than treating the helper source as a direct local media file.
- Delete for playable collection video targets the opened collection container, not the internal video entry.
- Deleting a playable collection video stops playback and follows documented collection deletion fallback behavior.
- Direct video file actions remain unchanged.

Expected tests/checks:

- Add focused failing tests first when practical.
- `test_mediaopenwithplan`
- `test_mediainformationprojection`
- deletion-related session tests
- `test_kiridocumentsession`

### Milestone 7: Integration And Regression Verification

Status: Pending

Dependencies: Milestones 1 through 6

Likely code areas:

- Cross-module tests and fixtures
- Any CMake test manifests touched by prior milestones
- QML lint surface if prior milestones touched QML

Acceptance criteria:

- Direct image, direct video, archive image, eligible archive collection video, ineligible archive collection video, and directory collection video workflows all pass.
- Stored ZIP and plain TAR playable video paths are covered.
- Deflated ZIP, directory, CBR/RAR, and CB7/7Z unsupported paths are covered.
- Active navigation, title, toolbar, Info Panel, Open With, Copy Path, and Delete behavior match the authoritative docs.
- No tests assert unrelated repository artifacts outside the ownership boundary of the code under test.
- Final verification succeeds.

Expected tests/checks:

- Focused CTest targets affected by the implementation.
- `devenv tasks run --mode single ci:lint:cpp` if C++ lint surface changed.
- `devenv tasks run --mode single ci:lint:qml` if QML files changed.
- `devenv tasks run --mode single ci:test`

## Cross-Milestone Dependencies

- Milestone 2 depends on the source-device contract from Milestone 1.
- Milestone 3 depends on Milestones 1 and 2 because handoff needs both eligibility and video device playback.
- Milestone 4 depends on Milestone 3 because session projection must observe the real collection video route.
- Milestone 5 depends on Milestone 3 because unsupported and playable collection video routing must be distinguished first.
- Milestone 6 depends on Milestones 4 and 5 because file actions and Info Panel need accurate session scope and placeholder state.
- Milestone 7 depends on all prior milestones.

## Stop Conditions

Stop and ask before proceeding if any of these conditions occur:

- `docs/spec/**` and `docs/architecture/**` conflict on behavior that affects implementation.
- The implementation would require editing `docs/spec/**` or `docs/architecture/**`.
- KArchive, Qt, or the existing media stack cannot expose the required stored-ZIP, plain-TAR, or source-device behavior without adding, replacing, vendoring, or patching external dependencies.
- The straightforward solution would require private upstream APIs, KIOFuse, temporary extraction files, helper URL exposure, or reuse of the direct video resolver for opened collection video.
- Work would modify upstream, third-party, generated, vendored, packaged dependency sources, or `po/*.po` files.
- Work would require destructive file or git operations, deleting real user files, or changing repository history.
- Verification would require external credentials, network-only samples, privileged portals, GUI permissions beyond normal local test execution, or host codec assumptions not already covered by the project test environment.
- A milestone exposes an unresolved ambiguity around toolbar action enablement, delete fallback after collection video deletion, or navigation scope conflicts between spec files.
