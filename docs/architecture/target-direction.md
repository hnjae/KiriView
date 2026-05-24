# Target Direction

KiriView should evolve toward coherent Rust policy units rather than many small FFI helpers. A Rust module should represent a meaningful workflow or algorithmic domain, such as opening, navigation, deletion, zoom, spread layout, tile selection, parsing, or cache policy.

Small local branches should stay in C++ unless they are part of a larger Qt-independent policy decision. Moving a branch to Rust is useful when it clarifies ownership, strengthens tests, or lets multiple C++ runtime paths share one policy result. It is not useful when the bridge types and conversions become larger than the decision being moved.

The preferred direction is fewer, larger policy boundaries:

- Workflow reducers that accept plain events and return state deltas plus effects.
- Geometry and rendering policy modules that compute values without owning renderer objects.
- Parsing modules that inspect bytes and return plain metadata or decoded domain values.
- Cache prioritization and navigation policies that can be tested without Qt event loops and do not own Qt runtime objects.

The document-session refactor should extract coherent contracts before moving implementation. Useful boundaries include active navigation projection from session and image-document snapshots, direct media routing plans, ordinary direct media deletion fallback plans, action-availability projections, and a named still-image-only direct media predecode eligibility policy.

Those boundaries should keep `DocumentSessionState` as the public C++ owner and keep Qt/KDE effects in C++. Rust may later host pure projection or plan computation from plain snapshots, but C++ applies the resulting state, executes document routing, publishes signal batches, and preserves cursor-generation rejection for stale async completions.

When later stages add C++ or Rust files for these boundaries, keep source ownership manifests in sync, including `src/rust_policy_sources.txt`, `src/rust_bridge_sources.txt`, and relevant REUSE coverage.

C++ should remain the place where those plans are executed through Qt/KDE APIs. This keeps the application adaptable while the pre-release codebase continues to change.
