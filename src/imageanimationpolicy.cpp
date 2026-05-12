// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageanimationpolicy.h"

#include "kiriview/src/imageanimationpolicy.cxx.h"

namespace {
KiriView::RustAnimationLoopState rustAnimationLoopState(KiriView::AnimationLoopState state)
{
    return KiriView::RustAnimationLoopState { state.loopCount, state.completedLoops };
}

KiriView::AnimationLoopAdvance animationLoopAdvance(KiriView::RustAnimationLoopAdvance advance)
{
    return KiriView::AnimationLoopAdvance { advance.should_continue, advance.completed_loops };
}

KiriView::DecodedAnimationAdvance decodedAnimationAdvance(
    KiriView::RustDecodedAnimationAdvance advance)
{
    return KiriView::DecodedAnimationAdvance { advance.frame_available, advance.frame_index,
        advance.completed_loops, advance.schedule_next_frame };
}
}

namespace KiriView {
int normalizedAnimationFrameDelay(int delayMs)
{
    return rustNormalizedAnimationFrameDelay(delayMs);
}

int animationFrameDelayFromTimescale(std::uint32_t duration, std::uint32_t timescale)
{
    return rustAnimationFrameDelayFromTimescale(duration, timescale);
}

bool animationHasRemainingLoops(AnimationLoopState state)
{
    return rustAnimationHasRemainingLoops(rustAnimationLoopState(state));
}

AnimationLoopAdvance advanceAnimationLoop(AnimationLoopState state)
{
    return animationLoopAdvance(rustAdvanceAnimationLoop(rustAnimationLoopState(state)));
}

DecodedAnimationAdvance advanceDecodedAnimation(
    std::size_t frameCount, std::size_t frameIndex, AnimationLoopState state)
{
    return decodedAnimationAdvance(
        rustAdvanceDecodedAnimation(frameCount, frameIndex, rustAnimationLoopState(state)));
}
}
