// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageanimationpolicy.h"
#include "presentation/imageanimationpresentergate.h"

#include <QObject>
#include <QTest>
#include <QVector>
#include <algorithm>
#include <limits>

class TestImageAnimationPolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void normalizedFrameDelayUsesDefaultForNegativeDelays();
    void normalizedFrameDelayAppliesMinimumToShortDelays();
    void loopPolicyTracksFiniteAndInfiniteRemainingLoops();
    void loopAdvanceIncrementsOnlyWhenMoreLoopsAreAvailable();
    void loopAdvanceSaturatesCompletedLoopCount();
    void playbackStateOwnsLoopProgress();
    void playbackStatePlansFrameSchedulingAndStop();
    void providerRevisionPresenterPassesDocumentedGate();
    void providerRevisionPresenterAcceptsRepresentativeMeasuredProviderTiming();
    void providerRevisionPresenterRejectsEachGateFailure();
};

namespace {
KiriView::AnimationLoopState loopState(int loopCount, int completedLoops)
{
    return KiriView::AnimationLoopState {
        loopCount,
        completedLoops,
    };
}

int maximumValue(const QVector<int> &values)
{
    return values.isEmpty() ? 0 : *std::max_element(values.cbegin(), values.cend());
}

int percentile95Value(QVector<int> values)
{
    if (values.isEmpty()) {
        return 0;
    }

    std::sort(values.begin(), values.end());
    const int index = std::min(values.size() - 1,
        static_cast<int>((static_cast<qint64>(values.size()) * 95 + 99) / 100) - 1);
    return values.at(index);
}

KiriView::AnimationProviderChurnGateSample sampleFromMeasuredProviderTiming(
    int frameDelayMs, const QVector<int> &timerDriftMs, const QVector<int> &providerLatencyMs)
{
    return KiriView::AnimationProviderChurnGateSample {
        KiriView::normalizedAnimationFrameDelay(frameDelayMs),
        false,
        maximumValue(timerDriftMs),
        percentile95Value(providerLatencyMs),
        maximumValue(providerLatencyMs),
        true,
        2,
        true,
        1,
        true,
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

void TestImageAnimationPolicy::playbackStateOwnsLoopProgress()
{
    KiriView::AnimationPlaybackState state;
    state.startLoop(2);

    QCOMPARE(state.loopState().loopCount, 2);
    QCOMPARE(state.loopState().completedLoops, 0);

    const KiriView::AnimationSequencePlan firstRestart = state.planAtSequenceEnd();
    QVERIFY(firstRestart.action == KiriView::AnimationSequenceAction::RestartSequence);
    QCOMPARE(firstRestart.completedLoops, 1);
    QCOMPARE(state.loopState().completedLoops, 1);

    const KiriView::AnimationSequencePlan secondRestart = state.planAtSequenceEnd();
    QVERIFY(secondRestart.action == KiriView::AnimationSequenceAction::RestartSequence);
    QCOMPARE(secondRestart.completedLoops, 2);
    QCOMPARE(state.loopState().completedLoops, 2);

    const KiriView::AnimationSequencePlan stop = state.planAtSequenceEnd();
    QVERIFY(stop.action == KiriView::AnimationSequenceAction::Stop);
    QCOMPARE(stop.completedLoops, 2);
    QCOMPARE(state.loopState().completedLoops, 2);

    state.clear();
    QCOMPARE(state.loopState().loopCount, 0);
    QCOMPARE(state.loopState().completedLoops, 0);
}

void TestImageAnimationPolicy::playbackStatePlansFrameSchedulingAndStop()
{
    KiriView::AnimationPlaybackState finiteState;
    finiteState.startLoop(1);

    KiriView::AnimationFramePlan plan = finiteState.planAfterFrame(true);
    QVERIFY(plan.action == KiriView::AnimationFrameAction::ScheduleNextFrame);
    QCOMPARE(plan.completedLoops, 0);

    plan = finiteState.planAfterFrame(false);
    QVERIFY(plan.action == KiriView::AnimationFrameAction::ScheduleNextFrame);
    QCOMPARE(plan.completedLoops, 0);

    finiteState.planAtSequenceEnd();
    plan = finiteState.planAfterFrame(false);
    QVERIFY(plan.action == KiriView::AnimationFrameAction::Stop);
    QCOMPARE(plan.completedLoops, 1);

    KiriView::AnimationPlaybackState infiniteState;
    infiniteState.startLoop(-1);
    plan = infiniteState.planAfterFrame(false);
    QVERIFY(plan.action == KiriView::AnimationFrameAction::ScheduleNextFrame);
}

void TestImageAnimationPolicy::providerRevisionPresenterPassesDocumentedGate()
{
    const KiriView::AnimationProviderChurnGateResult result
        = KiriView::evaluateAnimationProviderChurnGate(KiriView::AnimationProviderChurnGateSample {
            10,
            false,
            1,
            2,
            5,
            true,
            2,
            true,
            1,
            true,
        });

    QVERIFY(result.passed());
    QCOMPARE(result.presenter, KiriView::AnimationPresenterKind::ProviderImageRevisions);
    QVERIFY(result.requiresLoadOutcomeAcknowledgment);
    QVERIFY(result.requiresPreviousFrameRetention);
    QCOMPARE(result.maximumPinnedFrameEntriesPerPageRole, 2);
}

void TestImageAnimationPolicy::
    providerRevisionPresenterAcceptsRepresentativeMeasuredProviderTiming()
{
    const KiriView::AnimationProviderChurnGateSample sample
        = sampleFromMeasuredProviderTiming(16, { 0, 1, 1, 2, 1, 0 }, { 1, 1, 2, 2, 1, 2 });

    const KiriView::AnimationProviderChurnGateResult result
        = KiriView::evaluateAnimationProviderChurnGate(sample);

    QVERIFY(result.passed());
    QCOMPARE(sample.providerRequestLatencyP95Ms, 2);
    QCOMPARE(sample.providerRequestLatencyMaxMs, 2);
    QCOMPARE(sample.maximumFrameScheduleDriftMs, 2);
}

void TestImageAnimationPolicy::providerRevisionPresenterRejectsEachGateFailure()
{
    const KiriView::AnimationProviderChurnGateSample passingSample {
        10,
        false,
        1,
        2,
        5,
        true,
        2,
        true,
        1,
        true,
    };

    {
        KiriView::AnimationProviderChurnGateSample sample = passingSample;
        sample.timerWaitsForProviderLoad = true;
        const KiriView::AnimationProviderChurnGateResult result
            = KiriView::evaluateAnimationProviderChurnGate(sample);
        QVERIFY(!result.passed());
        QCOMPARE(result.failure, KiriView::AnimationProviderChurnGateFailure::FramePacing);
    }

    {
        KiriView::AnimationProviderChurnGateSample sample = passingSample;
        sample.providerRequestLatencyP95Ms = 3;
        const KiriView::AnimationProviderChurnGateResult result
            = KiriView::evaluateAnimationProviderChurnGate(sample);
        QVERIFY(!result.passed());
        QCOMPARE(
            result.failure, KiriView::AnimationProviderChurnGateFailure::ProviderRequestLatency);
    }

    {
        KiriView::AnimationProviderChurnGateSample sample = passingSample;
        sample.staleLoadOutcomeRejected = false;
        const KiriView::AnimationProviderChurnGateResult result
            = KiriView::evaluateAnimationProviderChurnGate(sample);
        QVERIFY(!result.passed());
        QCOMPARE(result.failure, KiriView::AnimationProviderChurnGateFailure::StaleFrameRejection);
    }

    {
        KiriView::AnimationProviderChurnGateSample sample = passingSample;
        sample.retainedFrameEntriesPerPageRole = 3;
        const KiriView::AnimationProviderChurnGateResult result
            = KiriView::evaluateAnimationProviderChurnGate(sample);
        QVERIFY(!result.passed());
        QCOMPARE(result.failure, KiriView::AnimationProviderChurnGateFailure::MemoryRetention);
    }

    {
        KiriView::AnimationProviderChurnGateSample sample = passingSample;
        sample.providerUrlsPerAcceptedFrame = 2;
        const KiriView::AnimationProviderChurnGateResult result
            = KiriView::evaluateAnimationProviderChurnGate(sample);
        QVERIFY(!result.passed());
        QCOMPARE(result.failure, KiriView::AnimationProviderChurnGateFailure::UrlChurn);
    }
}

QTEST_GUILESS_MAIN(TestImageAnimationPolicy)

#include "test_imageanimationpolicy.moc"
