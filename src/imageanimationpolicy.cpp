// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageanimationpolicy.h"

#include <algorithm>
#include <limits>

namespace {
constexpr int defaultAnimationFrameDelayMs = 100;
constexpr int minimumAnimationFrameDelayMs = 10;
constexpr std::uint64_t millisecondsPerSecond = 1000;
}

namespace KiriView {
void AnimationPlaybackState::startLoop(int loopCount)
{
    m_loopState = AnimationLoopState { loopCount, 0 };
}

void AnimationPlaybackState::clear() { m_loopState = {}; }

AnimationLoopState AnimationPlaybackState::loopState() const { return m_loopState; }

AnimationFramePlan AnimationPlaybackState::planAfterFrame(bool sourceHasMoreFrames) const
{
    return AnimationFramePlan {
        sourceHasMoreFrames || animationHasRemainingLoops(m_loopState)
            ? AnimationFrameAction::ScheduleNextFrame
            : AnimationFrameAction::Stop,
        m_loopState.completedLoops,
    };
}

AnimationSequencePlan AnimationPlaybackState::planAtSequenceEnd()
{
    const AnimationLoopAdvance advance = advanceAnimationLoop(m_loopState);
    m_loopState.completedLoops = advance.completedLoops;
    return AnimationSequencePlan {
        advance.shouldContinue ? AnimationSequenceAction::RestartSequence
                               : AnimationSequenceAction::Stop,
        advance.completedLoops,
    };
}

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

    const std::uint64_t delayMs
        = (static_cast<std::uint64_t>(duration) * millisecondsPerSecond + timescale - 1)
        / timescale;
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
        return AnimationLoopAdvance { false, state.completedLoops };
    }

    return AnimationLoopAdvance {
        true,
        state.completedLoops == std::numeric_limits<int>::max() ? state.completedLoops
                                                                : state.completedLoops + 1,
    };
}

}
