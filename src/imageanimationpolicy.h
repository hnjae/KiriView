// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEANIMATIONPOLICY_H
#define KIRIVIEW_IMAGEANIMATIONPOLICY_H

#include <cstdint>

namespace KiriView {
struct AnimationLoopState {
    int loopCount = 0;
    int completedLoops = 0;
};

struct AnimationLoopAdvance {
    bool shouldContinue = false;
    int completedLoops = 0;
};

class AnimationLoopTracker
{
public:
    void start(int loopCount);
    void clear();
    AnimationLoopState state() const;
    bool shouldScheduleAfterFrame(bool sourceHasMoreFrames) const;
    AnimationLoopAdvance completeSequence();

private:
    AnimationLoopState m_state;
};

int normalizedAnimationFrameDelay(int delayMs);
int animationFrameDelayFromTimescale(std::uint32_t duration, std::uint32_t timescale);
bool animationHasRemainingLoops(AnimationLoopState state);
AnimationLoopAdvance advanceAnimationLoop(AnimationLoopState state);
}

#endif
