// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodecachebudget.h"

#include "predecode/predecodecache.h"

namespace KiriView {
qsizetype defaultPredecodeCacheByteBudget()
{
    return defaultPredecodeCacheByteBudget(systemMemorySnapshot());
}

qsizetype defaultPredecodeCacheByteBudget(SystemMemorySnapshot systemMemory)
{
    return PredecodeCache::byteBudgetForSystemMemory(systemMemory.physicalByteSize);
}

qsizetype resolvedPredecodeCacheByteBudget(qsizetype requestedByteBudget)
{
    return resolvedPredecodeCacheByteBudget(requestedByteBudget, systemMemorySnapshot());
}

qsizetype resolvedPredecodeCacheByteBudget(
    qsizetype requestedByteBudget, SystemMemorySnapshot systemMemory)
{
    if (requestedByteBudget > 0) {
        return requestedByteBudget;
    }

    return defaultPredecodeCacheByteBudget(systemMemory);
}
}
