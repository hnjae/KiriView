// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "rendering/imagetiledecoderuntime.h"

#include <QImage>
#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

namespace {
constexpr qsizetype testTileCacheByteBudget = KiriView::imageFullDecodeFallbackByteLimit;

using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

std::shared_ptr<KiriView::DisplayedImageSurface> testSurface(const QSize &imageSize)
{
    KiriView::StaticImagePayload payload
        = staticTestImagePayload(testImage(imageSize), testImage(QSize(512, 512)), {});
    return std::make_shared<KiriView::DisplayedImageSurface>(
        KiriView::StaticTileSurface { std::move(payload), testTileCacheByteBudget });
}

KiriView::ImageTileDecodeRuntimePlan scheduleVisibleTiles(KiriView::ImageTileDecodeRuntime &runtime,
    const std::shared_ptr<KiriView::DisplayedImageSurface> &surface)
{
    return runtime.schedule(surface, QSizeF(2048.0, 1024.0), QRectF(0.0, 0.0, 2048.0, 1024.0),
        KiriView::ImageDocumentRenderContext {}, 0);
}

bool containsRequestForKey(
    const KiriView::ImageTileDecodeRuntimePlan &plan, const KiriView::TileKey &key)
{
    return std::any_of(plan.requests.cbegin(), plan.requests.cend(),
        [&key](const KiriView::TileRequest &request) { return request.key == key; });
}
}

class TestImageTileDecodeRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void schedulePlanCommitsAcceptedRequestsToRuntimeState();
    void completionAcceptanceRequiresCurrentGenerationAndPendingKey();
    void failedTileKeysAreScopedToCurrentSurface();
    void invalidateRejectsPendingRequestsAndAllowsReschedule();
};

void TestImageTileDecodeRuntime::schedulePlanCommitsAcceptedRequestsToRuntimeState()
{
    KiriView::ImageTileDecodeRuntime runtime;
    const auto surface = testSurface(QSize(2048, 1024));

    const KiriView::ImageTileDecodeRuntimePlan initialPlan = scheduleVisibleTiles(runtime, surface);
    QVERIFY(!initialPlan.isEmpty());
    QVERIFY(initialPlan.generation > 0);
    QVERIFY(initialPlan.source != nullptr);

    const KiriView::ImageTileDecodeRuntimePlan duplicatePlan
        = scheduleVisibleTiles(runtime, surface);
    QVERIFY(duplicatePlan.isEmpty());
}

void TestImageTileDecodeRuntime::completionAcceptanceRequiresCurrentGenerationAndPendingKey()
{
    KiriView::ImageTileDecodeRuntime runtime;
    const auto surface = testSurface(QSize(2048, 1024));
    const KiriView::ImageTileDecodeRuntimePlan plan = scheduleVisibleTiles(runtime, surface);
    QVERIFY(!plan.isEmpty());

    const KiriView::TileKey key = plan.requests.front().key;
    QVERIFY(!runtime.acceptFinishedTileDecode(plan.generation + 1, key, true));
    QVERIFY(runtime.acceptFinishedTileDecode(plan.generation, key, true));
    QVERIFY(!runtime.acceptFinishedTileDecode(plan.generation, key, true));
}

void TestImageTileDecodeRuntime::failedTileKeysAreScopedToCurrentSurface()
{
    KiriView::ImageTileDecodeRuntime runtime;
    const auto firstSurface = testSurface(QSize(2048, 1024));
    const auto secondSurface = testSurface(QSize(2048, 1024));

    const KiriView::ImageTileDecodeRuntimePlan firstPlan
        = scheduleVisibleTiles(runtime, firstSurface);
    QVERIFY(firstPlan.requests.size() >= std::size_t(2));
    const KiriView::TileKey failedKey = firstPlan.requests.front().key;
    const KiriView::TileKey pendingKey = firstPlan.requests.at(1).key;
    QVERIFY(runtime.acceptFinishedTileDecode(firstPlan.generation, failedKey, false));
    QVERIFY(runtime.acceptFinishedTileDecode(firstPlan.generation, pendingKey, true));

    const KiriView::ImageTileDecodeRuntimePlan sameSurfacePlan
        = scheduleVisibleTiles(runtime, firstSurface);
    QVERIFY(!containsRequestForKey(sameSurfacePlan, failedKey));

    const KiriView::ImageTileDecodeRuntimePlan nextSurfacePlan
        = scheduleVisibleTiles(runtime, secondSurface);
    QVERIFY(containsRequestForKey(nextSurfacePlan, failedKey));
}

void TestImageTileDecodeRuntime::invalidateRejectsPendingRequestsAndAllowsReschedule()
{
    KiriView::ImageTileDecodeRuntime runtime;
    const auto surface = testSurface(QSize(2048, 1024));
    const KiriView::ImageTileDecodeRuntimePlan plan = scheduleVisibleTiles(runtime, surface);
    QVERIFY(!plan.isEmpty());

    const KiriView::TileKey key = plan.requests.front().key;
    runtime.invalidate();

    QVERIFY(!runtime.acceptFinishedTileDecode(plan.generation, key, true));

    const KiriView::ImageTileDecodeRuntimePlan refreshedPlan
        = scheduleVisibleTiles(runtime, surface);
    QVERIFY(containsRequestForKey(refreshedPlan, key));
}

QTEST_GUILESS_MAIN(TestImageTileDecodeRuntime)

#include "test_imagetiledecoderuntime.moc"
