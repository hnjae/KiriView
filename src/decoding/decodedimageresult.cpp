// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimageresult.h"

#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace kiriview {
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

DecodedImageFailure *DecodedImageResult::failure()
{
    return std::get_if<DecodedImageFailure>(&m_payload);
}

const DecodedImage *DecodedImageResult::image() const
{
    return std::get_if<DecodedImage>(&m_payload);
}

DecodedImage *DecodedImageResult::image() { return std::get_if<DecodedImage>(&m_payload); }

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
    const QString diagnosticDetail = errorString;
    return DecodedImageResult(DecodedImageFailure {
        std::move(errorString),
        DecodedImageFailureRoute::Unknown,
        DecodedImageFailureOperation::Unknown,
        diagnosticDetail,
        DecodedImageFailureSeverity::Error,
        false,
    });
}

DecodedImageResult failedDecodedImageResult(DecodedImageFailure failure)
{
    if (failure.diagnosticDetail.isEmpty()) {
        failure.diagnosticDetail = failure.errorString;
    }
    return DecodedImageResult(std::move(failure));
}

DecodedImageResult successfulDecodedImageResult(DecodedImage image)
{
    return DecodedImageResult(std::move(image));
}

const DecodedImageFailure *decodedImageResultFailure(const DecodedImageResult &result)
{
    return result.failure();
}

DecodedImageFailure *decodedImageResultFailure(DecodedImageResult &result)
{
    return result.failure();
}

const DecodedImage *decodedImageResultImage(const DecodedImageResult &result)
{
    return result.image();
}

DecodedImage *decodedImageResultImage(DecodedImageResult &result) { return result.image(); }

const EmbeddedMetadata &decodedImageEmbeddedMetadata(const DecodedImage &image)
{
    return std::visit(
        [](const auto &decoded) -> const EmbeddedMetadata & { return decoded.embeddedMetadata; },
        image);
}

void setDecodedImageEmbeddedMetadata(DecodedImage &image, EmbeddedMetadata metadata)
{
    std::visit(
        [&metadata](auto &decoded) {
            decoded.embeddedMetadata = metadata;
            if constexpr (std::is_same_v<std::decay_t<decltype(decoded)>, StaticDecodedImage>) {
                decoded.displayImage.embeddedMetadata = metadata;
            }
        },
        image);
}
}
