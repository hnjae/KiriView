// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "staticimagedecode.h"

#include "decoding/imagedecoderequest.h"
#include "location/sourcekey.h"
#include "rendering/imagerendering.h"

#include <QImage>
#include <utility>

namespace {
QString errorStringValue(QString* errorString)
{
    return errorString == nullptr ? QString() : *errorString;
}

kiriview::DecodedImageResult failedStaticDecodedImageResult(
    kiriview::DecodedImageFailureOperation operation, QString* errorString,
    const kiriview::ImageTileSourceDisplayDecodeDiagnostics* diagnostics = nullptr)
{
    QString message = diagnostics == nullptr ? QString() : diagnostics->userMessage();
    if (message.isEmpty()) {
        message = errorStringValue(errorString);
    }
    QString diagnosticDetail = diagnostics == nullptr ? QString() : diagnostics->diagnosticDetail();
    if (diagnosticDetail.isEmpty()) {
        diagnosticDetail = message;
    }
    if (errorString != nullptr && !message.isEmpty()) {
        *errorString = message;
    }
    return kiriview::failedDecodedImageResult(kiriview::DecodedImageFailure {
        message,
        kiriview::DecodedImageFailureRoute::Unknown,
        operation,
        diagnosticDetail,
        kiriview::DecodedImageFailureSeverity::Error,
        false,
    });
}

QString sourceIdentityForRequest(const kiriview::ImageDecodeRequest& request)
{
    return kiriview::sourceKeyForUrl(request.imageUrl()).identity;
}

qreal displayPixelsPerSourcePixel(QSize originalSize, const QImage& image)
{
    const qreal pixelsPerSourcePixel
        = kiriview::imagePixelsPerSourcePixel(originalSize, image.size());
    return pixelsPerSourcePixel > 0.0 ? pixelsPerSourcePixel : 0.0;
}

kiriview::DisplayImageQuality displayQualityForImage(
    QSize originalSize, const QImage& image, bool firstDisplay)
{
    if (firstDisplay || image.size() != originalSize) {
        return kiriview::DisplayImageQuality::FirstDisplay;
    }
    return kiriview::DisplayImageQuality::Exact;
}

kiriview::StaticDisplayImagePayload staticDisplayPayload(
    std::shared_ptr<kiriview::ImageTileSource> source, const kiriview::ImageDecodeRequest& request,
    QImage image, bool firstDisplay, qreal firstDisplayPixelsPerSourcePixel)
{
    QImage displayImage = kiriview::displayReadyImage(image);
    const QSize originalSize = source == nullptr ? QSize() : source->imageSize();
    const qreal pixelsPerSourcePixel = firstDisplayPixelsPerSourcePixel > 0.0
        ? firstDisplayPixelsPerSourcePixel
        : displayPixelsPerSourcePixel(originalSize, displayImage);
    const kiriview::DisplayImageQuality quality
        = displayQualityForImage(originalSize, displayImage, firstDisplay);
    kiriview::StaticDisplayImagePayload payload {
        sourceIdentityForRequest(request),
        source == nullptr ? kiriview::StaticImageReaderTransform {}
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

namespace kiriview {
DecodedImageResult staticDecodedImageResult(std::shared_ptr<ImageTileSource> source,
    const ImageDecodeRequest& request, QString* errorString)
{
    if (source == nullptr) {
        return failedStaticDecodedImageResult(
            DecodedImageFailureOperation::OpenStaticImageSource, errorString);
    }

    ImageTileSourceFirstDisplayDecodeResult firstDisplayResult
        = source->decodeFirstDisplayImageWithDiagnostics(request.firstDisplay());
    switch (firstDisplayResult.firstDisplay.status) {
    case FirstDisplayImageDecodeStatus::Ready:
        if (firstDisplayResult.firstDisplay.image.isNull()) {
            return failedStaticDecodedImageResult(
                DecodedImageFailureOperation::DecodeFirstDisplayImage, errorString,
                &firstDisplayResult.diagnostics);
        }
        return successfulDecodedImageResult(StaticDecodedImage {
            staticDisplayPayload(std::move(source), request,
                std::move(firstDisplayResult.firstDisplay.image), true,
                firstDisplayResult.firstDisplay.displayPixelsPerSourcePixel),
            {},
        });
    case FirstDisplayImageDecodeStatus::NotImplemented:
        break;
    case FirstDisplayImageDecodeStatus::Error:
        return failedStaticDecodedImageResult(DecodedImageFailureOperation::DecodeFirstDisplayImage,
            errorString, &firstDisplayResult.diagnostics);
    }

    ImageTileSourceDisplayDecodeResult previewResult
        = source->decodeBlockingDisplayImageWithDiagnostics(imageBlockingDisplayLongEdgeMax);
    if (previewResult.image.isNull()) {
        return failedStaticDecodedImageResult(
            DecodedImageFailureOperation::DecodeBlockingDisplayImage, errorString,
            &previewResult.diagnostics);
    }

    return successfulDecodedImageResult(StaticDecodedImage {
        staticDisplayPayload(
            std::move(source), request, std::move(previewResult.image), false, 0.0),
        {},
    });
}
}
