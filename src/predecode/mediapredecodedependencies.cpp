// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodedependencies.h"

#include <utility>

namespace kiriview {
MediaPredecodeDependencies resolveMediaPredecodeDependencies(
    MediaPredecodeDependencyOverrides overrides)
{
    const SystemMemorySnapshot systemMemory
        = overrides.systemMemorySnapshot.value_or(systemMemorySnapshot());
    const qsizetype cacheByteBudget = overrides.cacheBudgetRequest.predecodeCacheByteBudget > 0
        ? overrides.cacheBudgetRequest.predecodeCacheByteBudget
        : predecodeCacheByteBudgetForSystemMemory(systemMemory.physicalByteSize);

    return MediaPredecodeDependencies {
        imageDecodeDependenciesWithDefaults(std::move(overrides.imageDecode)),
        powerSaverProviderWithDefault(std::move(overrides.powerSaver)),
        cacheByteBudget,
        timerSchedulerWithDefaults(std::move(overrides.timerScheduler)),
    };
}
}
