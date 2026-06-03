// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "rendering/imagerendering.h"
#include "rendering/imagetiledecoderuntime.h"
#include "rendering/svgtilesource.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QTest>
#include <Qt>
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

QByteArray svgData(const QSize &imageSize)
{
    return QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%1\" height=\"%2\">"
                          "<rect width=\"%1\" height=\"%2\" fill=\"red\"/>"
                          "</svg>")
        .arg(imageSize.width())
        .arg(imageSize.height())
        .toUtf8();
}

std::shared_ptr<KiriView::DisplayedImageSurface> svgSurface(const QSize &imageSize)
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(svgData(imageSize), &errorString);
    Q_ASSERT(source != nullptr);

    KiriView::StaticImagePayload payload {
        std::move(source),
        testImage(imageSize),
        {},
    };
    return std::make_shared<KiriView::DisplayedImageSurface>(
        KiriView::StaticTileSurface { std::move(payload), testTileCacheByteBudget });
}

KiriView::DecodedTile decodedTileForRequest(const KiriView::TileRequest &request)
{
    QImage image(request.textureLevelRect.size(), QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return KiriView::DecodedTile {
        request.key,
        request.levelSize,
        request.levelRect,
        request.textureLevelRect,
        std::move(image),
        request.displaySourceRect,
        request.displaySourceRectF,
    };
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

bool containsTileEntry(
    const std::vector<KiriView::ImageSurfaceDrawEntry> &entries, const KiriView::TileKey &key)
{
    return std::any_of(
        entries.cbegin(), entries.cend(), [&key](const KiriView::ImageSurfaceDrawEntry &entry) {
            return entry.identity.kind == KiriView::ImageSurfaceDrawIdentityKind::Tile
                && entry.identity.tileKey == key;
        });
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
    void staleSvgLowBucketCompletionRemainsCachedButNotDrawnAtHighZoom();
};

void TestImageTileDecodeRuntime::schedulePlanCommitsAcceptedRequestsToRuntimeState()
{
    KiriView::ImageTileDecodeRuntime runtime;
    const auto surface = testSurface(QSize(2048, 1024));

    const KiriView::ImageTileDecodeRuntimePlan initialPlan = scheduleVisibleTiles(runtime, surface);
    QVERIFY(!initialPlan.isEmpty());
    QVERIFY(initialPlan.generation > 0);
    QVERIFY(!initialPlan.surfaceKey.surfaceIdentity.isEmpty());
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
    KiriView::RenderSurfaceKey wrongSurfaceKey = plan.surfaceKey;
    wrongSurfaceKey.pageRole = QStringLiteral("secondary");
    QVERIFY(!runtime.acceptFinishedTileDecode(plan.generation + 1, plan.surfaceKey, key, true));
    QVERIFY(!runtime.acceptFinishedTileDecode(plan.generation, wrongSurfaceKey, key, true));
    QVERIFY(runtime.acceptFinishedTileDecode(plan.generation, plan.surfaceKey, key, true));
    QVERIFY(!runtime.acceptFinishedTileDecode(plan.generation, plan.surfaceKey, key, true));
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
    QVERIFY(runtime.acceptFinishedTileDecode(
        firstPlan.generation, firstPlan.surfaceKey, failedKey, false));
    QVERIFY(runtime.acceptFinishedTileDecode(
        firstPlan.generation, firstPlan.surfaceKey, pendingKey, true));

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

    QVERIFY(!runtime.acceptFinishedTileDecode(plan.generation, plan.surfaceKey, key, true));

    const KiriView::ImageTileDecodeRuntimePlan refreshedPlan
        = scheduleVisibleTiles(runtime, surface);
    QVERIFY(containsRequestForKey(refreshedPlan, key));
}

void TestImageTileDecodeRuntime::staleSvgLowBucketCompletionRemainsCachedButNotDrawnAtHighZoom()
{
    KiriView::ImageTileDecodeRuntime runtime;
    const auto surface = svgSurface(QSize(1000, 1000));
    const KiriView::ImageDocumentRenderContext context { 1.0, 4096 };

    const KiriView::ImageTileDecodeRuntimePlan lowPlan = runtime.schedule(
        surface, QSizeF(100.0, 100.0), QRectF(0.0, 0.0, 100.0, 100.0), context, 0);
    QVERIFY(!lowPlan.isEmpty());
    const KiriView::TileRequest lowRequest = lowPlan.requests.front();

    const KiriView::ImageTileDecodeRuntimePlan highPlan = runtime.schedule(
        surface, QSizeF(1600.0, 1600.0), QRectF(0.0, 0.0, 1000.0, 1000.0), context, 0);
    QVERIFY(!highPlan.isEmpty());
    const KiriView::TileRequest highRequest = highPlan.requests.front();
    QVERIFY(lowRequest.key.scaleBucket < highRequest.key.scaleBucket);

    QVERIFY(runtime.acceptFinishedTileDecode(
        lowPlan.generation, lowPlan.surfaceKey, lowRequest.key, true));
    QVERIFY(runtime.acceptFinishedTileDecode(
        highPlan.generation, highPlan.surfaceKey, highRequest.key, true));

    KiriView::StaticTileSurface *staticSurface = surface->staticTileSurface();
    QVERIFY(staticSurface != nullptr);
    QVERIFY(staticSurface->insertTile(decodedTileForRequest(lowRequest)));
    QVERIFY(staticSurface->insertTile(decodedTileForRequest(highRequest)));

    const std::vector<KiriView::ImageSurfaceDrawEntry> entries
        = KiriView::imageSurfaceDrawEntries(*surface,
            KiriView::ImageSurfaceDrawContext {
                QRectF(0.0, 0.0, 1000.0, 1000.0),
                QSizeF(1600.0, 1600.0),
                QRectF(0.0, 0.0, 1000.0, 1000.0),
                1.0,
                0,
            });

    QCOMPARE(entries.size(), std::size_t(2));
    QVERIFY(containsTileEntry(entries, highRequest.key));
    QVERIFY(!containsTileEntry(entries, lowRequest.key));
    QVERIFY(staticSurface->containsTile(lowRequest.key));
}

QTEST_GUILESS_MAIN(TestImageTileDecodeRuntime)

#include "test_imagetiledecoderuntime.moc"
