// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "cache/imagecachepolicy.h"

#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/cachebudget.cxx.h"

#include <cstdint>
#include <utility>

namespace {
std::int64_t rustByteCost(qsizetype byteCost) { return static_cast<std::int64_t>(byteCost); }

std::uint64_t rustUseClock(quint64 useClock) { return static_cast<std::uint64_t>(useClock); }

KiriView::RustLruCacheRetentionEntry rustRetentionEntry(KiriView::ImageCacheRetentionEntry entry)
{
    return KiriView::RustLruCacheRetentionEntry {
        rustByteCost(entry.byteCost),
        rustUseClock(entry.lastUse),
    };
}
}

namespace KiriView {
std::vector<std::size_t> lruCacheRetainedIndices(
    const std::vector<ImageCacheRetentionEntry> &entries, qsizetype byteBudget)
{
    rust::Vec<RustLruCacheRetentionEntry> rustEntries;
    rustEntries.reserve(entries.size());

    for (ImageCacheRetentionEntry entry : entries) {
        rustEntries.push_back(rustRetentionEntry(entry));
    }

    const rust::Vec<std::size_t> retainedIndices
        = rustLruCacheRetainedIndices(std::move(rustEntries), rustByteCost(byteBudget));
    return std::vector<std::size_t>(retainedIndices.cbegin(), retainedIndices.cend());
}

qsizetype staticTileCacheByteBudgetForSystemMemory(
    qsizetype systemMemoryByteSize, qsizetype preferredByteBudget)
{
    return Bridge::qtByteSize(rustStaticTileCacheByteBudgetForSystemMemory(
        Bridge::rustByteSize(systemMemoryByteSize), Bridge::rustByteSize(preferredByteBudget)));
}
}
