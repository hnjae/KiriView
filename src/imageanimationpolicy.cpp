// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageanimationpolicy.h"

#include <algorithm>
#include <limits>

namespace {
constexpr int defaultAnimationFrameDelayMs = 100;
constexpr int minimumAnimationFrameDelayMs = 10;
constexpr std::uint64_t millisecondsPerSecond = 1000;

KiriView::DecodedAnimationAdvance stoppedDecodedAnimationAdvance(int completedLoops)
{
    return KiriView::DecodedAnimationAdvance {
        false,
        0,
        completedLoops,
        false,
    };
}
}

namespace KiriView {
int normalizedAnimationFrameDelay(int delayMs)
{
    if (delayMs < 0) {
        return defaultAnimationFrameDelayMs;
    }

    return std::max(delayMs, minimumAnimationFrameDelayMs);
}

int animationFrameDelayFromTimescale(std::uint32_t duration, std::uint32_t timescale)
{
    if (duration == 0 || timescale == 0) {
        return 0;
    }

    const auto scale = static_cast<std::uint64_t>(timescale);
    const std::uint64_t delayMs
        = (static_cast<std::uint64_t>(duration) * millisecondsPerSecond + scale - 1) / scale;
    return static_cast<int>(
        std::min(delayMs, static_cast<std::uint64_t>(std::numeric_limits<int>::max())));
}

bool animationHasRemainingLoops(AnimationLoopState state)
{
    return state.loopCount < 0 || state.completedLoops < state.loopCount;
}

AnimationLoopAdvance advanceAnimationLoop(AnimationLoopState state)
{
    if (!animationHasRemainingLoops(state)) {
        return AnimationLoopAdvance {
            false,
            state.completedLoops,
        };
    }

    return AnimationLoopAdvance {
        true,
        state.completedLoops == std::numeric_limits<int>::max() ? std::numeric_limits<int>::max()
                                                                : state.completedLoops + 1,
    };
}

DecodedAnimationAdvance advanceDecodedAnimation(
    std::size_t frameCount, std::size_t frameIndex, AnimationLoopState state)
{
    if (frameCount == 0 || frameIndex >= frameCount) {
        return stoppedDecodedAnimationAdvance(state.completedLoops);
    }

    const std::size_t nextFrameIndex = frameIndex + 1;
    int completedLoops = state.completedLoops;
    if (nextFrameIndex >= frameCount) {
        const AnimationLoopAdvance loopAdvance = advanceAnimationLoop(state);
        if (!loopAdvance.shouldContinue) {
            return stoppedDecodedAnimationAdvance(loopAdvance.completedLoops);
        }

        frameIndex = 0;
        completedLoops = loopAdvance.completedLoops;
    } else {
        frameIndex = nextFrameIndex;
    }

    const AnimationLoopState nextState {
        state.loopCount,
        completedLoops,
    };
    return DecodedAnimationAdvance {
        true,
        frameIndex,
        completedLoops,
        frameIndex + 1 < frameCount || animationHasRemainingLoops(nextState),
    };
}
}
