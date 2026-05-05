// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimageresult.h"

#include <utility>

namespace KiriView {
DecodedImageResult successfulDecodedImageResult(DecodedImage image)
{
    return DecodedImageResult { std::move(image) };
}

const DecodedImageFailure *decodedImageResultFailure(const DecodedImageResult &result)
{
    return std::get_if<DecodedImageFailure>(&result);
}

const DecodedImage *decodedImageResultImage(const DecodedImageResult &result)
{
    return std::get_if<DecodedImage>(&result);
}

std::optional<DecodedImage> decodedImageFromResult(DecodedImageResult result)
{
    auto *decoded = std::get_if<DecodedImage>(&result);
    if (decoded == nullptr) {
        return std::nullopt;
    }

    return std::move(*decoded);
}
}
