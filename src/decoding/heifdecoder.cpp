// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

// Compatibility shim: KiriView decodes selected HEIF data through libheif
// because current KImageFormats/QImageReader support does not reliably cover
// every HEIF variant KiriView supports. HEIF image sequences use
// HeifSequenceReader so they play as authored animations with frames, delays,
// alpha, and supported JPEG/JPEG 2000/AVC/HEVC/VVC sequence codecs. Remove the
// sequence reader and route HEIF sequences through QImageReader once
// KImageFormats reliably exposes those semantics. Re-evaluate the still-image
// fallback separately once KImageFormats covers the still HEIF variants KiriView
// supports.

#include "heifdecoder.h"

#include "heifcontainer.h"
#include "heifsequencereader.h"
#include "heifsupport.h"
#include "imagedecoderequest.h"
#include "location/sourcekey.h"
#include "rendering/heiftilesource.h"
#include "staticimagedecode.h"

#include <memory>
#include <optional>
#include <utility>

namespace {
QString heifFailureOperationName(kiriview::DecodedImageFailureOperation operation)
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
    case kiriview::DecodedImageFailureOperation::DecodeAnimationOpen:
        return QStringLiteral("decode animation open");
    case kiriview::DecodedImageFailureOperation::DecodeRawImage:
        return QStringLiteral("decode raw image");
    case kiriview::DecodedImageFailureOperation::DecodeHeifSequenceOpen:
        return QStringLiteral("decode HEIF sequence open");
    case kiriview::DecodedImageFailureOperation::DecodeHeifSequenceFrame:
        return QStringLiteral("decode HEIF sequence frame");
    }
    return QStringLiteral("unknown");
}

QString heifFailureDiagnosticDetail(
    kiriview::DecodedImageFailureOperation operation, const QString &backendError)
{
    return QStringLiteral("HEIF decoder %1 failed: %2")
        .arg(heifFailureOperationName(operation),
            backendError.isEmpty() ? QStringLiteral("<empty>") : backendError);
}

kiriview::DecodedImageResult failedHeifDecodedImageResult(
    QString errorString, kiriview::DecodedImageFailureOperation operation)
{
    const QString backendError = errorString;
    return kiriview::failedDecodedImageResult(kiriview::DecodedImageFailure {
        std::move(errorString),
        kiriview::DecodedImageFailureRoute::HeifFamily,
        operation,
        heifFailureDiagnosticDetail(operation, backendError),
        kiriview::DecodedImageFailureSeverity::Error,
        false,
    });
}

void stampHeifFailure(kiriview::DecodedImageResult &result)
{
    kiriview::DecodedImageFailure *failure = kiriview::decodedImageResultFailure(result);
    if (failure == nullptr) {
        return;
    }
    failure->route = kiriview::DecodedImageFailureRoute::HeifFamily;
    failure->diagnosticDetail
        = heifFailureDiagnosticDetail(failure->operation, failure->diagnosticDetail);
}

std::optional<kiriview::DecodedImageResult> decodeHeifStillImageDataForInfo(const QByteArray &data,
    const kiriview::HeifContainerInfo &info, const kiriview::ImageDecodeRequest &request)
{
    if (!info.stillImage) {
        return std::nullopt;
    }

    QString errorString;
    std::shared_ptr<kiriview::ImageTileSource> source
        = kiriview::openHeifTileSource(data, &errorString);
    if (source == nullptr) {
        return failedHeifDecodedImageResult(
            errorString, kiriview::DecodedImageFailureOperation::OpenStaticImageSource);
    }

    kiriview::DecodedImageResult result
        = kiriview::staticDecodedImageResult(std::move(source), request, &errorString);
    stampHeifFailure(result);
    return result;
}

std::optional<kiriview::DecodedImageResult> decodeHeifSequenceImageDataForInfo(
    const QByteArray &data, const kiriview::HeifContainerInfo &info,
    const kiriview::ImageDecodeRequest &request)
{
    if (!info.isHeif()) {
        return std::nullopt;
    }

    kiriview::HeifSequenceReader reader;
    const kiriview::HeifSequenceOpenResult openResult = reader.open(data);
    if (openResult.status == kiriview::HeifSequenceOpenStatus::NotHeif
        || openResult.status == kiriview::HeifSequenceOpenStatus::NotSequence) {
        return std::nullopt;
    }
    if (openResult.status == kiriview::HeifSequenceOpenStatus::Error) {
        return failedHeifDecodedImageResult(
            openResult.errorString, kiriview::DecodedImageFailureOperation::DecodeHeifSequenceOpen);
    }

    QString errorString;
    std::optional<kiriview::AnimationFrame> firstFrame = reader.readNextFrame(&errorString);
    if (!firstFrame.has_value()) {
        if (errorString.isEmpty()) {
            errorString = kiriview::heifSequenceDecodeErrorString();
        }
        return failedHeifDecodedImageResult(
            errorString, kiriview::DecodedImageFailureOperation::DecodeHeifSequenceFrame);
    }

    return kiriview::successfulDecodedImageResult(kiriview::HeifSequenceAnimationImage {
        std::move(firstFrame->image),
        data,
        {},
        kiriview::sourceKeyForUrl(request.imageUrl()).identity,
    });
}
}

namespace kiriview {
std::optional<DecodedImageResult> decodeHeifStillImageData(const QByteArray &data)
{
    return decodeHeifStillImageData(data, {});
}

std::optional<DecodedImageResult> decodeHeifStillImageData(
    const QByteArray &data, const ImageDecodeRequest &request)
{
    return decodeHeifStillImageDataForInfo(data, heifContainerInfo(data), request);
}

std::optional<DecodedImageResult> decodeHeifSequenceImageData(const QByteArray &data)
{
    return decodeHeifSequenceImageDataForInfo(data, heifContainerInfo(data), {});
}

std::optional<DecodedImageResult> decodeHeifImageData(const QByteArray &data)
{
    return decodeHeifImageData(data, {});
}

std::optional<DecodedImageResult> decodeHeifImageData(
    const QByteArray &data, const ImageDecodeRequest &request)
{
    const HeifContainerInfo info = heifContainerInfo(data);
    if (std::optional<DecodedImageResult> sequenceResult
        = decodeHeifSequenceImageDataForInfo(data, info, request)) {
        return sequenceResult;
    }

    return decodeHeifStillImageDataForInfo(data, info, request);
}
}
