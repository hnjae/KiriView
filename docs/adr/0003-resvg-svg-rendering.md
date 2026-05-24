# Resvg SVG Rendering

## Context

KiriView displays SVG files through the same static-image presentation and tiled rendering paths as bitmap images. The previous path used QtSvg's `QSvgRenderer` directly inside `SvgTileSource`, which made SVG rasterization a Qt runtime concern and inherited renderer gaps such as incorrect handling for some clipped content.

SVG rasterization is a rendering algorithm over source bytes, target size, and an optional viewport. It does not need to own `QObject`, `QImage`, tile scheduling, or scene graph state.

## Decision

Rust owns Qt-independent static SVG parsing and rasterization through `usvg`, `resvg`, and `tiny-skia`. The Rust bridge exposes plain byte-oriented functions for intrinsic size, full-image rasterization, and tile rasterization.

C++ continues to own `ImageTileSource`, `QImage` construction, image lifetime, tile scheduling, and integration with the existing rendering pipeline. `SvgTileSource` calls the Rust bridge and converts successful premultiplied RGBA byte buffers into `QImage` instances.

SVG sources are resolution-independent tile sources. They must remain on the static tile surface even when the preview bitmap is small enough to fit a full-image surface, because the preview is only an immediate placeholder and not the final rasterization for every zoom level or device pixel ratio.

The SVG tile planner renders from oversampled raster scale buckets instead of the intrinsic bitmap pyramid used by raster sources. The current physical display scale is computed from display size, device pixel ratio, and intrinsic SVG size. The planner selects the smallest bucket scale that is strictly larger than the current need from a 1.5x geometric sequence such as `1/2.25`, `1/1.5`, `1.0`, `1.5`, `2.25`, and `3.375`. Tile requests inside a bucket use bucket raster coordinates, while decoded tiles carry the intrinsic SVG source rect used for final placement. This lets KiriView reuse SVG tiles across small zoom changes and request new tiles only when the view crosses a bucket boundary.

The render path treats the planner's selected layer as an explicit compositing contract. Raster image tiles composite only from the active bitmap pyramid level for the current display scale. SVG tiles composite only from the active SVG raster scale bucket for the current display scale. Cached decoded tiles, pending tiles, and failed tile records from other raster levels or SVG buckets may be retained for reuse, but they must not be drawn over the current view.

Rendering should move toward a single render-frame projection for each displayed page role. The projection input is the displayed surface identity and revision, display size, visible item rect, device pixel ratio, rotation, and page role. The projection output includes the active tile layer, draw entries for the current frame, and missing tile requests for that same frame. This keeps compositing and decode scheduling tied to the same snapshot instead of allowing a stale draw list to survive after zoom, pan, rotation, or render-context changes.

The scene graph render node consumes the projected draw entries and updates GPU resources from their identities and geometry. It must not independently choose SVG scale buckets, search cached tiles, or recompute draw policy from the surface. The decoded tile cache may retain other buckets for future reuse, but those cached tiles can enter the scene only through a later render-frame projection whose active layer selects them.

SVG tile decode may use integer raster texture rectangles within a bucket, but each decoded tile also carries the exact fractional intrinsic SVG coverage that the bucket tile represents. Scene placement uses that intrinsic coverage rather than deriving display bounds only from integer texture dimensions, so adjacent SVG tiles meet without visible gaps or overlaps at very high zoom.

SVG previews are placeholders, not fallback detail layers. They may remain visible while current-detail SVG tiles for the active bucket are missing, but stale lower-resolution bucket tiles must not substitute for the active bucket after zoom, viewport, rotation, or device-pixel-ratio changes. After zooming far out and then back in, the visible SVG area must eventually return to current-detail rendering, and stale lower-detail SVG tiles must stop being composited once the active layer is known.

Tile cache keys include the raster scale bucket. Raster image tiles use the default bucket identifier, preserving the existing bitmap behavior, while SVG tiles use the selected bucket identifier to keep different rasterizations of the same logical tile from colliding in pending, failed, and decoded tile state.

The first resvg path supports self-contained static SVG content, including clip paths. SVG script execution, animation playback, external file references, and network resource loading are not supported.

## Consequences

SVG behavior no longer depends on QtSvg renderer feature coverage for the supported static subset.

The language boundary remains value-based: Rust receives SVG bytes and integer target geometry and returns byte buffers or size values, while C++ keeps all Qt object ownership.

SVG support gains Rust unit coverage for parsing and rasterization and Qt tests for integration through the tile source, presentation surface selection, scale bucket tile planning, render-frame invalidation, active-layer draw filtering, and render-context bucket recomputation.
