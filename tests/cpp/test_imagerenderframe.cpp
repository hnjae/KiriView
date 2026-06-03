// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/imagerenderframe.h"

#include "image_test_support.h"
#include "rendering/imagetilegeometrypolicy.h"
#include "rendering/svgtilesource.h"

#include <QByteArray>
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

KiriView::StaticTileSurface rasterTileSurface(const QSize &imageSize)
{
    return KiriView::StaticTileSurface {
        staticTestImagePayload(testImage(imageSize), testImage(QSize(512, 512)), {}),
        testTileCacheByteBudget,
    };
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

KiriView::StaticTileSurface svgTileSurface(
    const QSize &imageSize, KiriView::StaticImageDisplayHints displayHints = {})
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(svgData(imageSize), &errorString);
    Q_ASSERT(source != nullptr);

    return KiriView::StaticTileSurface {
        KiriView::StaticImagePayload {
            std::move(source),
            testImage(imageSize),
            displayHints,
        },
        testTileCacheByteBudget,
    };
}

KiriView::ImageRenderFrame projectFrame(const KiriView::DisplayedImageSurface &surface,
    const QSizeF &displaySize, const QRectF &visibleItemRect, qreal devicePixelRatio = 1.0,
    KiriView::ImageTileDecodeExclusions tileDecodeExclusions = {})
{
    return KiriView::projectImageRenderFrame(KiriView::ImageRenderFrameInput {
        &surface,
        1,
        0,
        KiriView::ImageSurfaceDrawContext {
            QRectF(0.0, 0.0, displaySize.width(), displaySize.height()),
            displaySize,
            visibleItemRect,
            devicePixelRatio,
            0,
        },
        KiriView::DisplayedPageRole::Primary,
        std::move(tileDecodeExclusions),
    });
}

bool containsTileEntry(const KiriView::ImageRenderFrame &frame, const KiriView::TileKey &key)
{
    return std::any_of(frame.drawEntries.cbegin(), frame.drawEntries.cend(),
        [&key](const KiriView::ImageSurfaceDrawEntry &entry) {
            return entry.identity.kind == KiriView::ImageSurfaceDrawIdentityKind::Tile
                && entry.identity.tileKey == key;
        });
}

bool containsRequestForKey(const KiriView::ImageRenderFrame &frame, const KiriView::TileKey &key)
{
    return std::any_of(frame.missingTileRequests.cbegin(), frame.missingTileRequests.cend(),
        [&key](const KiriView::TileRequest &request) { return request.key == key; });
}

bool allMissingRequestsUseBucket(const KiriView::ImageRenderFrame &frame, int scaleBucket)
{
    return std::all_of(frame.missingTileRequests.cbegin(), frame.missingTileRequests.cend(),
        [scaleBucket](const KiriView::TileRequest &request) {
            return request.key.scaleBucket == scaleBucket;
        });
}
}

class TestImageRenderFrame : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void projectedFrameCarriesSurfaceIdentityRevisionAndPageRole();
    void visibleStaticTilesBecomeMissingDecodeRequests();
    void firstDisplayHintSuppressesUnneededRequests();
    void existingPendingAndFailedTilesAreFiltered();
    void visibleRectChangesCurrentDrawEntries();
    void svgScaleBucketRequestsChangeOnlyAcrossBucketBoundary();
    void svgFirstDisplayHintDoesNotSuppressCurrentDetailRequests();
    void svgHighZoomFrameDoesNotDrawCachedLowBucketTile();
    void devicePixelRatioChangeRecomputesSvgBucket();
};

void TestImageRenderFrame::projectedFrameCarriesSurfaceIdentityRevisionAndPageRole()
{
    const KiriView::DisplayedImageSurface surface(rasterTileSurface(QSize(512, 512)));

    const KiriView::ImageRenderFrame frame
        = KiriView::projectImageRenderFrame(KiriView::ImageRenderFrameInput {
            &surface,
            42,
            7,
            KiriView::ImageSurfaceDrawContext {
                QRectF(0.0, 0.0, 512.0, 512.0),
                QSizeF(512.0, 512.0),
                QRectF(0.0, 0.0, 512.0, 512.0),
                1.0,
                0,
            },
            KiriView::DisplayedPageRole::Secondary,
            {},
        });

    QVERIFY(frame.isRenderable());
    QCOMPARE(frame.surfaceIdentity, surface.identity());
    QCOMPARE(frame.surfaceRevision, quint64(42));
    QCOMPARE(frame.renderContextGeneration, quint64(7));
    QCOMPARE(frame.pageRole, KiriView::DisplayedPageRole::Secondary);
}

void TestImageRenderFrame::visibleStaticTilesBecomeMissingDecodeRequests()
{
    const KiriView::DisplayedImageSurface surface(rasterTileSurface(QSize(2048, 1024)));

    const KiriView::ImageRenderFrame frame
        = projectFrame(surface, QSizeF(2048.0, 1024.0), QRectF(0.0, 0.0, 1024.0, 1024.0));

    QVERIFY(frame.tileRequestSource != nullptr);
    QVERIFY(!frame.missingTileRequests.empty());
    for (const KiriView::TileRequest &request : frame.missingTileRequests) {
        QVERIFY(!request.textureLevelRect.isEmpty());
        QVERIFY(!request.sourceRect.isEmpty());
    }
}

void TestImageRenderFrame::firstDisplayHintSuppressesUnneededRequests()
{
    const KiriView::DisplayedImageSurface surface(KiriView::StaticTileSurface {
        staticTestImagePayload(testImage(QSize(2048, 2048)), testImage(QSize(512, 512)),
            KiriView::StaticImageDisplayHints { 0.25 }),
        testTileCacheByteBudget,
    });

    const KiriView::ImageRenderFrame suppressedFrame
        = projectFrame(surface, QSizeF(512.0, 512.0), QRectF(0.0, 0.0, 512.0, 512.0));
    QVERIFY(suppressedFrame.missingTileRequests.empty());
    QVERIFY(suppressedFrame.tileRequestSource == nullptr);

    const KiriView::ImageRenderFrame detailedFrame
        = projectFrame(surface, QSizeF(2048.0, 2048.0), QRectF(0.0, 0.0, 512.0, 512.0));
    QVERIFY(!detailedFrame.missingTileRequests.empty());
    QVERIFY(detailedFrame.tileRequestSource != nullptr);
}

void TestImageRenderFrame::existingPendingAndFailedTilesAreFiltered()
{
    KiriView::StaticTileSurface tileSurface = rasterTileSurface(QSize(2048, 1024));
    const KiriView::ImageRenderFrame initialFrame
        = projectFrame(KiriView::DisplayedImageSurface(tileSurface), QSizeF(2048.0, 1024.0),
            QRectF(0.0, 0.0, 2048.0, 1024.0));
    QVERIFY(initialFrame.missingTileRequests.size() >= std::size_t(3));

    const KiriView::TileKey existingKey = initialFrame.missingTileRequests.at(0).key;
    const KiriView::TileKey pendingKey = initialFrame.missingTileRequests.at(1).key;
    const KiriView::TileKey failedKey = initialFrame.missingTileRequests.at(2).key;
    QVERIFY(tileSurface.insertTile(decodedTileForRequest(initialFrame.missingTileRequests.at(0))));

    KiriView::ImageTileDecodeExclusions exclusions;
    exclusions.pendingTileKeys.insert(pendingKey);
    exclusions.failedTileKeys.insert(failedKey);

    const KiriView::DisplayedImageSurface surface(std::move(tileSurface));
    const KiriView::ImageRenderFrame filteredFrame = projectFrame(surface, QSizeF(2048.0, 1024.0),
        QRectF(0.0, 0.0, 2048.0, 1024.0), 1.0, std::move(exclusions));

    QVERIFY(!containsRequestForKey(filteredFrame, existingKey));
    QVERIFY(!containsRequestForKey(filteredFrame, pendingKey));
    QVERIFY(!containsRequestForKey(filteredFrame, failedKey));
    QCOMPARE(filteredFrame.missingTileRequests.size(), initialFrame.missingTileRequests.size() - 3);
}

void TestImageRenderFrame::visibleRectChangesCurrentDrawEntries()
{
    KiriView::StaticTileSurface surface = rasterTileSurface(QSize(4096, 512));
    const KiriView::TileKey leftKey { 0, 0, 0 };
    const KiriView::TileKey rightKey { 0, 7, 0 };
    QVERIFY(surface.insertTile(decodedTileForRequest(surface.pyramid().requestForTile(leftKey))));
    QVERIFY(surface.insertTile(decodedTileForRequest(surface.pyramid().requestForTile(rightKey))));

    const KiriView::DisplayedImageSurface displayedSurface(std::move(surface));
    const KiriView::ImageRenderFrame leftFrame
        = projectFrame(displayedSurface, QSizeF(4096.0, 512.0), QRectF(0.0, 0.0, 512.0, 512.0));
    const KiriView::ImageRenderFrame rightFrame
        = projectFrame(displayedSurface, QSizeF(4096.0, 512.0), QRectF(3584.0, 0.0, 512.0, 512.0));

    QVERIFY(containsTileEntry(leftFrame, leftKey));
    QVERIFY(!containsTileEntry(leftFrame, rightKey));
    QVERIFY(!containsTileEntry(rightFrame, leftKey));
    QVERIFY(containsTileEntry(rightFrame, rightKey));
}

void TestImageRenderFrame::svgScaleBucketRequestsChangeOnlyAcrossBucketBoundary()
{
    KiriView::StaticTileSurface surface = svgTileSurface(QSize(1000, 1000));
    KiriView::DisplayedImageSurface displayedSurface(std::move(surface));

    const KiriView::ImageRenderFrame initialFrame
        = projectFrame(displayedSurface, QSizeF(1000.0, 1000.0), QRectF(0.0, 0.0, 1000.0, 1000.0));
    QVERIFY(!initialFrame.missingTileRequests.empty());
    for (const KiriView::TileRequest &request : initialFrame.missingTileRequests) {
        QCOMPARE(request.key.level, 0);
        QCOMPARE(request.key.scaleBucket, 1);
        QCOMPARE(request.levelSize, QSize(1500, 1500));
        QVERIFY(!request.displaySourceRect.isEmpty());
    }

    auto *staticSurface = displayedSurface.staticTileSurface();
    QVERIFY(staticSurface != nullptr);
    for (const KiriView::TileRequest &request : initialFrame.missingTileRequests) {
        QVERIFY(staticSurface->insertTile(decodedTileForRequest(request)));
    }

    const KiriView::ImageRenderFrame sameBucketFrame
        = projectFrame(displayedSurface, QSizeF(1200.0, 1200.0), QRectF(0.0, 0.0, 1200.0, 1200.0));
    QVERIFY(sameBucketFrame.missingTileRequests.empty());

    const KiriView::ImageRenderFrame nextBucketFrame
        = projectFrame(displayedSurface, QSizeF(1600.0, 1600.0), QRectF(0.0, 0.0, 1600.0, 1600.0));
    QVERIFY(!nextBucketFrame.missingTileRequests.empty());
    for (const KiriView::TileRequest &request : nextBucketFrame.missingTileRequests) {
        QCOMPARE(request.key.level, 0);
        QCOMPARE(request.key.scaleBucket, 2);
        QCOMPARE(request.levelSize, QSize(2250, 2250));
    }
}

void TestImageRenderFrame::svgFirstDisplayHintDoesNotSuppressCurrentDetailRequests()
{
    KiriView::StaticTileSurface surface
        = svgTileSurface(QSize(1000, 1000), KiriView::StaticImageDisplayHints { 2.0 });
    KiriView::DisplayedImageSurface displayedSurface(std::move(surface));

    const KiriView::ImageRenderFrame frame
        = projectFrame(displayedSurface, QSizeF(1000.0, 1000.0), QRectF(0.0, 0.0, 1000.0, 1000.0));

    QVERIFY(frame.tileRequestSource != nullptr);
    QVERIFY(!frame.missingTileRequests.empty());
    QVERIFY(allMissingRequestsUseBucket(frame, 1));
}

void TestImageRenderFrame::svgHighZoomFrameDoesNotDrawCachedLowBucketTile()
{
    KiriView::StaticTileSurface surface = svgTileSurface(QSize(1000, 1000));
    const KiriView::TileRequest lowBucketRequest
        = KiriView::ImageTileGeometryPolicy::svgRasterTileRequests(
            QSize(1000, 1000), 512, 1, QSizeF(100.0, 100.0), QRectF(0.0, 0.0, 100.0, 100.0), 1.0)
              .front();
    const KiriView::TileRequest highBucketRequest
        = KiriView::ImageTileGeometryPolicy::svgRasterTileRequests(QSize(1000, 1000), 512, 1,
            QSizeF(1600.0, 1600.0), QRectF(0.0, 0.0, 1000.0, 1000.0), 1.0)
              .front();
    QVERIFY(lowBucketRequest.key.scaleBucket < highBucketRequest.key.scaleBucket);
    QVERIFY(surface.insertTile(decodedTileForRequest(lowBucketRequest)));
    QVERIFY(surface.insertTile(decodedTileForRequest(highBucketRequest)));

    const KiriView::DisplayedImageSurface displayedSurface(std::move(surface));
    const KiriView::ImageRenderFrame frame
        = projectFrame(displayedSurface, QSizeF(1600.0, 1600.0), QRectF(0.0, 0.0, 1000.0, 1000.0));

    QCOMPARE(frame.activeTileLayer.scaleBucket, highBucketRequest.key.scaleBucket);
    QVERIFY(!containsTileEntry(frame, lowBucketRequest.key));
    QVERIFY(containsTileEntry(frame, highBucketRequest.key));
}

void TestImageRenderFrame::devicePixelRatioChangeRecomputesSvgBucket()
{
    const KiriView::DisplayedImageSurface displayedSurface(svgTileSurface(QSize(1000, 1000)));
    const KiriView::ImageRenderFrame ordinaryDprFrame = projectFrame(
        displayedSurface, QSizeF(1000.0, 1000.0), QRectF(0.0, 0.0, 1000.0, 1000.0), 1.0);
    const KiriView::ImageRenderFrame highDprFrame = projectFrame(
        displayedSurface, QSizeF(1000.0, 1000.0), QRectF(0.0, 0.0, 1000.0, 1000.0), 2.0);

    QVERIFY(
        ordinaryDprFrame.activeTileLayer.scaleBucket < highDprFrame.activeTileLayer.scaleBucket);
    QVERIFY(allMissingRequestsUseBucket(
        ordinaryDprFrame, ordinaryDprFrame.activeTileLayer.scaleBucket));
    QVERIFY(allMissingRequestsUseBucket(highDprFrame, highDprFrame.activeTileLayer.scaleBucket));
}

QTEST_GUILESS_MAIN(TestImageRenderFrame)

#include "test_imagerenderframe.moc"
