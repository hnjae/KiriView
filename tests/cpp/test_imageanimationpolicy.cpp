// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageanimationpolicy.h"

#include <QObject>
#include <QTest>
#include <cstdint>
#include <limits>

class TestImageAnimationPolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void normalizedFrameDelayUsesDefaultForNegativeDelays();
    void normalizedFrameDelayAppliesMinimumToShortDelays();
    void timescaleFrameDelayRoundsUpDurationToMilliseconds();
    void timescaleFrameDelayRejectsMissingDurationOrTimescale();
    void timescaleFrameDelayClampsToTimerRange();
    void loopPolicyTracksFiniteAndInfiniteRemainingLoops();
    void loopAdvanceIncrementsOnlyWhenMoreLoopsAreAvailable();
    void loopAdvanceSaturatesCompletedLoopCount();
    void loopTrackerOwnsPlaybackLoopState();
    void loopTrackerSchedulesWhenSourceOrLoopCanContinue();
};

namespace {
KiriView::AnimationLoopState loopState(int loopCount, int completedLoops)
{
    return KiriView::AnimationLoopState {
        loopCount,
        completedLoops,
    };
}
}

void TestImageAnimationPolicy::normalizedFrameDelayUsesDefaultForNegativeDelays()
{
    QCOMPARE(KiriView::normalizedAnimationFrameDelay(-1), 100);
}

void TestImageAnimationPolicy::normalizedFrameDelayAppliesMinimumToShortDelays()
{
    QCOMPARE(KiriView::normalizedAnimationFrameDelay(0), 10);
    QCOMPARE(KiriView::normalizedAnimationFrameDelay(9), 10);
    QCOMPARE(KiriView::normalizedAnimationFrameDelay(10), 10);
    QCOMPARE(KiriView::normalizedAnimationFrameDelay(25), 25);
}

void TestImageAnimationPolicy::timescaleFrameDelayRoundsUpDurationToMilliseconds()
{
    QCOMPARE(KiriView::animationFrameDelayFromTimescale(1, 24), 42);
    QCOMPARE(KiriView::animationFrameDelayFromTimescale(3, 2), 1500);
}

void TestImageAnimationPolicy::timescaleFrameDelayRejectsMissingDurationOrTimescale()
{
    QCOMPARE(KiriView::animationFrameDelayFromTimescale(0, 24), 0);
    QCOMPARE(KiriView::animationFrameDelayFromTimescale(1, 0), 0);
}

void TestImageAnimationPolicy::timescaleFrameDelayClampsToTimerRange()
{
    QCOMPARE(
        KiriView::animationFrameDelayFromTimescale(std::numeric_limits<std::uint32_t>::max(), 1),
        std::numeric_limits<int>::max());
}

void TestImageAnimationPolicy::loopPolicyTracksFiniteAndInfiniteRemainingLoops()
{
    QVERIFY(KiriView::animationHasRemainingLoops(loopState(-1, 99)));
    QVERIFY(KiriView::animationHasRemainingLoops(loopState(2, 1)));
    QVERIFY(!KiriView::animationHasRemainingLoops(loopState(2, 2)));
}

void TestImageAnimationPolicy::loopAdvanceIncrementsOnlyWhenMoreLoopsAreAvailable()
{
    const KiriView::AnimationLoopAdvance advance = KiriView::advanceAnimationLoop(loopState(2, 1));
    QVERIFY(advance.shouldContinue);
    QCOMPARE(advance.completedLoops, 2);

    const KiriView::AnimationLoopAdvance stop = KiriView::advanceAnimationLoop(loopState(2, 2));
    QVERIFY(!stop.shouldContinue);
    QCOMPARE(stop.completedLoops, 2);
}

void TestImageAnimationPolicy::loopAdvanceSaturatesCompletedLoopCount()
{
    const KiriView::AnimationLoopAdvance advance
        = KiriView::advanceAnimationLoop(loopState(-1, std::numeric_limits<int>::max()));

    QVERIFY(advance.shouldContinue);
    QCOMPARE(advance.completedLoops, std::numeric_limits<int>::max());
}

void TestImageAnimationPolicy::loopTrackerOwnsPlaybackLoopState()
{
    KiriView::AnimationLoopTracker tracker;
    tracker.start(2);

    QCOMPARE(tracker.state().loopCount, 2);
    QCOMPARE(tracker.state().completedLoops, 0);

    const KiriView::AnimationLoopAdvance firstRestart = tracker.completeSequence();
    QVERIFY(firstRestart.shouldContinue);
    QCOMPARE(tracker.state().completedLoops, 1);

    const KiriView::AnimationLoopAdvance secondRestart = tracker.completeSequence();
    QVERIFY(secondRestart.shouldContinue);
    QCOMPARE(tracker.state().completedLoops, 2);

    const KiriView::AnimationLoopAdvance stop = tracker.completeSequence();
    QVERIFY(!stop.shouldContinue);
    QCOMPARE(tracker.state().completedLoops, 2);

    tracker.clear();
    QCOMPARE(tracker.state().loopCount, 0);
    QCOMPARE(tracker.state().completedLoops, 0);
}

void TestImageAnimationPolicy::loopTrackerSchedulesWhenSourceOrLoopCanContinue()
{
    KiriView::AnimationLoopTracker finiteTracker;
    finiteTracker.start(1);

    QVERIFY(finiteTracker.shouldScheduleAfterFrame(true));
    QVERIFY(finiteTracker.shouldScheduleAfterFrame(false));
    finiteTracker.completeSequence();
    QVERIFY(!finiteTracker.shouldScheduleAfterFrame(false));

    KiriView::AnimationLoopTracker infiniteTracker;
    infiniteTracker.start(-1);
    QVERIFY(infiniteTracker.shouldScheduleAfterFrame(false));
}

QTEST_GUILESS_MAIN(TestImageAnimationPolicy)

#include "test_imageanimationpolicy.moc"
