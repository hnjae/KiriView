// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecode/predecodeschedulestate.h"

#include "image_test_support.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

namespace {
struct TestPredecodeSchedulePayload final : KiriView::PredecodeSchedulePayload {
    int marker = 0;
};

std::shared_ptr<const KiriView::PredecodeSchedulePayload> testPayload(int marker)
{
    auto payload = std::make_shared<TestPredecodeSchedulePayload>();
    payload->marker = marker;
    return payload;
}

KiriView::PredecodeScheduleContext scheduleContext(const QUrl &url, int pageIndex = 0)
{
    KiriView::DisplayedPredecodeImage displayedImage {
        KiriView::DisplayedImageLocation::fromUrl(url),
        false,
        KiriView::TestSupport::staticTestImagePayload(KiriView::TestSupport::testImage()),
    };
    return KiriView::PredecodeScheduleContext {
        displayedImage.location,
        { std::move(displayedImage) },
        KiriView::ImageFirstDisplayDecodeContext {},
        pageIndex,
        {},
    };
}

const TestPredecodeSchedulePayload *testPayload(const KiriView::PredecodeScheduleContext &context)
{
    return KiriView::predecodeSchedulePayload<TestPredecodeSchedulePayload>(context);
}

int payloadMarker(const KiriView::PredecodeScheduleContext &context)
{
    const TestPredecodeSchedulePayload *payload = testPayload(context);
    return payload == nullptr ? 0 : payload->marker;
}

template <typename Operation>
const Operation *operationAt(const KiriView::PredecodeScheduleRuntimePlan &plan, std::size_t index)
{
    if (index >= plan.size()) {
        return nullptr;
    }

    return std::get_if<Operation>(&plan.at(index));
}
}

class TestPredecodeScheduleState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void schedulePublishesCurrentContextAndPendingGeneration();
    void invalidScheduleCancelsPendingAndClearsCurrentContext();
    void scheduleAcceptsCursorWithoutDisplayedImages();
    void pendingScheduleCarriesPayload();
    void powerSaverSuppressesPendingScheduleButKeepsCurrentContext();
    void disablingPowerSaverReschedulesCurrentContext();
    void cancelClearsCurrentContextAndMomentum();
    void settledNeutralScheduleReissuesPendingGeneration();
};

void TestPredecodeScheduleState::schedulePublishesCurrentContextAndPendingGeneration()
{
    KiriView::PredecodeScheduleState state;
    const QUrl firstUrl = KiriView::TestSupport::indexedImageUrl(1);

    const KiriView::PredecodeScheduleRuntimePlan update
        = state.schedule(scheduleContext(firstUrl, 1), 1000);

    QCOMPARE(update.size(), std::size_t(3));
    QVERIFY(operationAt<KiriView::CancelBackgroundPredecodeOperation>(update, 0) != nullptr);
    const auto *cacheOperation
        = operationAt<KiriView::CacheDisplayedPredecodeContextOperation>(update, 1);
    QVERIFY(cacheOperation != nullptr);
    QCOMPARE(cacheOperation->images.size(), std::size_t(1));
    QCOMPARE(cacheOperation->images.front().location.imageUrl(), firstUrl);
    const auto *debounceOperation
        = operationAt<KiriView::StartPredecodeDebounceOperation>(update, 2);
    QVERIFY(debounceOperation != nullptr);
    QCOMPARE(debounceOperation->schedule.context.currentLocation.imageUrl(), firstUrl);
    QVERIFY(state.accepts(debounceOperation->schedule.generation));

    const std::optional<KiriView::PredecodePendingSchedule> pending
        = state.pendingDebouncedSchedule();
    QVERIFY(pending.has_value());
    QCOMPARE(pending->generation, debounceOperation->schedule.generation);
}

void TestPredecodeScheduleState::invalidScheduleCancelsPendingAndClearsCurrentContext()
{
    KiriView::PredecodeScheduleState state;
    const KiriView::PredecodeScheduleRuntimePlan update
        = state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(2), 2), 1000);
    const auto *debounceOperation
        = operationAt<KiriView::StartPredecodeDebounceOperation>(update, 2);
    QVERIFY(debounceOperation != nullptr);

    const KiriView::PredecodeScheduleRuntimePlan invalidUpdate
        = state.schedule(KiriView::PredecodeScheduleContext {}, 1200);
    QCOMPARE(invalidUpdate.size(), std::size_t(1));
    QVERIFY(operationAt<KiriView::CancelBackgroundPredecodeOperation>(invalidUpdate, 0) != nullptr);
    QVERIFY(!state.pendingDebouncedSchedule().has_value());
    QVERIFY(!state.accepts(debounceOperation->schedule.generation));
    QVERIFY(!state.currentContext().has_value());
}

void TestPredecodeScheduleState::scheduleAcceptsCursorWithoutDisplayedImages()
{
    KiriView::PredecodeScheduleState state;
    const QUrl cursorUrl = KiriView::TestSupport::indexedImageUrl(11);

    const KiriView::PredecodeScheduleRuntimePlan update = state.schedule(
        KiriView::PredecodeScheduleContext {
            KiriView::DisplayedImageLocation::fromUrl(cursorUrl),
            {},
            KiriView::ImageFirstDisplayDecodeContext {},
            11,
        },
        1000);

    QCOMPARE(update.size(), std::size_t(3));
    QVERIFY(operationAt<KiriView::CancelBackgroundPredecodeOperation>(update, 0) != nullptr);
    const auto *cacheOperation
        = operationAt<KiriView::CacheDisplayedPredecodeContextOperation>(update, 1);
    QVERIFY(cacheOperation != nullptr);
    QVERIFY(cacheOperation->images.empty());
    const auto *debounceOperation
        = operationAt<KiriView::StartPredecodeDebounceOperation>(update, 2);
    QVERIFY(debounceOperation != nullptr);
    QCOMPARE(debounceOperation->schedule.context.currentLocation.imageUrl(), cursorUrl);
    QVERIFY(state.accepts(debounceOperation->schedule.generation));
}

void TestPredecodeScheduleState::pendingScheduleCarriesPayload()
{
    KiriView::PredecodeScheduleState state;
    const QUrl cursorUrl = KiriView::TestSupport::indexedImageUrl(12);

    KiriView::PredecodeScheduleContext context = scheduleContext(cursorUrl, 12);
    context.payload = testPayload(42);

    const KiriView::PredecodeScheduleRuntimePlan update = state.schedule(std::move(context), 1000);

    const auto *debounceOperation
        = operationAt<KiriView::StartPredecodeDebounceOperation>(update, 2);
    QVERIFY(debounceOperation != nullptr);
    QCOMPARE(payloadMarker(debounceOperation->schedule.context), 42);

    const std::optional<KiriView::PredecodePendingSchedule> pending
        = state.pendingDebouncedSchedule();
    QVERIFY(pending.has_value());
    QCOMPARE(payloadMarker(pending->context), 42);
}

void TestPredecodeScheduleState::powerSaverSuppressesPendingScheduleButKeepsCurrentContext()
{
    KiriView::PredecodeScheduleState state;
    const KiriView::PredecodeScheduleRuntimePlan enablePlan = state.setPowerSaverEnabled(true, 900);
    QCOMPARE(enablePlan.size(), std::size_t(2));
    QVERIFY(operationAt<KiriView::CancelBackgroundPredecodeOperation>(enablePlan, 0) != nullptr);
    QVERIFY(operationAt<KiriView::ClearPredecodeWindowUrlsOperation>(enablePlan, 1) != nullptr);
    const QUrl displayedUrl = KiriView::TestSupport::indexedImageUrl(3);

    const KiriView::PredecodeScheduleRuntimePlan update
        = state.schedule(scheduleContext(displayedUrl, 3), 1000);

    QCOMPARE(update.size(), std::size_t(3));
    QVERIFY(operationAt<KiriView::CancelBackgroundPredecodeOperation>(update, 0) != nullptr);
    const auto *cacheOperation
        = operationAt<KiriView::CacheDisplayedPredecodeContextOperation>(update, 1);
    QVERIFY(cacheOperation != nullptr);
    QVERIFY(operationAt<KiriView::ClearPredecodeWindowUrlsOperation>(update, 2) != nullptr);
    QVERIFY(!state.pendingDebouncedSchedule().has_value());

    const std::optional<KiriView::PredecodeScheduleContext> current = state.currentContext();
    QVERIFY(current.has_value());
    QCOMPARE(current->currentLocation.imageUrl(), displayedUrl);
}

void TestPredecodeScheduleState::disablingPowerSaverReschedulesCurrentContext()
{
    KiriView::PredecodeScheduleState state;
    state.setPowerSaverEnabled(true, 900);
    const QUrl displayedUrl = KiriView::TestSupport::indexedImageUrl(10);
    KiriView::PredecodeScheduleContext context = scheduleContext(displayedUrl, 10);
    context.payload = testPayload(77);
    const KiriView::PredecodeScheduleRuntimePlan suppressed
        = state.schedule(std::move(context), 1000);
    QVERIFY(operationAt<KiriView::ClearPredecodeWindowUrlsOperation>(suppressed, 2) != nullptr);
    QVERIFY(!state.pendingDebouncedSchedule().has_value());

    const KiriView::PredecodeScheduleRuntimePlan resumed = state.setPowerSaverEnabled(false, 1200);

    QCOMPARE(resumed.size(), std::size_t(3));
    QVERIFY(operationAt<KiriView::CancelBackgroundPredecodeOperation>(resumed, 0) != nullptr);
    const auto *cacheOperation
        = operationAt<KiriView::CacheDisplayedPredecodeContextOperation>(resumed, 1);
    QVERIFY(cacheOperation != nullptr);
    QCOMPARE(cacheOperation->images.size(), std::size_t(1));
    QCOMPARE(cacheOperation->images.front().location.imageUrl(), displayedUrl);
    const auto *debounceOperation
        = operationAt<KiriView::StartPredecodeDebounceOperation>(resumed, 2);
    QVERIFY(debounceOperation != nullptr);
    QCOMPARE(payloadMarker(debounceOperation->schedule.context), 77);
    QVERIFY(state.accepts(debounceOperation->schedule.generation));
}

void TestPredecodeScheduleState::cancelClearsCurrentContextAndMomentum()
{
    KiriView::PredecodeScheduleState state;
    state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(4), 4), 1000);
    state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(5), 5), 1300);
    state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(6), 6), 1600);
    QVERIFY(state.momentumMode() == KiriView::PredecodeMomentumMode::NextBiased);

    state.cancel();

    QVERIFY(!state.currentContext().has_value());
    QVERIFY(!state.pendingDebouncedSchedule().has_value());
    QVERIFY(state.momentumMode() == KiriView::PredecodeMomentumMode::Neutral);
}

void TestPredecodeScheduleState::settledNeutralScheduleReissuesPendingGeneration()
{
    KiriView::PredecodeScheduleState state;
    state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(7), 7), 1000);
    state.schedule(scheduleContext(KiriView::TestSupport::indexedImageUrl(8), 8), 1300);
    KiriView::PredecodeScheduleContext biasedContext
        = scheduleContext(KiriView::TestSupport::indexedImageUrl(9), 9);
    biasedContext.payload = testPayload(99);
    const KiriView::PredecodeScheduleRuntimePlan biasedUpdate
        = state.schedule(std::move(biasedContext), 1600);
    const auto *biasedDebounceOperation
        = operationAt<KiriView::StartPredecodeDebounceOperation>(biasedUpdate, 2);
    QVERIFY(biasedDebounceOperation != nullptr);
    const quint64 biasedGeneration = biasedDebounceOperation->schedule.generation;
    QVERIFY(state.momentumMode() == KiriView::PredecodeMomentumMode::NextBiased);

    const KiriView::PredecodeScheduleRuntimePlan settled = state.settlePendingScheduleToNeutral();

    QCOMPARE(settled.size(), std::size_t(2));
    QVERIFY(operationAt<KiriView::CancelBackgroundPredecodeOperation>(settled, 0) != nullptr);
    const auto *startOperation = operationAt<KiriView::StartAdjacentPredecodeOperation>(settled, 1);
    QVERIFY(startOperation != nullptr);
    QCOMPARE(payloadMarker(startOperation->schedule.context), 99);
    QVERIFY(state.momentumMode() == KiriView::PredecodeMomentumMode::Neutral);
    QVERIFY(startOperation->schedule.generation != biasedGeneration);
    QVERIFY(!state.accepts(biasedGeneration));
    QVERIFY(state.accepts(startOperation->schedule.generation));
}

QTEST_GUILESS_MAIN(TestPredecodeScheduleState)

#include "test_predecodeschedulestate.moc"
