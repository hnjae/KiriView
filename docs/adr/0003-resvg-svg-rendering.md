# Resvg SVG Rendering

## Context

KiriView displays SVG files through the same static-image presentation and tiled rendering paths as bitmap images. The previous path used QtSvg's `QSvgRenderer` directly inside `SvgTileSource`, which made SVG rasterization a Qt runtime concern and inherited renderer gaps such as incorrect handling for some clipped content.

SVG rasterization is a rendering algorithm over source bytes, target size, and an optional viewport. It does not need to own `QObject`, `QImage`, tile scheduling, or scene graph state.

## Decision

Rust owns Qt-independent static SVG parsing and rasterization through `usvg`, `resvg`, and `tiny-skia`. The Rust bridge exposes plain byte-oriented functions for intrinsic size, full-image rasterization, and tile rasterization.

C++ continues to own `ImageTileSource`, `QImage` construction, image lifetime, tile scheduling, and integration with the existing rendering pipeline. `SvgTileSource` calls the Rust bridge and converts successful premultiplied RGBA byte buffers into `QImage` instances.

SVG sources are resolution-independent tile sources. They must remain on the static tile surface even when the preview bitmap is small enough to fit a full-image surface, because the preview is only an immediate placeholder and not the final rasterization for every zoom level or device pixel ratio.

The SVG tile planner renders from oversampled raster scale buckets instead of the intrinsic bitmap pyramid used by raster sources. The current physical display scale is computed from display size, device pixel ratio, and intrinsic SVG size. The planner selects the smallest bucket scale that is strictly larger than the current need from a 1.5x geometric sequence such as `1/2.25`, `1/1.5`, `1.0`, `1.5`, `2.25`, and `3.375`. Tile requests inside a bucket use bucket raster coordinates, while decoded tiles carry the intrinsic SVG source rect used for final placement. This lets KiriView reuse SVG tiles across small zoom changes and request new tiles only when the view crosses a bucket boundary.

Tile cache keys include the raster scale bucket. Raster image tiles use the default bucket identifier, preserving the existing bitmap behavior, while SVG tiles use the selected bucket identifier to keep different rasterizations of the same logical tile from colliding in pending, failed, and decoded tile state.

The first resvg path supports self-contained static SVG content, including clip paths. SVG script execution, animation playback, external file references, and network resource loading are not supported.

## Consequences

SVG behavior no longer depends on QtSvg renderer feature coverage for the supported static subset.

The language boundary remains value-based: Rust receives SVG bytes and integer target geometry and returns byte buffers or size values, while C++ keeps all Qt object ownership.

SVG support gains Rust unit coverage for parsing and rasterization and Qt tests for integration through the tile source, presentation surface selection, and scale bucket tile planning.
