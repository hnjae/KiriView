// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagebytecost.h"

#include <algorithm>
#include <limits>

#if defined(Q_OS_LINUX)
#include <unistd.h>
#endif

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
    if (size.isEmpty()) {
        return 0;
    }

    constexpr qsizetype bytesPerPixel = 4;
    const qsizetype width = static_cast<qsizetype>(size.width());
    const qsizetype height = static_cast<qsizetype>(size.height());
    if (width > std::numeric_limits<qsizetype>::max() / height
        || width * height > std::numeric_limits<qsizetype>::max() / bytesPerPixel) {
        return std::numeric_limits<qsizetype>::max();
    }

    return width * height * bytesPerPixel;
}

std::optional<qsizetype> systemMemoryByteSize()
{
#if defined(Q_OS_LINUX)
    const long pageCount = ::sysconf(_SC_PHYS_PAGES);
    const long pageSize = ::sysconf(_SC_PAGE_SIZE);
    if (pageCount <= 0 || pageSize <= 0) {
        return std::nullopt;
    }

    const qsizetype normalizedPageCount = static_cast<qsizetype>(pageCount);
    const qsizetype normalizedPageSize = static_cast<qsizetype>(pageSize);
    const qsizetype maximumByteSize = std::numeric_limits<qsizetype>::max();
    if (normalizedPageCount > maximumByteSize / normalizedPageSize) {
        return maximumByteSize;
    }

    return normalizedPageCount * normalizedPageSize;
#else
    return std::nullopt;
#endif
}

qsizetype systemMemoryCappedByteBudget(
    qsizetype preferredByteBudget, qsizetype systemMemoryByteSize, qsizetype memoryDivisor)
{
    if (preferredByteBudget <= 0) {
        return 0;
    }
    if (systemMemoryByteSize <= 0 || memoryDivisor <= 0) {
        return preferredByteBudget;
    }

    return std::min(preferredByteBudget, systemMemoryByteSize / memoryDivisor);
}
}
