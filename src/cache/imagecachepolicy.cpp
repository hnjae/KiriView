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

KiriView::ImageCacheRetainedEntry retainedEntryFromRust(KiriView::RustLruCacheRetainedEntry entry)
{
    return KiriView::ImageCacheRetainedEntry {
        entry.original_index,
        KiriView::Bridge::qtByteSize(entry.byte_cost),
    };
}
}

namespace KiriView {
std::vector<ImageCacheRetainedEntry> lruCacheRetentionPlan(
    const std::vector<ImageCacheRetentionEntry> &entries, qsizetype byteBudget)
{
    rust::Vec<RustLruCacheRetentionEntry> rustEntries;
    rustEntries.reserve(entries.size());

    for (ImageCacheRetentionEntry entry : entries) {
        rustEntries.push_back(rustRetentionEntry(entry));
    }

    const rust::Vec<RustLruCacheRetainedEntry> retainedEntries
        = rustLruCacheRetentionPlan(std::move(rustEntries), rustByteCost(byteBudget));
    std::vector<ImageCacheRetainedEntry> retentionPlan;
    retentionPlan.reserve(retainedEntries.size());
    for (RustLruCacheRetainedEntry entry : retainedEntries) {
        retentionPlan.push_back(retainedEntryFromRust(entry));
    }
    return retentionPlan;
}

qsizetype staticTileCacheByteBudgetForSystemMemory(
    qsizetype systemMemoryByteSize, qsizetype preferredByteBudget)
{
    return Bridge::qtByteSize(rustStaticTileCacheByteBudgetForSystemMemory(
        Bridge::rustByteSize(systemMemoryByteSize), Bridge::rustByteSize(preferredByteBudget)));
}

qsizetype predecodeCachePreferredByteBudget()
{
    return Bridge::qtByteSize(rustPredecodeCachePreferredByteBudget());
}

qsizetype predecodeCacheByteBudgetForSystemMemory(qsizetype systemMemoryByteSize)
{
    return Bridge::qtByteSize(
        rustPredecodeCacheByteBudgetForSystemMemory(Bridge::rustByteSize(systemMemoryByteSize)));
}

qsizetype thumbnailCachePreferredByteBudget()
{
    return Bridge::qtByteSize(rustThumbnailCachePreferredByteBudget());
}

qsizetype thumbnailCacheByteBudgetForSystemMemory(qsizetype systemMemoryByteSize)
{
    return Bridge::qtByteSize(
        rustThumbnailCacheByteBudgetForSystemMemory(Bridge::rustByteSize(systemMemoryByteSize)));
}

ImageCacheBudgets resolvedImageCacheBudgets(
    ImageCacheBudgetRequest request, SystemMemorySnapshot systemMemory)
{
    const qsizetype predecodeCacheByteBudget = request.predecodeCacheByteBudget > 0
        ? request.predecodeCacheByteBudget
        : predecodeCacheByteBudgetForSystemMemory(systemMemory.physicalByteSize);
    const qsizetype staticTileCacheByteBudget = request.staticTileCacheByteBudget > 0
        ? request.staticTileCacheByteBudget
        : staticTileCacheByteBudgetForSystemMemory(
              systemMemory.physicalByteSize, request.staticTileCachePreferredByteBudget);
    const qsizetype thumbnailCacheByteBudget = request.thumbnailCacheByteBudget > 0
        ? request.thumbnailCacheByteBudget
        : thumbnailCacheByteBudgetForSystemMemory(systemMemory.physicalByteSize);
    const qsizetype displayImageCacheByteBudget = request.displayImageCacheByteBudget > 0
        ? request.displayImageCacheByteBudget
        : staticTileCacheByteBudget;
    return ImageCacheBudgets {
        predecodeCacheByteBudget,
        staticTileCacheByteBudget,
        thumbnailCacheByteBudget,
        displayImageCacheByteBudget,
    };
}
}
