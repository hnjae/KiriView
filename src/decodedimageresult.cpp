// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimageresult.h"

#include <optional>
#include <utility>
#include <variant>

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

const DecodedImageFailure *decodedImageResultFailure(const DecodedImageResult &result)
{
    return result.failure();
}

const DecodedImage *decodedImageResultImage(const DecodedImageResult &result)
{
    return result.image();
}
}
