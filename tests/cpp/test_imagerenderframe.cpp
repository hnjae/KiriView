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

KiriView::StaticTileSurface svgTileSurface(const QSize &imageSize)
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(svgData(imageSize), &errorString);
    Q_ASSERT(source != nullptr);

    return KiriView::StaticTileSurface {
        KiriView::StaticImagePayload {
            std::move(source),
            testImage(imageSize),
            {},
        },
        testTileCacheByteBudget,
    };
}

KiriView::ImageRenderFrame projectFrame(const KiriView::DisplayedImageSurface &surface,
    const QSizeF &displaySize, const QRectF &visibleItemRect, qreal devicePixelRatio = 1.0)
{
    return KiriView::projectImageRenderFrame(KiriView::ImageRenderFrameInput {
        &surface,
        1,
        KiriView::ImageSurfaceDrawContext {
            QRectF(0.0, 0.0, displaySize.width(), displaySize.height()),
            displaySize,
            visibleItemRect,
            devicePixelRatio,
            0,
        },
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
    void visibleRectChangesCurrentDrawEntries();
    void svgHighZoomFrameDoesNotDrawCachedLowBucketTile();
    void devicePixelRatioChangeRecomputesSvgBucket();
};

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
