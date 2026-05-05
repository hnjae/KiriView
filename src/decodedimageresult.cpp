// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimageresult.h"

#include <utility>

namespace {
bool staticDecodedImageIsPredecodeCacheable(
    const KiriView::StaticDecodedImage *image, qsizetype byteBudget)
{
    if (image == nullptr || !image->staticImage.isValid()) {
        return false;
    }

    return image->staticImage.byteCost() <= byteBudget;
}
}

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

bool decodedImageIsPredecodeCacheable(const DecodedImage &image, qsizetype byteBudget)
{
    return staticDecodedImageIsPredecodeCacheable(
        std::get_if<StaticDecodedImage>(&image), byteBudget);
}

bool decodedImageResultIsPredecodeCacheable(const DecodedImageResult &result, qsizetype byteBudget)
{
    const DecodedImage *image = decodedImageResultImage(result);
    return image != nullptr && decodedImageIsPredecodeCacheable(*image, byteBudget);
}
}
