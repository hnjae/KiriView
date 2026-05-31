// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "staticimagedecode.h"

#include <QImage>
#include <utility>

namespace {
QString errorStringValue(QString *errorString)
{
    return errorString == nullptr ? QString() : *errorString;
}
}

namespace KiriView {
DecodedImageResult staticDecodedImageResult(std::shared_ptr<ImageTileSource> source,
    const ImageFirstDisplayDecodeContext &firstDisplay, QString *errorString)
{
    if (source == nullptr) {
        return failedDecodedImageResult(errorStringValue(errorString));
    }

    FirstDisplayImageDecodeResult firstDisplayResult
        = source->decodeFirstDisplayImage(firstDisplay, errorString);
    switch (firstDisplayResult.status) {
    case FirstDisplayImageDecodeStatus::Ready:
        if (firstDisplayResult.image.isNull()) {
            return failedDecodedImageResult(errorStringValue(errorString));
        }
        return successfulDecodedImageResult(StaticDecodedImage {
            StaticImagePayload { std::move(source), std::move(firstDisplayResult.image),
                StaticImageDisplayHints { firstDisplayResult.displayPixelsPerSourcePixel } },
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
        StaticImagePayload { std::move(source), std::move(preview), {} },
        {},
    });
}
}
