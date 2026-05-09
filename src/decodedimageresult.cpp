// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimageresult.h"

#include "kiriview/src/decodedimageresult.cxx.h"

#include <optional>
#include <utility>
#include <variant>

namespace {
QString errorStringValue(QString *errorString)
{
    return errorString == nullptr ? QString() : *errorString;
}

KiriView::RustFirstDisplayImageDecodeStatus rustFirstDisplayImageDecodeStatus(
    KiriView::FirstDisplayImageDecodeStatus status)
{
    switch (status) {
    case KiriView::FirstDisplayImageDecodeStatus::Ready:
        return KiriView::RustFirstDisplayImageDecodeStatus::Ready;
    case KiriView::FirstDisplayImageDecodeStatus::NotImplemented:
        return KiriView::RustFirstDisplayImageDecodeStatus::NotImplemented;
    case KiriView::FirstDisplayImageDecodeStatus::Error:
        return KiriView::RustFirstDisplayImageDecodeStatus::Error;
    }

    return KiriView::RustFirstDisplayImageDecodeStatus::Error;
}
}

namespace KiriView {
DecodedImageResult::DecodedImageResult(DecodedImageFailure failure)
    : m_payload(std::move(failure))
{
}

DecodedImageResult::DecodedImageResult(DecodedImage image)
    : m_payload(std::move(image))
{
}

const DecodedImageFailure *DecodedImageResult::failure() const
{
    return std::get_if<DecodedImageFailure>(&m_payload);
}

const DecodedImage *DecodedImageResult::image() const
{
    return std::get_if<DecodedImage>(&m_payload);
}

std::optional<DecodedImage> DecodedImageResult::takeImage() &&
{
    auto *image = std::get_if<DecodedImage>(&m_payload);
    if (image == nullptr) {
        return std::nullopt;
    }

    return std::move(*image);
}

DecodedImageResult failedDecodedImageResult(QString errorString)
{
    return DecodedImageResult(DecodedImageFailure { std::move(errorString) });
}

DecodedImageResult successfulDecodedImageResult(DecodedImage image)
{
    return DecodedImageResult(std::move(image));
}

DecodedImageResult staticDecodedImageResult(std::shared_ptr<ImageTileSource> source,
    const ImageFirstDisplayDecodeContext &firstDisplay, QString *errorString)
{
    if (source == nullptr) {
        return failedDecodedImageResult(errorStringValue(errorString));
    }

    FirstDisplayImageDecodeResult firstDisplayResult
        = source->decodeFirstDisplayImage(firstDisplay, errorString);
    switch (rustStaticDecodedImagePlanAfterFirstDisplay(
        rustFirstDisplayImageDecodeStatus(firstDisplayResult.status),
        !firstDisplayResult.image.isNull())) {
    case RustStaticDecodedImagePlan::UseFirstDisplay:
        return successfulDecodedImageResult(StaticDecodedImage {
            StaticImagePayload { std::move(source), std::move(firstDisplayResult.image),
                StaticImageDisplayHints { firstDisplayResult.displayPixelsPerSourcePixel } },
        });
    case RustStaticDecodedImagePlan::DecodeBlockingDisplay:
        break;
    case RustStaticDecodedImagePlan::Fail:
        return failedDecodedImageResult(errorStringValue(errorString));
    }

    QImage preview
        = source->decodeBlockingDisplayImage(imageBlockingDisplayLongEdgeMax, errorString);
    if (rustStaticDecodedImagePlanAfterBlockingDisplay(!preview.isNull())
        == RustStaticDecodedImagePlan::Fail) {
        return failedDecodedImageResult(errorStringValue(errorString));
    }

    return successfulDecodedImageResult(StaticDecodedImage {
        StaticImagePayload { std::move(source), std::move(preview), {} },
    });
}

const DecodedImageFailure *decodedImageResultFailure(const DecodedImageResult &result)
{
    return result.failure();
}

const DecodedImage *decodedImageResultImage(const DecodedImageResult &result)
{
    return result.image();
}
}
