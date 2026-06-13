// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEANIMATIONPOLICY_H
#define KIRIVIEW_IMAGEANIMATIONPOLICY_H

namespace kiriview {
struct AnimationLoopState {
    int loopCount = 0;
    int completedLoops = 0;
};

struct AnimationLoopAdvance {
    bool shouldContinue = false;
    int completedLoops = 0;
};

enum class AnimationFrameAction {
    Stop,
    ScheduleNextFrame,
};

struct AnimationFramePlan {
    AnimationFrameAction action = AnimationFrameAction::Stop;
    int completedLoops = 0;
};

enum class AnimationSequenceAction {
    Stop,
    RestartSequence,
};

struct AnimationSequencePlan {
    AnimationSequenceAction action = AnimationSequenceAction::Stop;
    int completedLoops = 0;
};

class AnimationPlaybackState
{
public:
    void startLoop(int loopCount);
    void clear();
    AnimationLoopState loopState() const;
    AnimationFramePlan planAfterFrame(bool sourceHasMoreFrames) const;
    AnimationSequencePlan planAtSequenceEnd();

private:
    AnimationLoopState m_loopState;
};

int normalizedAnimationFrameDelay(int delayMs);
bool animationHasRemainingLoops(AnimationLoopState state);
AnimationLoopAdvance advanceAnimationLoop(AnimationLoopState state);
}

#endif
