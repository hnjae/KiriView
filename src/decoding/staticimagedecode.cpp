// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "staticimagedecode.h"

#include "decoding/imagedecoderequest.h"
#include "location/sourcekey.h"
#include "rendering/imagerendering.h"

#include <QImage>
#include <utility>

namespace {
QString errorStringValue(QString *errorString)
{
    return errorString == nullptr ? QString() : *errorString;
}

QString sourceIdentityForRequest(const KiriView::ImageDecodeRequest &request)
{
    return KiriView::sourceKeyForUrl(request.imageUrl()).identity;
}

qreal displayPixelsPerSourcePixel(const QSize &originalSize, const QImage &image)
{
    const qreal pixelsPerSourcePixel
        = KiriView::imagePixelsPerSourcePixel(originalSize, image.size());
    return pixelsPerSourcePixel > 0.0 ? pixelsPerSourcePixel : 0.0;
}

KiriView::DisplayImageQuality displayQualityForImage(
    const QSize &originalSize, const QImage &image, bool firstDisplay)
{
    if (firstDisplay || image.size() != originalSize) {
        return KiriView::DisplayImageQuality::FirstDisplay;
    }
    return KiriView::DisplayImageQuality::Exact;
}

KiriView::StaticDisplayImagePayload staticDisplayPayload(
    std::shared_ptr<KiriView::ImageTileSource> source, const KiriView::ImageDecodeRequest &request,
    QImage image, bool firstDisplay, qreal firstDisplayPixelsPerSourcePixel)
{
    QImage displayImage = KiriView::displayReadyImage(image);
    const QSize originalSize = source == nullptr ? QSize() : source->imageSize();
    const qreal pixelsPerSourcePixel = firstDisplayPixelsPerSourcePixel > 0.0
        ? firstDisplayPixelsPerSourcePixel
        : displayPixelsPerSourcePixel(originalSize, displayImage);
    const KiriView::DisplayImageQuality quality
        = displayQualityForImage(originalSize, displayImage, firstDisplay);
    KiriView::StaticDisplayImagePayload payload {
        sourceIdentityForRequest(request),
        source == nullptr ? KiriView::StaticImageReaderTransform {}
                          : source->imageReaderTransform(),
        originalSize,
        std::move(displayImage),
        quality,
        pixelsPerSourcePixel,
        {},
        std::move(source),
    };
    return payload;
}
}

namespace KiriView {
DecodedImageResult staticDecodedImageResult(std::shared_ptr<ImageTileSource> source,
    const ImageDecodeRequest &request, QString *errorString)
{
    if (source == nullptr) {
        return failedDecodedImageResult(errorStringValue(errorString));
    }

    FirstDisplayImageDecodeResult firstDisplayResult
        = source->decodeFirstDisplayImage(request.firstDisplay(), errorString);
    switch (firstDisplayResult.status) {
    case FirstDisplayImageDecodeStatus::Ready:
        if (firstDisplayResult.image.isNull()) {
            return failedDecodedImageResult(errorStringValue(errorString));
        }
        return successfulDecodedImageResult(StaticDecodedImage {
            staticDisplayPayload(std::move(source), request, std::move(firstDisplayResult.image),
                true, firstDisplayResult.displayPixelsPerSourcePixel),
            {},
        });
    case FirstDisplayImageDecodeStatus::NotImplemented:
        break;
    case FirstDisplayImageDecodeStatus::Error:
        return failedDecodedImageResult(errorStringValue(errorString));
    }

    QImage preview
        = source->decodeBlockingDisplayImage(imageBlockingDisplayLongEdgeMax, errorString);
    if (preview.isNull()) {
        return failedDecodedImageResult(errorStringValue(errorString));
    }

    return successfulDecodedImageResult(StaticDecodedImage {
        staticDisplayPayload(std::move(source), request, std::move(preview), false, 0.0),
        {},
    });
}
}
