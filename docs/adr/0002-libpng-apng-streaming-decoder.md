# Libpng APNG Streaming Decoder

## Context

The original APNG shim parsed APNG chunks in Rust, decoded every frame, composed every full-canvas image, and returned all frames to C++ at load completion. That kept APNG policy easy to unit test, but it made memory use scale with canvas size times frame count before the first frame could be presented.

APNG playback is inherently sequential: later frames are read in stream order and may depend on prior composition state through blend and dispose operations.

## Decision

C++ owns APNG runtime decoding through APNG-patched libpng. `ApngAnimationReader` keeps the original source bytes and libpng read state, while `ApngFrameComposer` owns the composed canvas, decoded frame buffer, first-displayed-frame composition rule, and any temporary restore region needed for `dispose=previous`.

Image loading opens the reader only far enough to decode the first displayable frame and returns a streamed APNG decoded-image variant. The animation player reopens the reader for playback, consumes the already displayed first frame, and then advances frames one at a time on timer ticks. Looping restarts by reopening the reader and replaying from the beginning.

The Rust APNG parser and the Rust `png` crate dependency are removed. Rust stays available for byte-level format inspection and policy, but APNG decoding depends on libpng state, `QImage`, and Qt animation lifetime, so it belongs in the C++ runtime layer.

## Consequences

APNG memory use no longer scales with the total number of frames. Startup still decodes the first displayable frame before replacing the current image, and playback keeps only the active stream state.

APNG frame seeking is not supported. This matches libpng's stream model and KiriView's current playback needs.

Host and Flatpak builds must link against libpng with APNG read support. Builds fail at compile time when `PNG_APNG_SUPPORTED` or `PNG_READ_APNG_SUPPORTED` is missing.
