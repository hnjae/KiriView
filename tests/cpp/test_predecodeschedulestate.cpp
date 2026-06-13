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
struct TestPredecodeSchedulePayload final : kiriview::PredecodeSchedulePayload {
    int marker = 0;
};

std::shared_ptr<const kiriview::PredecodeSchedulePayload> testPayload(int marker)
{
    auto payload = std::make_shared<TestPredecodeSchedulePayload>();
    payload->marker = marker;
    return payload;
}

kiriview::PredecodeScheduleContext scheduleContext(const QUrl &url, int pageIndex = 0)
{
    kiriview::DisplayedPredecodeImage displayedImage {
        kiriview::DisplayedImageLocation::fromUrl(url),
        false,
        kiriview::TestSupport::staticDisplayTestImagePayload(kiriview::TestSupport::testImage()),
    };
    return kiriview::PredecodeScheduleContext {
        displayedImage.location,
        { std::move(displayedImage) },
        kiriview::ImageFirstDisplayDecodeContext {},
        pageIndex,
        {},
    };
}

const TestPredecodeSchedulePayload *testPayload(const kiriview::PredecodeScheduleContext &context)
{
    return kiriview::predecodeSchedulePayload<TestPredecodeSchedulePayload>(context);
}

int payloadMarker(const kiriview::PredecodeScheduleContext &context)
{
    const TestPredecodeSchedulePayload *payload = testPayload(context);
    return payload == nullptr ? 0 : payload->marker;
}

template <typename Operation>
const Operation *operationAt(const kiriview::PredecodeScheduleRuntimePlan &plan, std::size_t index)
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
    kiriview::PredecodeScheduleState state;
    const QUrl firstUrl = kiriview::TestSupport::indexedImageUrl(1);

    const kiriview::PredecodeScheduleRuntimePlan update
        = state.schedule(scheduleContext(firstUrl, 1), 1000);

    QCOMPARE(update.size(), std::size_t(3));
    QVERIFY(operationAt<kiriview::CancelBackgroundPredecodeOperation>(update, 0) != nullptr);
    const auto *cacheOperation
        = operationAt<kiriview::CacheDisplayedPredecodeContextOperation>(update, 1);
    QVERIFY(cacheOperation != nullptr);
    QCOMPARE(cacheOperation->images.size(), std::size_t(1));
    QCOMPARE(cacheOperation->images.front().location.imageUrl(), firstUrl);
    const auto *debounceOperation
        = operationAt<kiriview::StartPredecodeDebounceOperation>(update, 2);
    QVERIFY(debounceOperation != nullptr);
    QCOMPARE(debounceOperation->schedule.context.currentLocation.imageUrl(), firstUrl);
    QVERIFY(state.accepts(debounceOperation->schedule.generation));

    const std::optional<kiriview::PredecodePendingSchedule> pending
        = state.pendingDebouncedSchedule();
    QVERIFY(pending.has_value());
    QCOMPARE(pending->generation, debounceOperation->schedule.generation);
}

void TestPredecodeScheduleState::invalidScheduleCancelsPendingAndClearsCurrentContext()
{
    kiriview::PredecodeScheduleState state;
    const kiriview::PredecodeScheduleRuntimePlan update
        = state.schedule(scheduleContext(kiriview::TestSupport::indexedImageUrl(2), 2), 1000);
    const auto *debounceOperation
        = operationAt<kiriview::StartPredecodeDebounceOperation>(update, 2);
    QVERIFY(debounceOperation != nullptr);

    const kiriview::PredecodeScheduleRuntimePlan invalidUpdate
        = state.schedule(kiriview::PredecodeScheduleContext {}, 1200);
    QCOMPARE(invalidUpdate.size(), std::size_t(1));
    QVERIFY(operationAt<kiriview::CancelBackgroundPredecodeOperation>(invalidUpdate, 0) != nullptr);
    QVERIFY(!state.pendingDebouncedSchedule().has_value());
    QVERIFY(!state.accepts(debounceOperation->schedule.generation));
    QVERIFY(!state.currentContext().has_value());
}

void TestPredecodeScheduleState::scheduleAcceptsCursorWithoutDisplayedImages()
{
    kiriview::PredecodeScheduleState state;
    const QUrl cursorUrl = kiriview::TestSupport::indexedImageUrl(11);

    const kiriview::PredecodeScheduleRuntimePlan update = state.schedule(
        kiriview::PredecodeScheduleContext {
            kiriview::DisplayedImageLocation::fromUrl(cursorUrl),
            {},
            kiriview::ImageFirstDisplayDecodeContext {},
            11,
        },
        1000);

    QCOMPARE(update.size(), std::size_t(3));
    QVERIFY(operationAt<kiriview::CancelBackgroundPredecodeOperation>(update, 0) != nullptr);
    const auto *cacheOperation
        = operationAt<kiriview::CacheDisplayedPredecodeContextOperation>(update, 1);
    QVERIFY(cacheOperation != nullptr);
    QVERIFY(cacheOperation->images.empty());
    const auto *debounceOperation
        = operationAt<kiriview::StartPredecodeDebounceOperation>(update, 2);
    QVERIFY(debounceOperation != nullptr);
    QCOMPARE(debounceOperation->schedule.context.currentLocation.imageUrl(), cursorUrl);
    QVERIFY(state.accepts(debounceOperation->schedule.generation));
}

void TestPredecodeScheduleState::pendingScheduleCarriesPayload()
{
    kiriview::PredecodeScheduleState state;
    const QUrl cursorUrl = kiriview::TestSupport::indexedImageUrl(12);

    kiriview::PredecodeScheduleContext context = scheduleContext(cursorUrl, 12);
    context.payload = testPayload(42);

    const kiriview::PredecodeScheduleRuntimePlan update = state.schedule(std::move(context), 1000);

    const auto *debounceOperation
        = operationAt<kiriview::StartPredecodeDebounceOperation>(update, 2);
    QVERIFY(debounceOperation != nullptr);
    QCOMPARE(payloadMarker(debounceOperation->schedule.context), 42);

    const std::optional<kiriview::PredecodePendingSchedule> pending
        = state.pendingDebouncedSchedule();
    QVERIFY(pending.has_value());
    QCOMPARE(payloadMarker(pending->context), 42);
}

void TestPredecodeScheduleState::powerSaverSuppressesPendingScheduleButKeepsCurrentContext()
{
    kiriview::PredecodeScheduleState state;
    const kiriview::PredecodeScheduleRuntimePlan enablePlan = state.setPowerSaverEnabled(true, 900);
    QCOMPARE(enablePlan.size(), std::size_t(2));
    QVERIFY(operationAt<kiriview::CancelBackgroundPredecodeOperation>(enablePlan, 0) != nullptr);
    QVERIFY(operationAt<kiriview::ClearPredecodeWindowUrlsOperation>(enablePlan, 1) != nullptr);
    const QUrl displayedUrl = kiriview::TestSupport::indexedImageUrl(3);

    const kiriview::PredecodeScheduleRuntimePlan update
        = state.schedule(scheduleContext(displayedUrl, 3), 1000);

    QCOMPARE(update.size(), std::size_t(3));
    QVERIFY(operationAt<kiriview::CancelBackgroundPredecodeOperation>(update, 0) != nullptr);
    const auto *cacheOperation
        = operationAt<kiriview::CacheDisplayedPredecodeContextOperation>(update, 1);
    QVERIFY(cacheOperation != nullptr);
    QVERIFY(operationAt<kiriview::ClearPredecodeWindowUrlsOperation>(update, 2) != nullptr);
    QVERIFY(!state.pendingDebouncedSchedule().has_value());

    const std::optional<kiriview::PredecodeScheduleContext> current = state.currentContext();
    QVERIFY(current.has_value());
    QCOMPARE(current->currentLocation.imageUrl(), displayedUrl);
}

void TestPredecodeScheduleState::disablingPowerSaverReschedulesCurrentContext()
{
    kiriview::PredecodeScheduleState state;
    state.setPowerSaverEnabled(true, 900);
    const QUrl displayedUrl = kiriview::TestSupport::indexedImageUrl(10);
    kiriview::PredecodeScheduleContext context = scheduleContext(displayedUrl, 10);
    context.payload = testPayload(77);
    const kiriview::PredecodeScheduleRuntimePlan suppressed
        = state.schedule(std::move(context), 1000);
    QVERIFY(operationAt<kiriview::ClearPredecodeWindowUrlsOperation>(suppressed, 2) != nullptr);
    QVERIFY(!state.pendingDebouncedSchedule().has_value());

    const kiriview::PredecodeScheduleRuntimePlan resumed = state.setPowerSaverEnabled(false, 1200);

    QCOMPARE(resumed.size(), std::size_t(3));
    QVERIFY(operationAt<kiriview::CancelBackgroundPredecodeOperation>(resumed, 0) != nullptr);
    const auto *cacheOperation
        = operationAt<kiriview::CacheDisplayedPredecodeContextOperation>(resumed, 1);
    QVERIFY(cacheOperation != nullptr);
    QCOMPARE(cacheOperation->images.size(), std::size_t(1));
    QCOMPARE(cacheOperation->images.front().location.imageUrl(), displayedUrl);
    const auto *debounceOperation
        = operationAt<kiriview::StartPredecodeDebounceOperation>(resumed, 2);
    QVERIFY(debounceOperation != nullptr);
    QCOMPARE(payloadMarker(debounceOperation->schedule.context), 77);
    QVERIFY(state.accepts(debounceOperation->schedule.generation));
}

void TestPredecodeScheduleState::cancelClearsCurrentContextAndMomentum()
{
    kiriview::PredecodeScheduleState state;
    state.schedule(scheduleContext(kiriview::TestSupport::indexedImageUrl(4), 4), 1000);
    state.schedule(scheduleContext(kiriview::TestSupport::indexedImageUrl(5), 5), 1300);
    state.schedule(scheduleContext(kiriview::TestSupport::indexedImageUrl(6), 6), 1600);
    QVERIFY(state.momentumMode() == kiriview::PredecodeMomentumMode::NextBiased);

    state.cancel();

    QVERIFY(!state.currentContext().has_value());
    QVERIFY(!state.pendingDebouncedSchedule().has_value());
    QVERIFY(state.momentumMode() == kiriview::PredecodeMomentumMode::Neutral);
}

void TestPredecodeScheduleState::settledNeutralScheduleReissuesPendingGeneration()
{
    kiriview::PredecodeScheduleState state;
    state.schedule(scheduleContext(kiriview::TestSupport::indexedImageUrl(7), 7), 1000);
    state.schedule(scheduleContext(kiriview::TestSupport::indexedImageUrl(8), 8), 1300);
    kiriview::PredecodeScheduleContext biasedContext
        = scheduleContext(kiriview::TestSupport::indexedImageUrl(9), 9);
    biasedContext.payload = testPayload(99);
    const kiriview::PredecodeScheduleRuntimePlan biasedUpdate
        = state.schedule(std::move(biasedContext), 1600);
    const auto *biasedDebounceOperation
        = operationAt<kiriview::StartPredecodeDebounceOperation>(biasedUpdate, 2);
    QVERIFY(biasedDebounceOperation != nullptr);
    const quint64 biasedGeneration = biasedDebounceOperation->schedule.generation;
    QVERIFY(state.momentumMode() == kiriview::PredecodeMomentumMode::NextBiased);

    const kiriview::PredecodeScheduleRuntimePlan settled = state.settlePendingScheduleToNeutral();

    QCOMPARE(settled.size(), std::size_t(2));
    QVERIFY(operationAt<kiriview::CancelBackgroundPredecodeOperation>(settled, 0) != nullptr);
    const auto *startOperation = operationAt<kiriview::StartAdjacentPredecodeOperation>(settled, 1);
    QVERIFY(startOperation != nullptr);
    QCOMPARE(payloadMarker(startOperation->schedule.context), 99);
    QVERIFY(state.momentumMode() == kiriview::PredecodeMomentumMode::Neutral);
    QVERIFY(startOperation->schedule.generation != biasedGeneration);
    QVERIFY(!state.accepts(biasedGeneration));
    QVERIFY(state.accepts(startOperation->schedule.generation));
}

QTEST_GUILESS_MAIN(TestPredecodeScheduleState)

#include "test_predecodeschedulestate.moc"
