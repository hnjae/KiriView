// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodeschedulestate.h"

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
    void cancelClearsDisplayedContextAndMomentum();
    void settledNeutralScheduleReissuesPendingGeneration();
};

void TestPredecodeScheduleState::schedulePublishesDisplayedContextAndPendingGeneration()
{
    KiriView::PredecodeScheduleState state;
    const QUrl firstUrl = KiriView::TestSupport::indexedImageUrl(1);

    const std::optional<KiriView::PredecodeScheduleUpdate> update
        = state.schedule(scheduleContext(firstUrl, 1), 1000);

    QVERIFY(update.has_value());
    QCOMPARE(update->context.primaryImage.location.imageUrl(), firstUrl);
    QVERIFY(update->pendingSchedule.has_value());
    QCOMPARE(update->pendingSchedule->context.primaryImage.location.imageUrl(), firstUrl);
    QVERIFY(state.accepts(update->pendingSchedule->generation));

    const std::optional<KiriView::PredecodePendingSchedule> pending
        = state.pendingDebouncedSchedule();
    QVERIFY(pending.has_value());
    QCOMPARE(pending->generation, update->pendingSchedule->generation);
}

void TestPredecodeScheduleState::invalidScheduleCancelsPendingWithoutReplacingDisplayedContext()
{
    KiriView::PredecodeScheduleState state;
    const QUrl displayedUrl = KiriView::TestSupport::indexedImageUrl(2);
    const std::optional<KiriView::PredecodeScheduleUpdate> update
        = state.schedule(scheduleContext(displayedUrl, 2), 1000);
    QVERIFY(update.has_value());
    QVERIFY(update->pendingSchedule.has_value());

    QVERIFY(!state.schedule(KiriView::PredecodeScheduleContext {}, 1200).has_value());
    QVERIFY(!state.pendingDebouncedSchedule().has_value());
    QVERIFY(!state.accepts(update->pendingSchedule->generation));

    const std::optional<KiriView::PredecodeScheduleContext> displayed = state.displayedContext();
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->primaryImage.location.imageUrl(), displayedUrl);
}

void TestPredecodeScheduleState::powerSaverSuppressesPendingScheduleButKeepsDisplayedContext()
{
    KiriView::PredecodeScheduleState state;
    QVERIFY(state.setPowerSaverEnabled(true));
    const QUrl displayedUrl = KiriView::TestSupport::indexedImageUrl(3);

    const std::optional<KiriView::PredecodeScheduleUpdate> update
        = state.schedule(scheduleContext(displayedUrl, 3), 1000);

    QVERIFY(update.has_value());
    QVERIFY(update->powerSaverEnabled);
    QVERIFY(!update->pendingSchedule.has_value());
    QVERIFY(!state.pendingDebouncedSchedule().has_value());

    const std::optional<KiriView::PredecodeScheduleContext> displayed = state.displayedContext();
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->primaryImage.location.imageUrl(), displayedUrl);
}

void TestPredecodeScheduleState::cancelClearsDisplayedContextAndMomentum()
{
    KiriView::PredecodeScheduleState state;
    QVERIFY(state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(4), 4), 1000)
            .has_value());
    QVERIFY(state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(5), 5), 1300)
            .has_value());
    QVERIFY(state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(6), 6), 1600)
            .has_value());
    QVERIFY(state.momentumMode() == KiriView::PredecodeMomentumMode::NextBiased);

    state.cancel();

    QVERIFY(!state.displayedContext().has_value());
    QVERIFY(!state.pendingDebouncedSchedule().has_value());
    QVERIFY(state.momentumMode() == KiriView::PredecodeMomentumMode::Neutral);
}

void TestPredecodeScheduleState::settledNeutralScheduleReissuesPendingGeneration()
{
    KiriView::PredecodeScheduleState state;
    QVERIFY(state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(7), 7), 1000)
            .has_value());
    QVERIFY(state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(8), 8), 1300)
            .has_value());
    const std::optional<KiriView::PredecodeScheduleUpdate> biasedUpdate
        = state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(9), 9), 1600);
    QVERIFY(biasedUpdate.has_value());
    QVERIFY(biasedUpdate->pendingSchedule.has_value());
    const quint64 biasedGeneration = biasedUpdate->pendingSchedule->generation;
    QVERIFY(state.momentumMode() == KiriView::PredecodeMomentumMode::NextBiased);

    const std::optional<KiriView::PredecodePendingSchedule> settled
        = state.settlePendingScheduleToNeutral();

    QVERIFY(settled.has_value());
    QVERIFY(state.momentumMode() == KiriView::PredecodeMomentumMode::Neutral);
    QVERIFY(settled->generation != biasedGeneration);
    QVERIFY(!state.accepts(biasedGeneration));
    QVERIFY(state.accepts(settled->generation));
}

QTEST_GUILESS_MAIN(TestPredecodeScheduleState)

#include "test_predecodeschedulestate.moc"
