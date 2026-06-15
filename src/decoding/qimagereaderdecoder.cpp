// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "qimagereaderdecoder.h"

#include "bufferedimagereader.h"
#include "localization/imageerrortext.h"
#include "location/sourcekey.h"
#include "rendering/imagerendering.h"
#include "rendering/qimagereadertilesource.h"
#include "staticimagedecode.h"

#include <QImage>
#include <memory>
#include <utility>

namespace {
QString qtRasterFailureOperationName(kiriview::DecodedImageFailureOperation operation)
{
    switch (operation) {
    case kiriview::DecodedImageFailureOperation::Unknown:
        return QStringLiteral("unknown");
    case kiriview::DecodedImageFailureOperation::OpenStaticImageSource:
        return QStringLiteral("open static image source");
    case kiriview::DecodedImageFailureOperation::DecodeFirstDisplayImage:
        return QStringLiteral("decode first display image");
    case kiriview::DecodedImageFailureOperation::DecodeBlockingDisplayImage:
        return QStringLiteral("decode blocking display image");
    }
    return QStringLiteral("unknown");
}

QString qtRasterFailureDiagnosticDetail(const QByteArray &format,
    kiriview::DecodedImageFailureOperation operation, const QString &backendError)
{
    return QStringLiteral("Qt image reader %1 failed for format %2: %3")
        .arg(qtRasterFailureOperationName(operation), QString::fromLatin1(format),
            backendError.isEmpty() ? QStringLiteral("<empty>") : backendError);
}

kiriview::DecodedImageResult failedQtRasterDecodedImageResult(QString errorString,
    kiriview::DecodedImageFailureOperation operation, const QByteArray &format,
    const QString &backendError)
{
    return kiriview::failedDecodedImageResult(kiriview::DecodedImageFailure {
        std::move(errorString),
        kiriview::DecodedImageFailureRoute::QtRaster,
        operation,
        qtRasterFailureDiagnosticDetail(format, operation, backendError),
        kiriview::DecodedImageFailureSeverity::Error,
        false,
    });
}

void stampQtRasterFailure(kiriview::DecodedImageResult &result, const QByteArray &format)
{
    kiriview::DecodedImageFailure *failure = kiriview::decodedImageResultFailure(result);
    if (failure == nullptr) {
        return;
    }
    failure->route = kiriview::DecodedImageFailureRoute::QtRaster;
    failure->diagnosticDetail
        = qtRasterFailureDiagnosticDetail(format, failure->operation, failure->diagnosticDetail);
}

kiriview::DecodedImageResult openedStaticImageResult(
    const QByteArray &data, const kiriview::ImageDecodeRequest &request, const QByteArray &format)
{
    QString errorString;
    std::shared_ptr<kiriview::ImageTileSource> source
        = kiriview::QImageReaderTileSource::open(data, format, &errorString);
    if (source == nullptr) {
        return failedQtRasterDecodedImageResult(errorString,
            kiriview::DecodedImageFailureOperation::OpenStaticImageSource, format, errorString);
    }

    kiriview::DecodedImageResult result
        = kiriview::staticDecodedImageResult(std::move(source), request, &errorString);
    stampQtRasterFailure(result, format);
    return result;
}

QString sourceIdentityForRequest(const kiriview::ImageDecodeRequest &request)
{
    return kiriview::sourceKeyForUrl(request.imageUrl()).identity;
}
}

namespace kiriview {
DecodedImageResult decodeQImageReaderImageData(
    const QByteArray &data, const ImageDecodeRequest &request, QtRasterFormat format)
{
    const QByteArray readerFormat = qtImageReaderFormat(format);
    BufferedImageReader reader(data, readerFormat);
    if (!reader) {
        return failedQtRasterDecodedImageResult(imageErrorText(ImageErrorTextId::ReadImageData),
            DecodedImageFailureOperation::OpenStaticImageSource, readerFormat,
            QStringLiteral("QImageReader could not be constructed"));
    }

    const bool supportsAnimation = reader.supportsAnimation();
    if (!supportsAnimation) {
        return openedStaticImageResult(data, request, readerFormat);
    }

    QImage image = reader.read();
    if (image.isNull()) {
        return openedStaticImageResult(data, request, readerFormat);
    }

    const QByteArray animationFormat = reader.format();
    const bool hasMoreFrames = reader.canRead();

    QImage firstFrame = displayReadyImage(image);
    if (hasMoreFrames) {
        return successfulDecodedImageResult(ReaderAnimationImage {
            std::move(firstFrame),
            data,
            animationFormat,
            {},
            sourceIdentityForRequest(request),
        });
    }
    return openedStaticImageResult(data, request, readerFormat);
}
}
