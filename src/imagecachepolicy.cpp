// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecachepolicy.h"

#include "kiriview/src/cachebudget.cxx.h"

#include <cstdint>
#include <utility>

namespace {
std::int64_t rustByteCost(qsizetype byteCost) { return static_cast<std::int64_t>(byteCost); }

std::uint64_t rustUseClock(quint64 useClock) { return static_cast<std::uint64_t>(useClock); }
}

namespace KiriView {
std::vector<std::size_t> lruCacheRetainedIndices(const std::vector<qsizetype> &byteCosts,
    const std::vector<quint64> &lastUses, qsizetype byteBudget)
{
    rust::Vec<std::int64_t> rustByteCosts;
    rust::Vec<std::uint64_t> rustLastUses;
    rustByteCosts.reserve(byteCosts.size());
    rustLastUses.reserve(lastUses.size());

    for (qsizetype byteCost : byteCosts) {
        rustByteCosts.push_back(rustByteCost(byteCost));
    }
    for (quint64 lastUse : lastUses) {
        rustLastUses.push_back(rustUseClock(lastUse));
    }

    const rust::Vec<std::size_t> retainedIndices = rustLruCacheRetainedIndices(
        std::move(rustByteCosts), std::move(rustLastUses), rustByteCost(byteBudget));
    return std::vector<std::size_t>(retainedIndices.cbegin(), retainedIndices.cend());
}
}
