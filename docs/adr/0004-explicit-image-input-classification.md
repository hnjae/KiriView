# Explicit Image Input Classification

## Context

KiriView's image decode path historically tried decoder stages in a fixed order. That made Qt imageformats the final fallback for inputs that earlier decoders declined, so format recognition was split across C++ decoder code and Rust file-extension policy.

The architecture direction says byte-level format inspection belongs in Rust when it can be expressed as plain values, while C++ should own `QImage`, LibRaw, Qt/KDE runtime integration, and decoder execution.

## Decision

Rust owns image byte and file-name classification. The classifier inspects the input bytes plus the display file name and returns a plain classification: RAW, SVG, APNG, HEIF-family, Qt raster with an explicit Qt imageformat, or unknown.

C++ decode code executes the classifier result. A decoder no longer decides whether it should claim the input at routing time; it handles the input that the classifier assigned to it.

The selected decoder's failure is final. KiriView does not try another decoder as fallback after classification has assigned an input to RAW, SVG, APNG, HEIF-family, or Qt raster.

Qt imageformats are used only for inputs explicitly classified as `QtRaster(format)`, such as PNG, JPEG, GIF, WebP, BMP, ordinary TIFF, JPEG XL, or JPEG 2000. Unknown data is reported as a read/decode failure without asking Qt to probe it.

TIFF-family inputs are split before decoding. TIFF-family bytes with DNG metadata, CFA photometric metadata, known RAW maker metadata, or a supported RAW extension are classified as RAW. TIFF-family bytes without those RAW signals are classified as `QtRaster(Tiff)`.

Clear non-RAW magic bytes take precedence over file-name hints, so a PNG, JPEG, GIF, WebP, BMP, JPEG XL, JPEG 2000, APNG, HEIF-family, or SVG-like byte stream is classified from content even if the file name has a RAW extension. For TIFF-family and proprietary RAW formats with weak or varied magic bytes, a supported RAW extension is a strong RAW hint.

## Consequences

RAW files no longer fall through to Qt TIFF or other Qt imageformat handlers after LibRaw fails.

Qt raster decoding receives an explicit format instead of inferring one inside the C++ reader wrapper.

Rust unit tests cover classification policy and edge cases. C++ tests cover routing behavior, selected-decoder failure handling, and lazy HEIF compatibility transforms.
