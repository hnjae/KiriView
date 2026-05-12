// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEANIMATIONPOLICY_H
#define KIRIVIEW_IMAGEANIMATIONPOLICY_H

#include <cstddef>
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

struct DecodedAnimationAdvance {
    bool frameAvailable = false;
    std::size_t frameIndex = 0;
    int completedLoops = 0;
    bool scheduleNextFrame = false;
};

int normalizedAnimationFrameDelay(int delayMs);
int animationFrameDelayFromTimescale(std::uint32_t duration, std::uint32_t timescale);
bool animationHasRemainingLoops(AnimationLoopState state);
AnimationLoopAdvance advanceAnimationLoop(AnimationLoopState state);
DecodedAnimationAdvance advanceDecodedAnimation(
    std::size_t frameCount, std::size_t frameIndex, AnimationLoopState state);
}

#endif
