// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ANIMATIONTIMING_H
#define KIRIVIEW_ANIMATIONTIMING_H

#include <cstdint>

namespace kiriview {
int heifFrameDelay(std::uint32_t duration, std::uint32_t timescale);
int jxlFrameDelay(std::uint32_t duration, std::uint32_t ticksPerSecondNumerator,
    std::uint32_t ticksPerSecondDenominator);
int apngFrameDelay(std::uint32_t numerator, std::uint32_t denominator);
int animationLoopCountForPlayCount(std::uint32_t playCount);
int apngLoopCountForPlayCount(std::uint32_t playCount);
}

#endif
