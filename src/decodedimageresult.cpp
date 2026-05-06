// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimageresult.h"

#include <utility>
#include <variant>

namespace {
QString errorStringValue(QString *errorString)
{
    return errorString == nullptr ? QString() : *errorString;
}
}

namespace KiriView {
DecodedImageResult failedDecodedImageResult(QString errorString)
{
    return DecodedImageFailure { std::move(errorString) };
}

DecodedImageResult successfulDecodedImageResult(DecodedImage image)
{
    return std::visit(
        [](auto &&decoded) -> DecodedImageResult {
            return DecodedImageResult { std::forward<decltype(decoded)>(decoded) };
        },
        std::move(image));
}

DecodedImageResult staticDecodedImageResult(std::shared_ptr<ImageTileSource> source,
    const ImageFirstDisplayDecodeContext &firstDisplay, QString *errorString)
{
    if (source == nullptr) {
        return failedDecodedImageResult(errorStringValue(errorString));
    }

    FirstDisplayImageDecodeResult firstDisplayResult
        = source->decodeFirstDisplayImage(firstDisplay, errorString);
    if (firstDisplayResult.status == FirstDisplayImageDecodeStatus::Ready) {
        if (firstDisplayResult.image.isNull()) {
            return failedDecodedImageResult(errorStringValue(errorString));
        }

        return successfulDecodedImageResult(StaticDecodedImage {
            StaticImagePayload { std::move(source), std::move(firstDisplayResult.image),
                StaticImageDisplayHints { firstDisplayResult.displayPixelsPerSourcePixel } },
        });
    }
    if (firstDisplayResult.status == FirstDisplayImageDecodeStatus::Error) {
        return failedDecodedImageResult(errorStringValue(errorString));
    }

    QImage preview
        = source->decodeBlockingDisplayImage(imageBlockingDisplayLongEdgeMax, errorString);
    if (preview.isNull()) {
        return failedDecodedImageResult(errorStringValue(errorString));
    }

    return successfulDecodedImageResult(StaticDecodedImage {
        StaticImagePayload { std::move(source), std::move(preview), {} },
    });
}

const DecodedImageFailure *decodedImageResultFailure(const DecodedImageResult &result)
{
    return std::get_if<DecodedImageFailure>(&result);
}
}
