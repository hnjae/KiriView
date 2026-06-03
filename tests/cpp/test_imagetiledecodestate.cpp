// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/imagetiledecodestate.h"

#include <QObject>
#include <QString>
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

KiriView::RenderSurfaceKey testRenderSurfaceKey(
    const QString &surfaceIdentity = QStringLiteral("surface-1"), quint64 surfaceGeneration = 1)
{
    return KiriView::renderSurfaceKey(
        surfaceIdentity, surfaceGeneration, 1, QStringLiteral("primary"), QStringLiteral("raster"));
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
    const KiriView::RenderSurfaceKey firstSurfaceKey = testRenderSurfaceKey();
    const KiriView::RenderSurfaceKey secondSurfaceKey
        = testRenderSurfaceKey(QStringLiteral("surface-2"), 1);

    const KiriView::ImageTileDecodeScheduleState firstSchedule
        = state.beginSchedule(firstSurface, firstSurfaceKey);
    const quint64 firstGeneration = firstSchedule.generation;
    QVERIFY(firstSchedule.exclusions.pendingTileKeys.isEmpty());
    QVERIFY(firstSchedule.exclusions.failedTileKeys.isEmpty());
    QCOMPARE(
        state.commitScheduleRequests(firstSchedule, { tileRequest(key) }).size(), std::size_t(1));
    QVERIFY(state.finish(firstGeneration, firstSchedule.surfaceKey, key));
    state.fail(firstSchedule.surfaceKey, key);
    QVERIFY(state.beginSchedule(firstSurface, firstSurfaceKey).exclusions.contains(key));

    const quint64 repeatedGeneration
        = state.beginSchedule(firstSurface, firstSurfaceKey).generation;
    QCOMPARE(repeatedGeneration, firstGeneration);
    QVERIFY(state.beginSchedule(firstSurface, firstSurfaceKey).exclusions.contains(key));

    const quint64 secondGeneration
        = state.beginSchedule(secondSurface, secondSurfaceKey).generation;
    QVERIFY(secondGeneration != firstGeneration);
    QVERIFY(!state.beginSchedule(secondSurface, secondSurfaceKey).exclusions.contains(key));
    QVERIFY(!state.finish(firstGeneration, firstSchedule.surfaceKey, key));
}

void TestImageTileDecodeState::requestStartCommitsOnlyAcceptedScheduleWork()
{
    KiriView::ImageTileDecodeState state;
    const auto surface = std::make_shared<KiriView::DisplayedImageSurface>();
    const KiriView::ImageTileDecodeScheduleState schedule
        = state.beginSchedule(surface, testRenderSurfaceKey());
    const KiriView::TileKey firstKey { 0, 0, 0 };
    const KiriView::TileKey secondKey { 0, 0, 1 };
    const KiriView::TileKey failedKey { 0, 1, 0 };

    state.fail(schedule.surfaceKey, failedKey);
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
    QVERIFY(state.beginSchedule(surface, schedule.surfaceKey)
            .exclusions.pendingTileKeys.contains(firstKey));
    QVERIFY(state.beginSchedule(surface, schedule.surfaceKey)
            .exclusions.pendingTileKeys.contains(secondKey));
    QVERIFY(state.beginSchedule(surface, schedule.surfaceKey)
            .exclusions.failedTileKeys.contains(failedKey));
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
    const KiriView::ImageTileDecodeScheduleState schedule
        = state.beginSchedule(surface, testRenderSurfaceKey());

    QCOMPARE(state
                 .commitScheduleRequests(
                     state.beginSchedule(surface, schedule.surfaceKey), { tileRequest(key) })
                 .size(),
        std::size_t(1));
    state.invalidate();

    QVERIFY(!state.finish(schedule.generation, schedule.surfaceKey, key));
    QVERIFY(!state.beginSchedule(surface, schedule.surfaceKey).exclusions.contains(key));
}

QTEST_GUILESS_MAIN(TestImageTileDecodeState)

#include "test_imagetiledecodestate.moc"
