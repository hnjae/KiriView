// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimageresult.h"

#include "imagebytecost.h"

namespace KiriView {
bool decodedImageResultIsPredecodeCacheable(const DecodedImageResult &result, qsizetype byteBudget)
{
    const auto *image = std::get_if<StaticDecodedImage>(&result);
    return image != nullptr && !image->image.isNull() && imageByteCost(image->image) <= byteBudget;
}
}
