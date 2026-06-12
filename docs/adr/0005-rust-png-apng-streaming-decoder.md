# Rust png APNG Streaming Decoder

## Context

KiriView needs APNG playback to remain streaming and sequential: the first displayable animation frame is available before the whole animation is decoded, later frames are consumed in stream order, and APNG frame seeking is not supported.

ADR 0002 moved APNG runtime decoding to C++ through APNG-patched libpng to avoid materializing every composed frame before display. The Rust `png` crate now exposes APNG animation metadata, frame control data, and sequential raw-frame decoding, which lets KiriView keep the same streaming playback model without carrying a patched libpng build boundary.

APNG subframes still require KiriView-specific composition for blend and dispose rules, and the composed playback frame is still a Qt presentation object.

## Decision

Rust owns APNG byte-stream decoding through the `png` crate. The Rust reader keeps PNG/APNG decode state, validates that the input is APNG, exposes animation metadata, and returns raw RGBA8 APNG subframes with frame control metadata over the CXX bridge.

C++ remains the Qt presentation adapter. `ApngAnimationReader` keeps its public C++ API, opens the Rust reader, passes raw decoded subframes into `ApngFrameComposer`, and returns composed full-frame `QImage` playback frames. The existing C++ presentation path continues to own premultiplied full-frame image output, first-frame publication, looping, and timer-driven playback.

The reader skips a separate default image when APNG marks the first frame hidden, so playback starts from the first displayable animation frame.

## Consequences

KiriView no longer depends directly on APNG-patched libpng for APNG decoding. Host and Flatpak builds do not need KiriView-specific libpng link or include wiring, though Qt/KDE/runtime dependencies may still use libpng independently.

APNG memory use remains bounded by the active decoder state, raw subframe buffer, and composed canvas rather than by total frame count.

The Rust/C++ boundary carries APNG frame controls and raw RGBA8 bytes, not `QImage`. Blend and dispose composition remains covered by the existing C++ composition path.
