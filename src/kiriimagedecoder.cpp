// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedecoder.h"

#include "apngdecoder.h"
#include "bufferedimagereader.h"
#include "heifdecoder.h"
#include "imagerendering.h"
#include "imagetilesource.h"
#include "imageviewtext.h"
#include "kiriview/src/avifcompat.cxx.h"
#include "rawdecoder.h"

#include <memory>
#include <optional>
#include <utility>

namespace {
std::optional<KiriView::DecodedImageResult> decodeSvgImageData(const QByteArray &data)
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(data, &errorString);
    if (source == nullptr) {
        return std::nullopt;
    }

    return KiriView::staticDecodedImageResult(std::move(source), {}, &errorString);
}

KiriView::DecodedImageResult openedStaticImageResult(
    const QByteArray &data, const KiriView::ImageDecodeRequest &request)
{
    QString errorString;
    std::shared_ptr<KiriView::ImageTileSource> source
        = KiriView::QImageReaderTileSource::open(data, &errorString);
    if (source == nullptr) {
        return KiriView::failedDecodedImageResult(errorString);
    }

    return KiriView::staticDecodedImageResult(
        std::move(source), request.firstDisplay(), &errorString);
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

    ApngAnimationReader apngReader;
    ApngOpenResult apngResult = apngReader.open(data);
    if (apngResult.status == ApngOpenStatus::Success) {
        return successfulDecodedImageResult(ApngAnimationImage {
            std::move(apngResult.firstFrame),
            data,
            apngResult.firstFrameDelay,
            apngResult.loopCount,
        });
    }
    if (apngResult.status == ApngOpenStatus::Error) {
        return failedDecodedImageResult(apngResult.errorString);
    }

    const QByteArray imageData = avifDataWithCompatibilityFixes(data);
    if (const std::optional<DecodedImageResult> heifResult = decodeHeifImageData(imageData)) {
        return *heifResult;
    }

    if (const std::optional<DecodedImageResult> rawResult
        = decodeRawImageData(imageData, request)) {
        return *rawResult;
    }

    BufferedImageReader reader(imageData);
    if (!reader) {
        return failedDecodedImageResult(imageViewText("Could not read the selected image data."));
    }

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
        return successfulDecodedImageResult(ReaderAnimationImage {
            std::move(firstFrame),
            imageData,
            format,
            loopCount,
            firstFrameDelay,
        });
    }
    return openedStaticImageResult(imageData, request);
}

}
