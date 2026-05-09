// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagebytecost.h"

#include "kiriview/src/imagebytecost.cxx.h"

#include <cstdint>
#include <limits>

#if defined(Q_OS_LINUX)
#include <unistd.h>
#endif

namespace {
qsizetype qtByteSize(std::int64_t byteSize)
{
    constexpr qsizetype maximumByteSize = std::numeric_limits<qsizetype>::max();
    constexpr qsizetype minimumByteSize = std::numeric_limits<qsizetype>::min();
    if (byteSize > static_cast<std::int64_t>(maximumByteSize)) {
        return maximumByteSize;
    }
    if (byteSize < static_cast<std::int64_t>(minimumByteSize)) {
        return minimumByteSize;
    }

    return static_cast<qsizetype>(byteSize);
}

std::int64_t rustByteSize(qsizetype byteSize) { return static_cast<std::int64_t>(byteSize); }
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
    return qtByteSize(rustEstimatedRgbaByteCost(size.width(), size.height()));
}

std::optional<qsizetype> systemMemoryByteSize()
{
#if defined(Q_OS_LINUX)
    const long pageCount = ::sysconf(_SC_PHYS_PAGES);
    const long pageSize = ::sysconf(_SC_PAGE_SIZE);
    const RustSystemMemoryByteSize byteSize = rustSystemMemoryByteSize(pageCount, pageSize);
    if (!byteSize.found) {
        return std::nullopt;
    }

    return qtByteSize(byteSize.byte_size);
#else
    return std::nullopt;
#endif
}

qsizetype systemMemoryCappedByteBudget(
    qsizetype preferredByteBudget, qsizetype systemMemoryByteSize, qsizetype memoryDivisor)
{
    return qtByteSize(rustSystemMemoryCappedByteBudget(rustByteSize(preferredByteBudget),
        rustByteSize(systemMemoryByteSize), rustByteSize(memoryDivisor)));
}

qsizetype defaultSystemMemoryCappedByteBudget(
    qsizetype preferredByteBudget, qsizetype memoryDivisor)
{
    const std::optional<qsizetype> systemMemory = systemMemoryByteSize();
    if (!systemMemory.has_value()) {
        return systemMemoryCappedByteBudget(preferredByteBudget, 0, memoryDivisor);
    }

    return systemMemoryCappedByteBudget(preferredByteBudget, *systemMemory, memoryDivisor);
}
}
