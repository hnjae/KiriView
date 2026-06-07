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
std::optional<KiriView::DecodedImageResult> decodeHeifStillImageDataForInfo(const QByteArray &data,
    const KiriView::HeifContainerInfo &info, const KiriView::ImageDecodeRequest &request)
{
    if (!info.stillImage) {
        return std::nullopt;
    }

    QString errorString;
    std::shared_ptr<KiriView::ImageTileSource> source
        = KiriView::openHeifTileSource(data, &errorString);
    if (source == nullptr) {
        return KiriView::failedDecodedImageResult(errorString);
    }

    return KiriView::staticDecodedImageResult(std::move(source), request, &errorString);
}

std::optional<KiriView::DecodedImageResult> decodeHeifSequenceImageDataForInfo(
    const QByteArray &data, const KiriView::HeifContainerInfo &info,
    const KiriView::ImageDecodeRequest &request)
{
    if (!info.isHeif()) {
        return std::nullopt;
    }

    KiriView::HeifSequenceReader reader;
    const KiriView::HeifSequenceOpenResult openResult = reader.open(data);
    if (openResult.status == KiriView::HeifSequenceOpenStatus::NotHeif
        || openResult.status == KiriView::HeifSequenceOpenStatus::NotSequence) {
        return std::nullopt;
    }
    if (openResult.status == KiriView::HeifSequenceOpenStatus::Error) {
        return KiriView::failedDecodedImageResult(openResult.errorString);
    }

    QString errorString;
    std::optional<KiriView::AnimationFrame> firstFrame = reader.readNextFrame(&errorString);
    if (!firstFrame.has_value()) {
        if (errorString.isEmpty()) {
            errorString = KiriView::heifSequenceDecodeErrorString();
        }
        return KiriView::failedDecodedImageResult(errorString);
    }

    return KiriView::successfulDecodedImageResult(KiriView::HeifSequenceAnimationImage {
        std::move(firstFrame->image),
        data,
        {},
        KiriView::sourceKeyForUrl(request.imageUrl()).identity,
    });
}
}

namespace KiriView {
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
