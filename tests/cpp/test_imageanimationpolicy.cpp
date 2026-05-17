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

QTEST_GUILESS_MAIN(TestImageAnimationPolicy)

#include "test_imageanimationpolicy.moc"
