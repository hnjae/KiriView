// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedecoder.h"

#include "apngdecoder.h"
#include "heifdecoder.h"
#include "imagerendering.h"
#include "imageviewtext.h"
#include "kiriview/src/avifcompat.cxx.h"

#include <QBuffer>
#include <QIODevice>
#include <QImageReader>
#include <QRectF>
#include <QSvgRenderer>
#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace {
QSize svgIntrinsicSize(const QSvgRenderer &renderer)
{
    const QSize defaultSize = renderer.defaultSize();
    if (!defaultSize.isEmpty()) {
        return defaultSize;
    }

    const QRectF viewBox = renderer.viewBoxF();
    if (!viewBox.isValid()) {
        return {};
    }

    return QSize(std::max(1, static_cast<int>(std::ceil(viewBox.width()))),
        std::max(1, static_cast<int>(std::ceil(viewBox.height()))));
}

KiriView::DecodedImageResult decodedImageFailure(const QString &errorString)
{
    return KiriView::DecodedImageFailure { errorString };
}

std::optional<KiriView::DecodedImageResult> decodeSvgImageData(const QByteArray &data)
{
    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        return std::nullopt;
    }

    const QSize intrinsicSize = svgIntrinsicSize(renderer);
    if (intrinsicSize.isEmpty()) {
        return decodedImageFailure(
            KiriView::imageViewText("Could not determine the selected SVG image size."));
    }

    return KiriView::SvgDecodedImage { data, intrinsicSize };
}
}

namespace KiriView {
DecodedImageResult decodeImageData(const QByteArray &data)
{
    if (const std::optional<DecodedImageResult> svgResult = decodeSvgImageData(data)) {
        return *svgResult;
    }

    ApngDecodeResult apngResult = decodeApngAnimation(data);
    if (apngResult.status == ApngDecodeStatus::Success) {
        if (apngResult.animation.frames.empty()) {
            return decodedImageFailure(
                imageViewText("Could not decode the selected APNG animation."));
        }

        for (AnimationFrame &frame : apngResult.animation.frames) {
            frame.image = displayReadyImage(frame.image);
        }

        return DecodedAnimationImage {
            std::move(apngResult.animation.frames),
            apngResult.animation.loopCount,
        };
    }
    if (apngResult.status == ApngDecodeStatus::Error) {
        return decodedImageFailure(apngResult.errorString);
    }

    const QByteArray imageData = avifDataWithCompatibilityFixes(data);
    if (const std::optional<DecodedImageResult> heifSequenceResult
        = decodeHeifSequenceImageData(imageData)) {
        return *heifSequenceResult;
    }

    QBuffer buffer;
    buffer.setData(imageData);

    if (!buffer.open(QIODevice::ReadOnly)) {
        return decodedImageFailure(imageViewText("Could not read the selected image data."));
    }

    QImageReader reader(&buffer);
    reader.setAutoTransform(true);
    const bool supportsAnimation = reader.supportsAnimation();

    QImage image = reader.read();
    if (image.isNull()) {
        if (std::optional<DecodedImageResult> heifResult = decodeHeifStillImageData(imageData)) {
            return *heifResult;
        }
        return decodedImageFailure(reader.errorString());
    }

    const QByteArray format = reader.format();
    const int firstFrameDelay = reader.nextImageDelay();
    const int loopCount = reader.loopCount();
    const bool hasMoreFrames = reader.canRead();

    QImage firstFrame = displayReadyImage(image);
    if (supportsAnimation && hasMoreFrames) {
        return ReaderAnimationImage {
            std::move(firstFrame),
            imageData,
            format,
            loopCount,
            firstFrameDelay,
        };
    }
    return StaticDecodedImage { std::move(firstFrame) };
}

}
