// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimageresult.h"

namespace KiriView {
bool decodedImageResultIsPredecodeCacheable(const DecodedImageResult &result, qsizetype byteBudget)
{
    const auto *image = std::get_if<StaticDecodedImage>(&result);
    if (image == nullptr || !image->staticImage.isValid()) {
        return false;
    }

    return image->staticImage.byteCost() <= byteBudget;
}
}
