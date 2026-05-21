// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECACHEPOLICY_H
#define KIRIVIEW_IMAGECACHEPOLICY_H

#include <QtGlobal>
#include <cstddef>
#include <vector>

namespace KiriView {
struct ImageCacheRetentionEntry {
    qsizetype byteCost = 0;
    quint64 lastUse = 0;
};

std::vector<std::size_t> lruCacheRetainedIndices(
    const std::vector<ImageCacheRetentionEntry> &entries, qsizetype byteBudget);
qsizetype staticTileCacheByteBudgetForSystemMemory(
    qsizetype systemMemoryByteSize, qsizetype preferredByteBudget);
}

#endif
