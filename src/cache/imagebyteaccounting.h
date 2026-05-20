// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEBYTEACCOUNTING_H
#define KIRIVIEW_IMAGEBYTEACCOUNTING_H

#include <QtGlobal>
#include <cstdint>
#include <limits>

namespace KiriView {
inline qsizetype saturatedQtByteSize(std::int64_t byteSize)
{
    constexpr qsizetype maximumByteSize = std::numeric_limits<qsizetype>::max();
    constexpr qsizetype minimumByteSize = std::numeric_limits<qsizetype>::min();
    if (byteSize > static_cast<std::int64_t>(maximumByteSize)) {
        return maximumByteSize;
    }
    if (byteSize < static_cast<std::int64_t>(minimumByteSize)) {
        return minimumByteSize;
    }

    return static_cast<qsizetype>(byteSize);
}

inline std::int64_t saturatedPositiveByteProduct(std::int64_t left, std::int64_t right)
{
    if (left <= 0 || right <= 0) {
        return 0;
    }
    if (left > std::numeric_limits<std::int64_t>::max() / right) {
        return std::numeric_limits<std::int64_t>::max();
    }

    return left * right;
}

inline qsizetype saturatedQtByteProduct(std::int64_t left, std::int64_t right)
{
    return saturatedQtByteSize(saturatedPositiveByteProduct(left, right));
}

inline qsizetype saturatedQtByteSum(qsizetype left, qsizetype right)
{
    if (left <= 0) {
        return right > 0 ? right : 0;
    }
    if (right <= 0) {
        return left;
    }
    if (left > std::numeric_limits<qsizetype>::max() - right) {
        return std::numeric_limits<qsizetype>::max();
    }

    return left + right;
}
}

#endif
