# Architecture Overview

KiriView is a KDE Kirigami image viewer built from cooperating UI, facade, runtime, and policy layers:

```mermaid
flowchart TD
    UI["QML / Kirigami UI composition"]
    Facade["C++ facade boundary"]
    Runtime["C++ Qt/KDE runtime and effect owners"]
    Policy["Rust Qt-independent policy core"]

    UI --> Facade --> Runtime --> Policy
    Policy -- "typed plans, state deltas, and effect descriptions" --> Runtime
```

The architecture keeps product policy testable without moving Qt object lifetime, KDE side effects, QML rendering objects, or authoritative runtime state into Rust. Rust computes policy from plain values. C++ owns runtime state, executes effects through Qt and KDE, rejects stale completions, and publishes coherent projections to QML.

## Dependency Direction

- QML owns declarative composition and consumes the facade as a placement and interaction surface only.
- The facade owns the QML API boundary, type conversion, and forwarding. It must not own domain workflow state.
- C++ runtime owners own Qt/KDE effects, async lifecycles, projections, and platform integration.
- Rust policy modules consume plain snapshots and return typed plans or values. They must not depend on Qt objects, call KDE adapters, or publish QML-facing state.
- Shared support domains provide explicit capability snapshots or provider ports. Runtime owners consume those ports instead of probing platform state independently.

## Component Ownership Shape

The component graph is a responsibility contract, not a complete call graph:

```mermaid
flowchart TD
    UI["UI composition"]
    Facade["Facade boundary"]
    Shell["Application shell"]
    Session["Document session"]
    Image["Image runtime"]
    Video["Video runtime"]
    Navigation["Navigation and candidate lists"]
    Presentation["Image presentation"]
    Rendering["Provider display and refinement"]
    Decoding["Image decode and metadata"]
    Collections["Opened collection access"]
    Predecode["Adjacent still-image preparation"]
    Actions["Actions, shortcuts, and UI gates"]
    System["System, localization, location, and async support"]
    Bridge["Value bridge"]
    Policy["Rust policy"]

    UI --> Facade
    Facade --> Shell
    Facade --> Session
    Shell --> Actions
    Shell --> System
    Session --> Image
    Session --> Video
    Session --> Navigation
    Session --> Predecode
    Session --> Actions
    Session --> System
    Image --> Navigation
    Image --> Presentation
    Image --> Decoding
    Image --> Collections
    Presentation --> Rendering
    Rendering --> Decoding
    Video --> Navigation
    Video --> Collections
    Navigation --> Collections
    Shell --> Bridge
    Session --> Bridge
    Image --> Bridge
    Video --> Bridge
    Navigation --> Bridge
    Presentation --> Bridge
    Rendering --> Bridge
    Decoding --> Bridge
    Collections --> Bridge
    Predecode --> Bridge
    Bridge --> Policy
```

- The document session owns top-level mixed-media routing, public source identity, active navigation projection, active zoom projection, title subject, displayed-media operation availability, displayed-media operation planning inputs, direct-media deletion follow-up, thumbnail-strip projection, and action-availability inputs.
- Image runtime owns image-mode loading, opened collection page state, presentation commands, still-image display resources, animation playback, embedded image metadata, image-mode removal fallback facts, and image-specific navigation facts.
- Video runtime owns direct-video resolution, opened-collection video source-device acceptance, playback state, video status, video zoom readout, video metadata where supported, and playback-control readiness.
- Navigation owns candidate ordering, page/media cursor state, boundary facts, live direct-media refresh, and sibling archive discovery. It exposes snapshots and plans rather than public UI state.
- Collection access owns directly opened archive and directory listing, entry-byte access, entry metadata, and eligible collection-video playback devices. It must not update document, video, thumbnail, or QML state directly.
- Provider rendering owns immutable provider display entries, display-source projections, display refinement, render-context capability inputs, and display-store memory pressure. Production image display uses provider-backed whole-image entries, not custom render nodes or visual tile scheduling.
- Decoding owns route-specific image decoding, animation frame enumeration, metadata extraction, and whole-image refinement payloads. Decoder failures preserve typed diagnostics before any user-facing projection is derived.
- Predecode owns still-image-only adjacent preparation. Video rows may be cursor positions for scheduling, but they do not produce video-frame quick-navigation payloads.
- Actions own `QAction` identity, shortcut routing, accepted UI-gate revisions, command dispatch, and unsupported-media shortcut interception. QML reports UI-local gate facts and renders action placements.

## Build and Tooling Ownership

Production native source membership must have build-validated ownership inventories for each language boundary. Build, lint, and editor tooling must consume those inventories instead of maintaining divergent source lists or compiler flags.

The production application build is the single authority for compiling production Rust, C++, generated language-boundary code, generated configuration code, QML resources, and the resulting application artifact. Test build systems may compile test-local binaries and fixtures, but they must consume production application artifacts instead of rebuilding production sources through a separate ownership path.

Compile-command metadata follows build ownership. Production compile commands must be derived from the production application build, and test compile commands must be derived from the test build that owns those test artifacts. Development-environment tooling may orchestrate refresh, merge, and editor installation, but it must not synthesize hand-authored production or test compile commands.

Development-environment modules that need Qt, CXX-Qt, build, runtime, lint, or editor metadata must consume a shared tooling context instead of duplicating wrapper commands, QML import paths, compile-command inputs, generated-include refresh logic, or Qt runtime environment snippets.
