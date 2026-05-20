// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecode/predecodeschedulestate.h"

#include "image_test_support.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
KiriView::PredecodeScheduleContext scheduleContext(const QUrl &url, int pageIndex = 0)
{
    return KiriView::PredecodeScheduleContext {
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromUrl(url),
            false,
            KiriView::TestSupport::staticTestImagePayload(KiriView::TestSupport::testImage()),
        },
        std::nullopt,
        KiriView::ImageFirstDisplayDecodeContext {},
        pageIndex,
    };
}
}

class TestPredecodeScheduleState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void schedulePublishesDisplayedContextAndPendingGeneration();
    void invalidScheduleCancelsPendingWithoutReplacingDisplayedContext();
    void powerSaverSuppressesPendingScheduleButKeepsDisplayedContext();
    void disablingPowerSaverReschedulesDisplayedContext();
    void cancelClearsDisplayedContextAndMomentum();
    void settledNeutralScheduleReissuesPendingGeneration();
};

void TestPredecodeScheduleState::schedulePublishesDisplayedContextAndPendingGeneration()
{
    KiriView::PredecodeScheduleState state;
    const QUrl firstUrl = KiriView::TestSupport::indexedImageUrl(1);

    const KiriView::PredecodeScheduleEffectPlan update
        = state.schedule(scheduleContext(firstUrl, 1), 1000);

    QVERIFY(update.cancelBackgroundEffects);
    QVERIFY(update.cacheDisplayedContext.has_value());
    QCOMPARE(update.cacheDisplayedContext->primaryImage.location.imageUrl(), firstUrl);
    QVERIFY(update.pendingSchedule.has_value());
    QVERIFY(update.startDebounceTimers);
    QVERIFY(!update.clearWindowUrls);
    QCOMPARE(update.pendingSchedule->context.primaryImage.location.imageUrl(), firstUrl);
    QVERIFY(state.accepts(update.pendingSchedule->generation));

    const std::optional<KiriView::PredecodePendingSchedule> pending
        = state.pendingDebouncedSchedule();
    QVERIFY(pending.has_value());
    QCOMPARE(pending->generation, update.pendingSchedule->generation);
}

void TestPredecodeScheduleState::invalidScheduleCancelsPendingWithoutReplacingDisplayedContext()
{
    KiriView::PredecodeScheduleState state;
    const QUrl displayedUrl = KiriView::TestSupport::indexedImageUrl(2);
    const KiriView::PredecodeScheduleEffectPlan update
        = state.schedule(scheduleContext(displayedUrl, 2), 1000);
    QVERIFY(update.pendingSchedule.has_value());

    const KiriView::PredecodeScheduleEffectPlan invalidUpdate
        = state.schedule(KiriView::PredecodeScheduleContext {}, 1200);
    QVERIFY(invalidUpdate.cancelBackgroundEffects);
    QVERIFY(!invalidUpdate.cacheDisplayedContext.has_value());
    QVERIFY(!invalidUpdate.pendingSchedule.has_value());
    QVERIFY(!invalidUpdate.startDebounceTimers);
    QVERIFY(!state.pendingDebouncedSchedule().has_value());
    QVERIFY(!state.accepts(update.pendingSchedule->generation));

    const std::optional<KiriView::PredecodeScheduleContext> displayed = state.displayedContext();
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->primaryImage.location.imageUrl(), displayedUrl);
}

void TestPredecodeScheduleState::powerSaverSuppressesPendingScheduleButKeepsDisplayedContext()
{
    KiriView::PredecodeScheduleState state;
    const KiriView::PredecodeScheduleEffectPlan enablePlan = state.setPowerSaverEnabled(true, 900);
    QVERIFY(enablePlan.cancelBackgroundEffects);
    QVERIFY(enablePlan.clearWindowUrls);
    const QUrl displayedUrl = KiriView::TestSupport::indexedImageUrl(3);

    const KiriView::PredecodeScheduleEffectPlan update
        = state.schedule(scheduleContext(displayedUrl, 3), 1000);

    QVERIFY(update.cancelBackgroundEffects);
    QVERIFY(update.cacheDisplayedContext.has_value());
    QVERIFY(update.clearWindowUrls);
    QVERIFY(!update.pendingSchedule.has_value());
    QVERIFY(!update.startDebounceTimers);
    QVERIFY(!state.pendingDebouncedSchedule().has_value());

    const std::optional<KiriView::PredecodeScheduleContext> displayed = state.displayedContext();
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->primaryImage.location.imageUrl(), displayedUrl);
}

void TestPredecodeScheduleState::disablingPowerSaverReschedulesDisplayedContext()
{
    KiriView::PredecodeScheduleState state;
    state.setPowerSaverEnabled(true, 900);
    const QUrl displayedUrl = KiriView::TestSupport::indexedImageUrl(10);
    const KiriView::PredecodeScheduleEffectPlan suppressed
        = state.schedule(scheduleContext(displayedUrl, 10), 1000);
    QVERIFY(suppressed.clearWindowUrls);
    QVERIFY(!suppressed.pendingSchedule.has_value());

    const KiriView::PredecodeScheduleEffectPlan resumed = state.setPowerSaverEnabled(false, 1200);

    QVERIFY(resumed.cancelBackgroundEffects);
    QVERIFY(resumed.cacheDisplayedContext.has_value());
    QCOMPARE(resumed.cacheDisplayedContext->primaryImage.location.imageUrl(), displayedUrl);
    QVERIFY(resumed.pendingSchedule.has_value());
    QVERIFY(resumed.startDebounceTimers);
    QVERIFY(!resumed.clearWindowUrls);
    QVERIFY(state.accepts(resumed.pendingSchedule->generation));
}

void TestPredecodeScheduleState::cancelClearsDisplayedContextAndMomentum()
{
    KiriView::PredecodeScheduleState state;
    state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(4), 4), 1000);
    state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(5), 5), 1300);
    state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(6), 6), 1600);
    QVERIFY(state.momentumMode() == KiriView::PredecodeMomentumMode::NextBiased);

    state.cancel();

    QVERIFY(!state.displayedContext().has_value());
    QVERIFY(!state.pendingDebouncedSchedule().has_value());
    QVERIFY(state.momentumMode() == KiriView::PredecodeMomentumMode::Neutral);
}

void TestPredecodeScheduleState::settledNeutralScheduleReissuesPendingGeneration()
{
    KiriView::PredecodeScheduleState state;
    state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(7), 7), 1000);
    state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(8), 8), 1300);
    const KiriView::PredecodeScheduleEffectPlan biasedUpdate
        = state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(9), 9), 1600);
    QVERIFY(biasedUpdate.pendingSchedule.has_value());
    const quint64 biasedGeneration = biasedUpdate.pendingSchedule->generation;
    QVERIFY(state.momentumMode() == KiriView::PredecodeMomentumMode::NextBiased);

    const KiriView::PredecodeScheduleEffectPlan settled = state.settlePendingScheduleToNeutral();

    QVERIFY(settled.cancelBackgroundEffects);
    QVERIFY(settled.pendingSchedule.has_value());
    QVERIFY(!settled.startDebounceTimers);
    QVERIFY(state.momentumMode() == KiriView::PredecodeMomentumMode::Neutral);
    QVERIFY(settled.pendingSchedule->generation != biasedGeneration);
    QVERIFY(!state.accepts(biasedGeneration));
    QVERIFY(state.accepts(settled.pendingSchedule->generation));
}

QTEST_GUILESS_MAIN(TestPredecodeScheduleState)

#include "test_predecodeschedulestate.moc"
