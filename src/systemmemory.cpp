// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "systemmemory.h"

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

std::int64_t saturatedByteProduct(std::int64_t left, std::int64_t right)
{
    if (left <= 0 || right <= 0) {
        return 0;
    }
    if (left > std::numeric_limits<std::int64_t>::max() / right) {
        return std::numeric_limits<std::int64_t>::max();
    }

    return left * right;
}
}

namespace KiriView {
std::optional<qsizetype> physicalSystemMemoryByteSize()
{
#if defined(Q_OS_LINUX)
    const long pageCount = ::sysconf(_SC_PHYS_PAGES);
    const long pageSize = ::sysconf(_SC_PAGE_SIZE);
    if (pageCount <= 0 || pageSize <= 0) {
        return std::nullopt;
    }

    return qtByteSize(saturatedByteProduct(pageCount, pageSize));
#else
    return std::nullopt;
#endif
}
}
