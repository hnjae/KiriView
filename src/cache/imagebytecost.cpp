// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "cache/imagebytecost.h"

#include "cache/imagebyteaccounting.h"

namespace {
constexpr std::int64_t rgbaBytesPerPixel = 4;
}

namespace KiriView {
qsizetype imageByteCost(const QImage &image)
{
    if (image.isNull()) {
        return 0;
    }
    return image.sizeInBytes();
}

qsizetype estimatedRgbaByteCost(const QSize &size)
{
    const std::int64_t pixelCount = saturatedPositiveByteProduct(size.width(), size.height());
    return saturatedQtByteProduct(pixelCount, rgbaBytesPerPixel);
}
}
