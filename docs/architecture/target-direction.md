# Policy Boundary Contracts

KiriView uses coherent Rust policy units rather than many small FFI entry points. A Rust module must represent a meaningful workflow or algorithmic domain, such as opening, navigation, deletion, zoom, spread layout, display bucket selection, parsing, or cache policy.

Small local branches stay in C++ unless they are part of a larger Qt-independent policy decision. Rust policy boundaries must clarify ownership, preserve value-based tests, or let multiple C++ runtime paths share one policy result. A branch must not cross the language boundary when the bridge types and conversions are larger than the decision being represented.

Policy boundaries must be fewer and larger:

- Workflow reducers that accept plain events and return state deltas plus effects.
- Geometry and rendering policy modules that compute values without owning renderer objects.
- Parsing modules that inspect bytes and return plain metadata or decoded domain values.
- Cache prioritization and navigation policies that can be tested without Qt event loops and do not own Qt runtime objects.

Document-session language-boundary changes must cross coherent contracts rather than individual property-shaped adapters. Stable boundaries include active navigation projection from session and image-document snapshots, direct media routing plans, ordinary direct media deletion fallback plans, action-availability projections, and a named still-image-only direct media predecode eligibility policy.

Session-to-leaf document boundaries expose cohesive snapshot families and explicit command/effect ports, not property-shaped signal bags or direct leaf facade queries. Projection refresh, thumbnail-source adaptation, routing, page navigation, deletion, playback stop, and video-output attachment must cross the session boundary through typed ports whose ownership is visible at the boundary.

Document-session subowners must be introduced only when a concern needs its own lifecycle, stale-completion checks, projection commit order, or grouped command/effect ports. Route planning, active-navigation dispatch, public projection publication, leaf synchronization, direct-media navigation, deletion completion, and video-output attachment may be split into named collaborators, but their public output must still go through the document-session state owner.

Session runtime code may supply committed snapshots and bind ports, but it must not become a second owner for projection commits, leaf-to-public mapping, route-local sequencing, or stale-completion acceptance. These boundaries stay stable at the contract level across internal refactoring that preserves the same ownership, port, and stale-completion contracts.

Viewport scan-start state, pan/scan command planning, anchored zoom positioning, viewport point-query geometry, and viewport command projection application belong to the image-document runtime/presentation layer, not QML. Facade and non-rendering viewport adapters may forward UI commands and physical viewport observations, but C++ owns command revision tracking, begin/complete/acknowledge ordering, and observation revision reconciliation.

Those boundaries keep the document-session state owner as the public C++ owner and keep Qt/KDE effects in C++. Rust hosts pure projection or plan computation only from plain snapshots; C++ applies the resulting state, executes document routing, publishes signal batches, and preserves cursor-generation rejection for stale async completions.

C++ remains the place where those plans are executed through Qt/KDE APIs.
