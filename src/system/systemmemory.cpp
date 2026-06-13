// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "systemmemory.h"

#include "cache/imagebyteaccounting.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#if defined(Q_OS_LINUX)
#include <unistd.h>
#endif

namespace kiriview {
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

SystemMemoryRuntime defaultSystemMemoryRuntime()
{
    return SystemMemoryRuntime {
        physicalSystemMemoryByteSize,
    };
}

SystemMemoryRuntime systemMemoryRuntimeWithDefaults(SystemMemoryRuntime runtime)
{
    SystemMemoryRuntime defaults = defaultSystemMemoryRuntime();
    if (!runtime.readPhysicalSystemMemory) {
        runtime.readPhysicalSystemMemory = std::move(defaults.readPhysicalSystemMemory);
    }

    return runtime;
}

SystemMemorySnapshot systemMemorySnapshot(SystemMemoryRuntime runtime)
{
    runtime = systemMemoryRuntimeWithDefaults(std::move(runtime));
    const std::optional<qsizetype> physicalByteSize = runtime.readPhysicalSystemMemory();
    return SystemMemorySnapshot {
        std::max<qsizetype>(0, physicalByteSize.value_or(0)),
    };
}
}
