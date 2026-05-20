# Target Direction

KiriView should evolve toward coherent Rust policy units rather than many small FFI helpers. A Rust module should represent a meaningful workflow or algorithmic domain, such as opening, navigation, deletion, zoom, spread layout, tile selection, parsing, or cache policy.

Small local branches should stay in C++ unless they are part of a larger Qt-independent policy decision. Moving a branch to Rust is useful when it clarifies ownership, strengthens tests, or lets multiple C++ runtime paths share one policy result. It is not useful when the bridge types and conversions become larger than the decision being moved.

The preferred direction is fewer, larger policy boundaries:

- Workflow reducers that accept plain events and return state deltas plus effects.
- Geometry and rendering policy modules that compute values without owning renderer objects.
- Parsing modules that inspect bytes and return plain metadata or decoded domain values.
- Cache prioritization and navigation policies that can be tested without Qt event loops and do not own Qt runtime objects.

C++ should remain the place where those plans are executed through Qt/KDE APIs. This keeps the application adaptable while the pre-release codebase continues to change.
