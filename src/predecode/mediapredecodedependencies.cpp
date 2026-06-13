// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodedependencies.h"

#include "system/systemmemory.h"

#include <utility>

namespace kiriview {
MediaPredecodeDependencies resolveMediaPredecodeDependencies(
    MediaPredecodeDependencyOverrides overrides)
{
    const qsizetype cacheByteBudget = overrides.cacheBudgetRequest.predecodeCacheByteBudget > 0
        ? overrides.cacheBudgetRequest.predecodeCacheByteBudget
        : predecodeCacheByteBudgetForSystemMemory(systemMemorySnapshot().physicalByteSize);

    return MediaPredecodeDependencies {
        imageDecodeDependenciesWithDefaults(std::move(overrides.imageDecode)),
        powerSaverProviderWithDefault(std::move(overrides.powerSaver)),
        cacheByteBudget,
    };
}
}
