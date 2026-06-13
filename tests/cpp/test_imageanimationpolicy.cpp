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
kiriview::AnimationLoopState loopState(int loopCount, int completedLoops)
{
    return kiriview::AnimationLoopState {
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
    const int lastIndex = static_cast<int>(values.size()) - 1;
    const int index = std::min(
        lastIndex, static_cast<int>((static_cast<qint64>(values.size()) * 95 + 99) / 100) - 1);
    return values.at(index);
}

kiriview::AnimationProviderChurnGateSample sampleFromMeasuredProviderTiming(
    int frameDelayMs, const QVector<int> &timerDriftMs, const QVector<int> &providerLatencyMs)
{
    return kiriview::AnimationProviderChurnGateSample {
        kiriview::normalizedAnimationFrameDelay(frameDelayMs),
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
    QCOMPARE(kiriview::normalizedAnimationFrameDelay(-1), 100);
}

void TestImageAnimationPolicy::normalizedFrameDelayAppliesMinimumToShortDelays()
{
    QCOMPARE(kiriview::normalizedAnimationFrameDelay(0), 10);
    QCOMPARE(kiriview::normalizedAnimationFrameDelay(9), 10);
    QCOMPARE(kiriview::normalizedAnimationFrameDelay(10), 10);
    QCOMPARE(kiriview::normalizedAnimationFrameDelay(25), 25);
}

void TestImageAnimationPolicy::loopPolicyTracksFiniteAndInfiniteRemainingLoops()
{
    QVERIFY(kiriview::animationHasRemainingLoops(loopState(-1, 99)));
    QVERIFY(kiriview::animationHasRemainingLoops(loopState(2, 1)));
    QVERIFY(!kiriview::animationHasRemainingLoops(loopState(2, 2)));
}

void TestImageAnimationPolicy::loopAdvanceIncrementsOnlyWhenMoreLoopsAreAvailable()
{
    const kiriview::AnimationLoopAdvance advance = kiriview::advanceAnimationLoop(loopState(2, 1));
    QVERIFY(advance.shouldContinue);
    QCOMPARE(advance.completedLoops, 2);

    const kiriview::AnimationLoopAdvance stop = kiriview::advanceAnimationLoop(loopState(2, 2));
    QVERIFY(!stop.shouldContinue);
    QCOMPARE(stop.completedLoops, 2);
}

void TestImageAnimationPolicy::loopAdvanceSaturatesCompletedLoopCount()
{
    const kiriview::AnimationLoopAdvance advance
        = kiriview::advanceAnimationLoop(loopState(-1, std::numeric_limits<int>::max()));

    QVERIFY(advance.shouldContinue);
    QCOMPARE(advance.completedLoops, std::numeric_limits<int>::max());
}

void TestImageAnimationPolicy::playbackStateOwnsLoopProgress()
{
    kiriview::AnimationPlaybackState state;
    state.startLoop(2);

    QCOMPARE(state.loopState().loopCount, 2);
    QCOMPARE(state.loopState().completedLoops, 0);

    const kiriview::AnimationSequencePlan firstRestart = state.planAtSequenceEnd();
    QVERIFY(firstRestart.action == kiriview::AnimationSequenceAction::RestartSequence);
    QCOMPARE(firstRestart.completedLoops, 1);
    QCOMPARE(state.loopState().completedLoops, 1);

    const kiriview::AnimationSequencePlan secondRestart = state.planAtSequenceEnd();
    QVERIFY(secondRestart.action == kiriview::AnimationSequenceAction::RestartSequence);
    QCOMPARE(secondRestart.completedLoops, 2);
    QCOMPARE(state.loopState().completedLoops, 2);

    const kiriview::AnimationSequencePlan stop = state.planAtSequenceEnd();
    QVERIFY(stop.action == kiriview::AnimationSequenceAction::Stop);
    QCOMPARE(stop.completedLoops, 2);
    QCOMPARE(state.loopState().completedLoops, 2);

    state.clear();
    QCOMPARE(state.loopState().loopCount, 0);
    QCOMPARE(state.loopState().completedLoops, 0);
}

void TestImageAnimationPolicy::playbackStatePlansFrameSchedulingAndStop()
{
    kiriview::AnimationPlaybackState finiteState;
    finiteState.startLoop(1);

    kiriview::AnimationFramePlan plan = finiteState.planAfterFrame(true);
    QVERIFY(plan.action == kiriview::AnimationFrameAction::ScheduleNextFrame);
    QCOMPARE(plan.completedLoops, 0);

    plan = finiteState.planAfterFrame(false);
    QVERIFY(plan.action == kiriview::AnimationFrameAction::ScheduleNextFrame);
    QCOMPARE(plan.completedLoops, 0);

    finiteState.planAtSequenceEnd();
    plan = finiteState.planAfterFrame(false);
    QVERIFY(plan.action == kiriview::AnimationFrameAction::Stop);
    QCOMPARE(plan.completedLoops, 1);

    kiriview::AnimationPlaybackState infiniteState;
    infiniteState.startLoop(-1);
    plan = infiniteState.planAfterFrame(false);
    QVERIFY(plan.action == kiriview::AnimationFrameAction::ScheduleNextFrame);
}

void TestImageAnimationPolicy::providerRevisionPresenterPassesDocumentedGate()
{
    const kiriview::AnimationProviderChurnGateResult result
        = kiriview::evaluateAnimationProviderChurnGate(kiriview::AnimationProviderChurnGateSample {
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
    QCOMPARE(result.presenter, kiriview::AnimationPresenterKind::ProviderImageRevisions);
    QVERIFY(result.requiresLoadOutcomeAcknowledgment);
    QVERIFY(result.requiresPreviousFrameRetention);
    QCOMPARE(result.maximumPinnedFrameEntriesPerPageRole, 2);
}

void TestImageAnimationPolicy::
    providerRevisionPresenterAcceptsRepresentativeMeasuredProviderTiming()
{
    const kiriview::AnimationProviderChurnGateSample sample
        = sampleFromMeasuredProviderTiming(16, { 0, 1, 1, 2, 1, 0 }, { 1, 1, 2, 2, 1, 2 });

    const kiriview::AnimationProviderChurnGateResult result
        = kiriview::evaluateAnimationProviderChurnGate(sample);

    QVERIFY(result.passed());
    QCOMPARE(sample.providerRequestLatencyP95Ms, 2);
    QCOMPARE(sample.providerRequestLatencyMaxMs, 2);
    QCOMPARE(sample.maximumFrameScheduleDriftMs, 2);
}

void TestImageAnimationPolicy::providerRevisionPresenterRejectsEachGateFailure()
{
    const kiriview::AnimationProviderChurnGateSample passingSample {
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
        kiriview::AnimationProviderChurnGateSample sample = passingSample;
        sample.timerWaitsForProviderLoad = true;
        const kiriview::AnimationProviderChurnGateResult result
            = kiriview::evaluateAnimationProviderChurnGate(sample);
        QVERIFY(!result.passed());
        QCOMPARE(result.failure, kiriview::AnimationProviderChurnGateFailure::FramePacing);
    }

    {
        kiriview::AnimationProviderChurnGateSample sample = passingSample;
        sample.providerRequestLatencyP95Ms = 3;
        const kiriview::AnimationProviderChurnGateResult result
            = kiriview::evaluateAnimationProviderChurnGate(sample);
        QVERIFY(!result.passed());
        QCOMPARE(
            result.failure, kiriview::AnimationProviderChurnGateFailure::ProviderRequestLatency);
    }

    {
        kiriview::AnimationProviderChurnGateSample sample = passingSample;
        sample.staleLoadOutcomeRejected = false;
        const kiriview::AnimationProviderChurnGateResult result
            = kiriview::evaluateAnimationProviderChurnGate(sample);
        QVERIFY(!result.passed());
        QCOMPARE(result.failure, kiriview::AnimationProviderChurnGateFailure::StaleFrameRejection);
    }

    {
        kiriview::AnimationProviderChurnGateSample sample = passingSample;
        sample.retainedFrameEntriesPerPageRole = 3;
        const kiriview::AnimationProviderChurnGateResult result
            = kiriview::evaluateAnimationProviderChurnGate(sample);
        QVERIFY(!result.passed());
        QCOMPARE(result.failure, kiriview::AnimationProviderChurnGateFailure::MemoryRetention);
    }

    {
        kiriview::AnimationProviderChurnGateSample sample = passingSample;
        sample.providerUrlsPerAcceptedFrame = 2;
        const kiriview::AnimationProviderChurnGateResult result
            = kiriview::evaluateAnimationProviderChurnGate(sample);
        QVERIFY(!result.passed());
        QCOMPARE(result.failure, kiriview::AnimationProviderChurnGateFailure::UrlChurn);
    }
}

QTEST_GUILESS_MAIN(TestImageAnimationPolicy)

#include "test_imageanimationpolicy.moc"
