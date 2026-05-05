// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimageresult.h"

#include <type_traits>
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
std::optional<DecodedImage> decodedImageFromResult(DecodedImageResult result)
{
    return std::visit(
        [](auto &decoded) -> std::optional<DecodedImage> {
            using Decoded = std::decay_t<decltype(decoded)>;
            if constexpr (std::is_same_v<Decoded, DecodedImageFailure>) {
                return std::nullopt;
            } else {
                return DecodedImage { std::move(decoded) };
            }
        },
        result);
}

bool decodedImageIsPredecodeCacheable(const DecodedImage &image, qsizetype byteBudget)
{
    return staticDecodedImageIsPredecodeCacheable(
        std::get_if<StaticDecodedImage>(&image), byteBudget);
}

bool decodedImageResultIsPredecodeCacheable(const DecodedImageResult &result, qsizetype byteBudget)
{
    return staticDecodedImageIsPredecodeCacheable(
        std::get_if<StaticDecodedImage>(&result), byteBudget);
}
}
