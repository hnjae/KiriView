// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "systemmemory.h"

#include "imagebyteaccounting.h"

#include <cstdint>

#if defined(Q_OS_LINUX)
#include <unistd.h>
#endif

namespace KiriView {
std::optional<qsizetype> physicalSystemMemoryByteSize()
{
#if defined(Q_OS_LINUX)
    const long pageCount = ::sysconf(_SC_PHYS_PAGES);
    const long pageSize = ::sysconf(_SC_PAGE_SIZE);
    if (pageCount <= 0 || pageSize <= 0) {
        return std::nullopt;
    }

    return saturatedQtByteProduct(pageCount, pageSize);
#else
    return std::nullopt;
#endif
}
}
