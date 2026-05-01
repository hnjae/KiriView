// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedecoder.h"

#include "kiriview/src/avifcompat.cxx.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QIODevice>
#include <QImageReader>
#include <QPainter>
#include <QRectF>
#include <QSvgRenderer>
#include <Qt>
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
    KiriView::DecodedImageResult result;
    result.errorString = errorString;
    return result;
}

std::optional<KiriView::DecodedImageResult> decodeSvgImageData(const QByteArray &data)
{
    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        return std::nullopt;
    }

    const QSize intrinsicSize = svgIntrinsicSize(renderer);
    if (intrinsicSize.isEmpty()) {
        return decodedImageFailure(QCoreApplication::translate(
            "KiriImageView", "Could not determine the selected SVG image size."));
    }

    KiriView::DecodedImageResult result;
    result.success = true;
    result.isSvg = true;
    result.svgData = data;
    result.svgIntrinsicSize = intrinsicSize;
    return result;
}
}

namespace KiriView {
QImage displayReadyImage(const QImage &image)
{
    if (image.format() == QImage::Format_RGBA8888_Premultiplied) {
        return image;
    }
    return image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
}

QSize svgRasterSize(const QSizeF &displaySize, qreal devicePixelRatio, int maximumTextureSize)
{
    if (displaySize.isEmpty() || !std::isfinite(devicePixelRatio) || devicePixelRatio <= 0.0) {
        return {};
    }

    const qreal width = displaySize.width() * devicePixelRatio;
    const qreal height = displaySize.height() * devicePixelRatio;
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0 || height <= 0.0) {
        return {};
    }

    const int maximumSize = maximumTextureSize > 0 ? maximumTextureSize : fallbackTextureSizeMax;
    const qreal scale = std::min<qreal>(1.0, std::min(maximumSize / width, maximumSize / height));
    return QSize(std::clamp(static_cast<int>(std::ceil(width * scale)), 1, maximumSize),
        std::clamp(static_cast<int>(std::ceil(height * scale)), 1, maximumSize));
}

QImage renderSvgImage(const QByteArray &data, const QSize &size)
{
    if (data.isEmpty() || size.isEmpty()) {
        return {};
    }

    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        return {};
    }

    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    if (image.isNull()) {
        return {};
    }

    image.fill(Qt::transparent);
    QPainter painter(&image);
    renderer.render(&painter, QRectF(QPointF(0.0, 0.0), QSizeF(size)));
    return image;
}

DecodedImageResult decodeImageData(const QByteArray &data)
{
    if (const std::optional<DecodedImageResult> svgResult = decodeSvgImageData(data)) {
        return *svgResult;
    }

    ApngDecodeResult apngResult = decodeApngAnimation(data);
    if (apngResult.status == ApngDecodeStatus::Success) {
        if (apngResult.animation.frames.empty()) {
            return decodedImageFailure(QCoreApplication::translate(
                "KiriImageView", "Could not decode the selected APNG animation."));
        }

        for (AnimationFrame &frame : apngResult.animation.frames) {
            frame.image = displayReadyImage(frame.image);
        }

        DecodedImageResult result;
        result.success = true;
        result.image = apngResult.animation.frames.front().image;
        result.decodedAnimationFrames = std::move(apngResult.animation.frames);
        result.animationLoopCount = apngResult.animation.loopCount;
        return result;
    }
    if (apngResult.status == ApngDecodeStatus::Error) {
        return decodedImageFailure(apngResult.errorString);
    }

    const QByteArray imageData = avifDataWithCompatibilityFixes(data);

    QBuffer buffer;
    buffer.setData(imageData);

    if (!buffer.open(QIODevice::ReadOnly)) {
        return decodedImageFailure(QCoreApplication::translate(
            "KiriImageView", "Could not read the selected image data."));
    }

    QImageReader reader(&buffer);
    reader.setAutoTransform(true);
    const bool supportsAnimation = reader.supportsAnimation();

    QImage image = reader.read();
    if (image.isNull()) {
        return decodedImageFailure(reader.errorString());
    }

    const QByteArray format = reader.format();
    const int firstFrameDelay = reader.nextImageDelay();
    const int loopCount = reader.loopCount();
    const bool hasMoreFrames = reader.canRead();

    DecodedImageResult result;
    result.success = true;
    result.image = displayReadyImage(image);
    if (supportsAnimation && hasMoreFrames) {
        result.animationData = imageData;
        result.animationFormat = format;
        result.animationLoopCount = loopCount;
        result.firstFrameDelay = firstFrameDelay;
        result.hasAnimationReaderFrames = true;
    }
    return result;
}

qsizetype imageByteCost(const QImage &image)
{
    if (image.isNull()) {
        return 0;
    }
    return image.sizeInBytes();
}

bool decodedImageResultIsPredecodeCacheable(const DecodedImageResult &result, qsizetype byteBudget)
{
    return result.success && !result.isSvg && !result.image.isNull()
        && result.decodedAnimationFrames.empty() && !result.hasAnimationReaderFrames
        && imageByteCost(result.image) <= byteBudget;
}
}
