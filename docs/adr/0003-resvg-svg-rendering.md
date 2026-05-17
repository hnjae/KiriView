# Resvg SVG Rendering

## Context

KiriView displays SVG files through the same static-image presentation and tiled rendering paths as bitmap images. The previous path used QtSvg's `QSvgRenderer` directly inside `SvgTileSource`, which made SVG rasterization a Qt runtime concern and inherited renderer gaps such as incorrect handling for some clipped content.

SVG rasterization is a rendering algorithm over source bytes, target size, and an optional viewport. It does not need to own `QObject`, `QImage`, tile scheduling, or scene graph state.

## Decision

Rust owns Qt-independent static SVG parsing and rasterization through `usvg`, `resvg`, and `tiny-skia`. The Rust bridge exposes plain byte-oriented functions for intrinsic size, full-image rasterization, and tile rasterization.

C++ continues to own `ImageTileSource`, `QImage` construction, image lifetime, tile scheduling, and integration with the existing rendering pipeline. `SvgTileSource` calls the Rust bridge and converts successful premultiplied RGBA byte buffers into `QImage` instances.

The first resvg path supports self-contained static SVG content, including clip paths. SVG script execution, animation playback, external file references, and network resource loading are not supported.

## Consequences

SVG behavior no longer depends on QtSvg renderer feature coverage for the supported static subset.

The language boundary remains value-based: Rust receives SVG bytes and integer target geometry and returns byte buffers or size values, while C++ keeps all Qt object ownership.

SVG support gains Rust unit coverage for parsing and rasterization and Qt tests for integration through the tile source.
