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
        return kiriview::failedDecodedImageResult(errorString);
    }

    return kiriview::staticDecodedImageResult(std::move(source), request, &errorString);
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
        return kiriview::failedDecodedImageResult(openResult.errorString);
    }

    QString errorString;
    std::optional<kiriview::AnimationFrame> firstFrame = reader.readNextFrame(&errorString);
    if (!firstFrame.has_value()) {
        if (errorString.isEmpty()) {
            errorString = kiriview::heifSequenceDecodeErrorString();
        }
        return kiriview::failedDecodedImageResult(errorString);
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
