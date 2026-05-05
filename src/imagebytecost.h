// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEBYTECOST_H
#define KIRIVIEW_IMAGEBYTECOST_H

#include <QImage>
#include <QSize>
#include <QtGlobal>
#include <optional>

namespace KiriView {
qsizetype imageByteCost(const QImage &image);
qsizetype estimatedRgbaByteCost(const QSize &size);
std::optional<qsizetype> systemMemoryByteSize();
qsizetype systemMemoryCappedByteBudget(
    qsizetype preferredByteBudget, qsizetype systemMemoryByteSize, qsizetype memoryDivisor);
qsizetype defaultSystemMemoryCappedByteBudget(
    qsizetype preferredByteBudget, qsizetype memoryDivisor);
}

#endif
