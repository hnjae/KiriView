// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "animationtiming.h"

#include <algorithm>
#include <limits>

namespace {
constexpr std::uint32_t apngDefaultDelayDenominator = 100;
constexpr std::uint64_t millisecondsPerSecond = 1000;

enum class DelayRounding {
    Floor,
    Ceiling,
};

int animationFrameDelayFromTimescale(
    std::uint32_t duration, std::uint32_t timescale, DelayRounding rounding)
{
    if (duration == 0 || timescale == 0) {
        return 0;
    }

    std::uint64_t delayMs = static_cast<std::uint64_t>(duration) * millisecondsPerSecond;
    if (rounding == DelayRounding::Ceiling) {
        delayMs += timescale - 1;
    }
    delayMs /= timescale;
    return static_cast<int>(
        std::min(delayMs, static_cast<std::uint64_t>(std::numeric_limits<int>::max())));
}
}

namespace KiriView {
int heifFrameDelay(std::uint32_t duration, std::uint32_t timescale)
{
    return animationFrameDelayFromTimescale(duration, timescale, DelayRounding::Ceiling);
}

int apngFrameDelay(std::uint32_t numerator, std::uint32_t denominator)
{
    return animationFrameDelayFromTimescale(numerator,
        denominator == 0 ? apngDefaultDelayDenominator : denominator, DelayRounding::Floor);
}

int apngLoopCountForPlayCount(std::uint32_t playCount)
{
    if (playCount == 0) {
        return -1;
    }
    return static_cast<int>(
        std::min(playCount - 1, static_cast<std::uint32_t>(std::numeric_limits<int>::max())));
}
}
