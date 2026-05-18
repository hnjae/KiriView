// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedecoder.h"

#include "apnganimationreader.h"
#include "heifdecoder.h"
#include "kiriview/src/avifcompat.cxx.h"
#include "qimagereaderdecoder.h"
#include "rawdecoder.h"
#include "staticimagedecode.h"
#include "svgtilesource.h"

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

    return decodeQImageReaderImageData(imageData, request);
}

}
