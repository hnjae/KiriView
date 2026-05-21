// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/imagetiledecodestate.h"

#include <QObject>
#include <QTest>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace {
KiriView::TileRequest tileRequest(const KiriView::TileKey &key)
{
    KiriView::TileRequest request;
    request.key = key;
    return request;
}
}

class TestImageTileDecodeState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void surfaceChangeOwnsGenerationAndClearsRequestState();
    void requestStartCommitsOnlyAcceptedScheduleWork();
    void invalidateRejectsPendingRequests();
};

void TestImageTileDecodeState::surfaceChangeOwnsGenerationAndClearsRequestState()
{
    KiriView::ImageTileDecodeState state;
    const auto firstSurface = std::make_shared<KiriView::DisplayedImageSurface>();
    const auto secondSurface = std::make_shared<KiriView::DisplayedImageSurface>();
    const KiriView::TileKey key { 0, 0, 0 };

    const KiriView::ImageTileDecodeScheduleState firstSchedule = state.beginSchedule(firstSurface);
    const quint64 firstGeneration = firstSchedule.generation;
    QVERIFY(firstSchedule.exclusions.pendingTileKeys.isEmpty());
    QVERIFY(firstSchedule.exclusions.failedTileKeys.isEmpty());
    QCOMPARE(
        state.commitScheduleRequests(firstSchedule, { tileRequest(key) }).size(), std::size_t(1));
    QVERIFY(state.finish(firstGeneration, key));
    state.fail(key);
    QVERIFY(state.beginSchedule(firstSurface).exclusions.contains(key));

    const quint64 repeatedGeneration = state.beginSchedule(firstSurface).generation;
    QCOMPARE(repeatedGeneration, firstGeneration);
    QVERIFY(state.beginSchedule(firstSurface).exclusions.contains(key));

    const quint64 secondGeneration = state.beginSchedule(secondSurface).generation;
    QVERIFY(secondGeneration != firstGeneration);
    QVERIFY(!state.beginSchedule(secondSurface).exclusions.contains(key));
    QVERIFY(!state.finish(firstGeneration, key));
}

void TestImageTileDecodeState::requestStartCommitsOnlyAcceptedScheduleWork()
{
    KiriView::ImageTileDecodeState state;
    const auto surface = std::make_shared<KiriView::DisplayedImageSurface>();
    const KiriView::ImageTileDecodeScheduleState schedule = state.beginSchedule(surface);
    const KiriView::TileKey firstKey { 0, 0, 0 };
    const KiriView::TileKey secondKey { 0, 0, 1 };
    const KiriView::TileKey failedKey { 0, 1, 0 };

    state.fail(failedKey);
    std::vector<KiriView::TileRequest> requests {
        tileRequest(firstKey),
        tileRequest(firstKey),
        tileRequest(failedKey),
        tileRequest(secondKey),
    };

    std::vector<KiriView::TileRequest> accepted
        = state.commitScheduleRequests(schedule, std::move(requests));

    QCOMPARE(accepted.size(), std::size_t(2));
    QCOMPARE(accepted.at(0).key, firstKey);
    QCOMPARE(accepted.at(1).key, secondKey);
    QVERIFY(state.beginSchedule(surface).exclusions.pendingTileKeys.contains(firstKey));
    QVERIFY(state.beginSchedule(surface).exclusions.pendingTileKeys.contains(secondKey));
    QVERIFY(state.beginSchedule(surface).exclusions.failedTileKeys.contains(failedKey));
    KiriView::ImageTileDecodeScheduleState staleSchedule = schedule;
    staleSchedule.generation += 1;
    QVERIFY(
        state.commitScheduleRequests(staleSchedule, { tileRequest(KiriView::TileKey { 0, 2, 0 }) })
            .empty());
}

void TestImageTileDecodeState::invalidateRejectsPendingRequests()
{
    KiriView::ImageTileDecodeState state;
    const auto surface = std::make_shared<KiriView::DisplayedImageSurface>();
    const KiriView::TileKey key { 0, 0, 0 };
    const quint64 generation = state.beginSchedule(surface).generation;

    QCOMPARE(
        state.commitScheduleRequests(state.beginSchedule(surface), { tileRequest(key) }).size(),
        std::size_t(1));
    state.invalidate();

    QVERIFY(!state.finish(generation, key));
    QVERIFY(!state.beginSchedule(surface).exclusions.contains(key));
}

QTEST_GUILESS_MAIN(TestImageTileDecodeState)

#include "test_imagetiledecodestate.moc"
