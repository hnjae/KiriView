// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECACHEPOLICY_H
#define KIRIVIEW_IMAGECACHEPOLICY_H

#include <QtGlobal>
#include <cstddef>
#include <vector>

namespace KiriView {
std::vector<std::size_t> lruCacheRetainedIndices(const std::vector<qsizetype> &byteCosts,
    const std::vector<quint64> &lastUses, qsizetype byteBudget);
}

#endif
