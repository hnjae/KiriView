// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

// Compatibility shim: the APNG implementation lives in apngdecoder.rs because
// QImageReader's normal PNG path does not reliably expose authored APNG
// animations yet. Remove this adapter with the Rust decoder once Qt's PNG stack
// handles APNG playback itself.

#include "apngdecoder.h"

#include "imageviewtext.h"
#include "kiriview/src/apngdecoder.cxx.h"

#include <QString>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>

namespace {
KiriView::ApngDecodeResult notApng() { return {}; }

KiriView::ApngDecodeResult apngError(KiriView::RustApngErrorKind errorKind)
{
    KiriView::ApngDecodeResult result;
    result.status = KiriView::ApngDecodeStatus::Error;

    switch (errorKind) {
    case KiriView::RustApngErrorKind::Png:
        result.errorString = KiriView::imageViewText("Could not decode the selected PNG image.");
        break;
    case KiriView::RustApngErrorKind::FrameData:
    case KiriView::RustApngErrorKind::Apng:
    case KiriView::RustApngErrorKind::NoError:
        result.errorString
            = KiriView::imageViewText("Could not decode the selected APNG animation.");
        break;
    }

    return result;
}

std::optional<QImage> imageFromRgbaBytes(
    const rust::Slice<const std::uint8_t> bytes, std::uint32_t width, std::uint32_t height)
{
    if (width == 0 || height == 0
        || width > static_cast<std::uint32_t>(std::numeric_limits<int>::max())
        || height > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        return std::nullopt;
    }

    const auto expectedSize = std::uint64_t(width) * std::uint64_t(height) * 4U;
    if (expectedSize > static_cast<std::uint64_t>(std::numeric_limits<qsizetype>::max())
        || bytes.size() != expectedSize) {
        return std::nullopt;
    }

    const int imageWidth = static_cast<int>(width);
    const int imageHeight = static_cast<int>(height);
    const qsizetype bytesPerLine = qsizetype(width) * 4;
    const QImage borrowedImage(reinterpret_cast<const uchar *>(bytes.data()), imageWidth,
        imageHeight, bytesPerLine, QImage::Format_RGBA8888_Premultiplied);
    if (borrowedImage.isNull()) {
        return std::nullopt;
    }

    return borrowedImage.copy();
}
}

namespace KiriView {
ApngDecodeResult decodeApngAnimation(const QByteArray &data)
{
    rust::Box<RustApngDecodeResult> rustResult = decodeApngAnimationRust(data);
    switch (rustApngStatus(*rustResult)) {
    case RustApngDecodeStatus::NotApng:
        return notApng();
    case RustApngDecodeStatus::Error:
        return apngError(rustApngErrorKind(*rustResult));
    case RustApngDecodeStatus::Success:
        break;
    }

    const std::uint32_t width = rustApngWidth(*rustResult);
    const std::uint32_t height = rustApngHeight(*rustResult);
    const std::size_t frameCount = rustApngFrameCount(*rustResult);

    ApngDecodeResult result;
    result.status = ApngDecodeStatus::Success;
    result.animation.loopCount = rustApngLoopCount(*rustResult);
    result.animation.frames.reserve(frameCount);

    for (std::size_t index = 0; index < frameCount; ++index) {
        std::optional<QImage> image
            = imageFromRgbaBytes(rustApngFrameBytes(*rustResult, index), width, height);
        if (!image.has_value()) {
            return apngError(RustApngErrorKind::Apng);
        }
        result.animation.frames.push_back(
            AnimationFrame { std::move(*image), rustApngFrameDelay(*rustResult, index) });
    }

    return result;
}
}
