// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedecoder.h"

#include "apngdecoder.h"
#include "heifdecoder.h"
#include "imagerendering.h"
#include "imagetilesource.h"
#include "imageviewtext.h"
#include "kiriview/src/avifcompat.cxx.h"

#include <QBuffer>
#include <QIODevice>
#include <QImageReader>
#include <memory>
#include <optional>
#include <utility>

namespace {
KiriView::DecodedImageResult decodedImageFailure(const QString &errorString)
{
    return KiriView::DecodedImageFailure { errorString };
}

std::optional<KiriView::DecodedImageResult> decodeSvgImageData(const QByteArray &data)
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(data, &errorString);
    if (source == nullptr) {
        return std::nullopt;
    }

    QImage preview = source->decodeBlockingDisplayImage(
        KiriView::imageBlockingDisplayLongEdgeMax, &errorString);
    if (preview.isNull()) {
        return decodedImageFailure(errorString);
    }

    return KiriView::StaticDecodedImage { std::move(source), std::move(preview), {} };
}

KiriView::DecodedImageResult openedStaticImageResult(
    const QByteArray &data, const KiriView::ImageDecodeRequest &request)
{
    QString errorString;
    std::shared_ptr<KiriView::ImageTileSource> source
        = KiriView::QImageReaderTileSource::open(data, &errorString);
    if (source == nullptr) {
        return decodedImageFailure(errorString);
    }

    const KiriView::FirstDisplayImageDecodeResult firstDisplay
        = source->decodeFirstDisplayImage(request.firstDisplay, &errorString);
    if (firstDisplay.status == KiriView::FirstDisplayImageDecodeStatus::Ready) {
        if (firstDisplay.image.isNull()) {
            return decodedImageFailure(errorString);
        }

        return KiriView::StaticDecodedImage { std::move(source), std::move(firstDisplay.image),
            KiriView::StaticImageDisplayHints { firstDisplay.displayPixelsPerSourcePixel } };
    }
    if (firstDisplay.status == KiriView::FirstDisplayImageDecodeStatus::Error) {
        return decodedImageFailure(errorString);
    }

    QImage preview = source->decodeBlockingDisplayImage(
        KiriView::imageBlockingDisplayLongEdgeMax, &errorString);
    if (preview.isNull()) {
        return decodedImageFailure(errorString);
    }

    return KiriView::StaticDecodedImage { std::move(source), std::move(preview), {} };
}
}

namespace KiriView {
DecodedImageResult decodeImageData(const QByteArray &data)
{
    return decodeImageData(data, ImageDecodeRequest {});
}

DecodedImageResult decodeImageData(const QByteArray &data, const ImageDecodeRequest &request)
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
    if (const std::optional<DecodedImageResult> heifStillResult
        = decodeHeifStillImageData(imageData)) {
        return *heifStillResult;
    }

    QBuffer buffer;
    buffer.setData(imageData);

    if (!buffer.open(QIODevice::ReadOnly)) {
        return decodedImageFailure(imageViewText("Could not read the selected image data."));
    }

    QImageReader reader(&buffer);
    reader.setAutoTransform(true);
    const bool supportsAnimation = reader.supportsAnimation();

    if (!supportsAnimation) {
        return openedStaticImageResult(imageData, request);
    }

    QImage image = reader.read();
    if (image.isNull()) {
        return openedStaticImageResult(imageData, request);
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
    return openedStaticImageResult(imageData, request);
}

}
