// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "cache/imagebytecost.h"

#include "cache/imagebyteaccounting.h"
#include "kiriview/src/policy/imagebytecost.cxx.h"

namespace kiriview {
qsizetype imageByteCost(const QImage& image)
{
    if (image.isNull()) {
        return 0;
    }
    return image.sizeInBytes();
}

qsizetype estimatedRgbaByteCost(QSize size)
{
    return saturatedQtByteSize(rustEstimatedRgbaByteCost(size.width(), size.height()));
}
}
