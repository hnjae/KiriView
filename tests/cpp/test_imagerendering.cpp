// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/imagerendering.h"

#include "image_test_support.h"
#include "rendering/imagetilegeometrypolicy.h"
#include "rendering/svgtilesource.h"

#include <QByteArray>
#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QTest>
#include <Qt>
#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
constexpr qsizetype testTileCacheByteBudget = KiriView::imageFullDecodeFallbackByteLimit;

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
            KiriView::TestSupport::testImage(imageSize),
            {},
        },
        testTileCacheByteBudget,
    };
}
}

class TestImageRendering : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scaledImageSizeToFitKeepsAspectRatioWithoutUpscaling();
    void scaledImageSizeToFitRejectsInvalidInput();
    void firstDisplayScaledImageSizeOnlyReturnsDownscaleTarget();
    void imagePixelsPerSourcePixelUsesLimitingAxis();
    void renderContextNormalizationUsesSafeDefaults();
    void firstDisplayDecodeContextUsesPhysicalViewport();
    void staticSurfaceDrawEntriesKeepPreviewAndTileRectsSeparate();
    void staticSurfaceDrawEntriesOnlyDrawActiveRasterLevel();
    void svgSurfaceDrawEntriesOnlyDrawActiveScaleBucket();
    void svgZoomOutThenInSkipsStaleLowBucketTiles();
    void svgTileDrawEntriesUseFractionalDisplayCoverage();
    void rotatedFullImageDrawEntriesRotateTextureCoordinates();
    void rotatedStaticTileDrawEntriesMapSourceRects();
};

void TestImageRendering::scaledImageSizeToFitKeepsAspectRatioWithoutUpscaling()
{
    QCOMPARE(KiriView::scaledImageSizeToFit(QSizeF(4000.0, 2000.0), QSize(1000, 1000)),
        QSize(1000, 500));
    QCOMPARE(KiriView::scaledImageSizeToFit(QSizeF(333.0, 100.0), QSize(200, 200)), QSize(200, 61));
    QCOMPARE(
        KiriView::scaledImageSizeToFit(QSizeF(200.0, 100.0), QSize(1000, 1000)), QSize(200, 100));
    QCOMPARE(KiriView::scaledImageSizeToFit(QSizeF(0.5, 0.5), QSize(100, 100)), QSize(1, 1));
}

void TestImageRendering::scaledImageSizeToFitRejectsInvalidInput()
{
    const qreal nan = std::numeric_limits<qreal>::quiet_NaN();
    QCOMPARE(KiriView::scaledImageSizeToFit(QSizeF(), QSize(100, 100)), QSize());
    QCOMPARE(KiriView::scaledImageSizeToFit(QSizeF(100.0, 100.0), QSize()), QSize());
    QCOMPARE(KiriView::scaledImageSizeToFit(QSizeF(nan, 100.0), QSize(100, 100)), QSize());
}

void TestImageRendering::firstDisplayScaledImageSizeOnlyReturnsDownscaleTarget()
{
    QCOMPARE(
        KiriView::firstDisplayScaledImageSize(QSize(1600, 1200), QSize(400, 300)), QSize(400, 300));
    QCOMPARE(KiriView::firstDisplayScaledImageSize(QSize(200, 100), QSize(400, 300)), QSize());
    QCOMPARE(KiriView::firstDisplayScaledImageSize(QSize(1600, 1200), QSize()), QSize());
}

void TestImageRendering::imagePixelsPerSourcePixelUsesLimitingAxis()
{
    QCOMPARE(KiriView::imagePixelsPerSourcePixel(QSize(1600, 1200), QSize(400, 300)), 0.25);
    QCOMPARE(KiriView::imagePixelsPerSourcePixel(QSize(1600, 1200), QSize(800, 300)), 0.25);
    QCOMPARE(KiriView::imagePixelsPerSourcePixel(QSize(1600, 1200), QSize()), 0.0);
}

void TestImageRendering::renderContextNormalizationUsesSafeDefaults()
{
    const qreal nan = std::numeric_limits<qreal>::quiet_NaN();

    const KiriView::ImageDocumentRenderContext valid
        = KiriView::normalizedImageDocumentRenderContext({ 2.0, 4096 });
    QCOMPARE(valid.devicePixelRatio, 2.0);
    QCOMPARE(valid.maximumTextureSize, 4096);

    const KiriView::ImageDocumentRenderContext invalid
        = KiriView::normalizedImageDocumentRenderContext({ nan, 0 });
    QCOMPARE(invalid.devicePixelRatio, 1.0);
    QCOMPARE(invalid.maximumTextureSize, KiriView::fallbackTextureSizeMax);
}

void TestImageRendering::firstDisplayDecodeContextUsesPhysicalViewport()
{
    const KiriView::ImageFirstDisplayDecodeContext context
        = KiriView::imageFirstDisplayDecodeContext(QSizeF(400.25, 300.0), 2.0);
    QCOMPARE(context.physicalViewportSize, QSize(801, 600));
    QVERIFY(context.isValid());

    QVERIFY(!KiriView::imageFirstDisplayDecodeContext(QSizeF(), 2.0).isValid());
    QVERIFY(!KiriView::imageFirstDisplayDecodeContext(QSizeF(400.0, 300.0), 0.0).isValid());
}

void TestImageRendering::staticSurfaceDrawEntriesKeepPreviewAndTileRectsSeparate()
{
    const QImage sourceImage = KiriView::TestSupport::testImage(1024, 1024);
    KiriView::StaticTileSurface surface(KiriView::TestSupport::staticTestImagePayload(sourceImage,
                                            KiriView::TestSupport::testImage(256, 256)),
        testTileCacheByteBudget);
    QVERIFY(surface.insertTile(KiriView::DecodedTile {
        KiriView::TileKey { 0, 0, 0 },
        QSize(1024, 1024),
        QRect(0, 0, 512, 512),
        QRect(0, 0, 513, 513),
        KiriView::TestSupport::testImage(513, 513),
    }));

    const KiriView::DisplayedImageSurface displayedSurface(std::move(surface));
    const QRectF targetRect(10.0, 20.0, 1000.0, 500.0);
    const std::vector<KiriView::ImageSurfaceDrawEntry> entries
        = KiriView::imageSurfaceDrawEntries(displayedSurface,
            KiriView::ImageSurfaceDrawContext {
                targetRect,
                QSizeF(1024.0, 1024.0),
                targetRect,
                1.0,
                0,
            });

    QCOMPARE(entries.size(), std::size_t(2));
    QCOMPARE(entries[0].targetRect, targetRect);
    QCOMPARE(entries[0].textureRect, QRectF(0.0, 0.0, 1.0, 1.0));
    QCOMPARE(entries[1].targetRect, QRectF(10.0, 20.0, 500.0, 250.0));
    QVERIFY(entries[1].targetRect != entries[0].targetRect);
    QVERIFY(entries[1].textureRect != entries[0].textureRect);
    QCOMPARE(entries[1].textureRect.x(), 0.0);
    QCOMPARE(entries[1].textureRect.y(), 0.0);
    QVERIFY(qFuzzyCompare(entries[1].textureRect.width(), 512.0 / 513.0));
    QVERIFY(qFuzzyCompare(entries[1].textureRect.height(), 512.0 / 513.0));
}

void TestImageRendering::staticSurfaceDrawEntriesOnlyDrawActiveRasterLevel()
{
    KiriView::StaticTileSurface surface(
        KiriView::TestSupport::staticTestImagePayload(KiriView::TestSupport::testImage(2048, 2048),
            KiriView::TestSupport::testImage(512, 512)),
        testTileCacheByteBudget);
    const KiriView::TileKey inactiveKey { 0, 0, 0 };
    const KiriView::TileKey activeKey { 2, 0, 0 };
    QVERIFY(
        surface.insertTile(decodedTileForRequest(surface.pyramid().requestForTile(inactiveKey))));
    QVERIFY(surface.insertTile(decodedTileForRequest(surface.pyramid().requestForTile(activeKey))));

    const KiriView::DisplayedImageSurface displayedSurface(std::move(surface));
    const std::vector<KiriView::ImageSurfaceDrawEntry> entries
        = KiriView::imageSurfaceDrawEntries(displayedSurface,
            KiriView::ImageSurfaceDrawContext {
                QRectF(0.0, 0.0, 512.0, 512.0),
                QSizeF(512.0, 512.0),
                QRectF(0.0, 0.0, 512.0, 512.0),
                1.0,
                0,
            });

    QCOMPARE(entries.size(), std::size_t(2));
    QCOMPARE(entries[0].identity.kind, KiriView::ImageSurfaceDrawIdentityKind::Preview);
    QCOMPARE(entries[1].identity.kind, KiriView::ImageSurfaceDrawIdentityKind::Tile);
    QCOMPARE(entries[1].identity.tileKey, activeKey);
    QVERIFY(displayedSurface.staticTileSurface()->containsTile(inactiveKey));
    QVERIFY(displayedSurface.staticTileSurface()->containsTile(activeKey));
}

void TestImageRendering::svgSurfaceDrawEntriesOnlyDrawActiveScaleBucket()
{
    KiriView::StaticTileSurface surface = svgTileSurface(QSize(1000, 1000));
    const KiriView::TileRequest lowBucketRequest
        = KiriView::ImageTileGeometryPolicy::svgRasterTileRequests(QSize(1000, 1000), 512, 1,
            QSizeF(1000.0, 1000.0), QRectF(0.0, 0.0, 1000.0, 1000.0), 1.0)
              .front();
    const KiriView::TileRequest highBucketRequest
        = KiriView::ImageTileGeometryPolicy::svgRasterTileRequests(QSize(1000, 1000), 512, 1,
            QSizeF(1600.0, 1600.0), QRectF(0.0, 0.0, 1600.0, 1600.0), 1.0)
              .front();
    QCOMPARE(lowBucketRequest.key.scaleBucket, 1);
    QCOMPARE(highBucketRequest.key.scaleBucket, 2);
    QVERIFY(surface.insertTile(decodedTileForRequest(lowBucketRequest)));
    QVERIFY(surface.insertTile(decodedTileForRequest(highBucketRequest)));

    const KiriView::DisplayedImageSurface displayedSurface(std::move(surface));
    const std::vector<KiriView::ImageSurfaceDrawEntry> entries
        = KiriView::imageSurfaceDrawEntries(displayedSurface,
            KiriView::ImageSurfaceDrawContext {
                QRectF(0.0, 0.0, 1600.0, 1600.0),
                QSizeF(1600.0, 1600.0),
                QRectF(0.0, 0.0, 1600.0, 1600.0),
                1.0,
                0,
            });

    QCOMPARE(entries.size(), std::size_t(2));
    QCOMPARE(entries[1].identity.tileKey, highBucketRequest.key);
    QVERIFY(entries[1].identity.resolutionIndependent);
    QVERIFY(displayedSurface.staticTileSurface()->containsTile(lowBucketRequest.key));
    QVERIFY(displayedSurface.staticTileSurface()->containsTile(highBucketRequest.key));
}

void TestImageRendering::svgZoomOutThenInSkipsStaleLowBucketTiles()
{
    KiriView::StaticTileSurface surface = svgTileSurface(QSize(1000, 1000));
    const KiriView::TileRequest lowBucketRequest
        = KiriView::ImageTileGeometryPolicy::svgRasterTileRequests(
            QSize(1000, 1000), 512, 1, QSizeF(500.0, 500.0), QRectF(0.0, 0.0, 500.0, 500.0), 1.0)
              .front();
    const KiriView::TileRequest highBucketRequest
        = KiriView::ImageTileGeometryPolicy::svgRasterTileRequests(QSize(1000, 1000), 512, 1,
            QSizeF(1600.0, 1600.0), QRectF(0.0, 0.0, 1600.0, 1600.0), 1.0)
              .front();
    QVERIFY(lowBucketRequest.key.scaleBucket < highBucketRequest.key.scaleBucket);
    QVERIFY(surface.insertTile(decodedTileForRequest(lowBucketRequest)));
    QVERIFY(surface.insertTile(decodedTileForRequest(highBucketRequest)));

    const KiriView::DisplayedImageSurface displayedSurface(std::move(surface));
    const std::vector<KiriView::ImageSurfaceDrawEntry> entries
        = KiriView::imageSurfaceDrawEntries(displayedSurface,
            KiriView::ImageSurfaceDrawContext {
                QRectF(0.0, 0.0, 1600.0, 1600.0),
                QSizeF(1600.0, 1600.0),
                QRectF(0.0, 0.0, 1600.0, 1600.0),
                1.0,
                0,
            });

    QCOMPARE(entries.size(), std::size_t(2));
    QCOMPARE(entries[1].identity.tileKey, highBucketRequest.key);
}

void TestImageRendering::svgTileDrawEntriesUseFractionalDisplayCoverage()
{
    KiriView::StaticTileSurface surface = svgTileSurface(QSize(48, 48));
    const std::vector<KiriView::TileRequest> requests
        = KiriView::ImageTileGeometryPolicy::svgRasterTileRequests(
            QSize(48, 48), 512, 1, QSizeF(1920.0, 1920.0), QRectF(0.0, 0.0, 1920.0, 1920.0), 1.0);
    const auto firstIt = std::find_if(
        requests.cbegin(), requests.cend(), [](const KiriView::TileRequest &request) {
            return request.key.x == 0 && request.key.y == 0;
        });
    const auto secondIt = std::find_if(
        requests.cbegin(), requests.cend(), [](const KiriView::TileRequest &request) {
            return request.key.x == 1 && request.key.y == 0;
        });
    QVERIFY(firstIt != requests.cend());
    QVERIFY(secondIt != requests.cend());
    QVERIFY(firstIt->displaySourceRectF.width() > 8.0);
    QVERIFY(firstIt->displaySourceRectF.width() < 9.0);
    QVERIFY(surface.insertTile(decodedTileForRequest(*firstIt)));
    QVERIFY(surface.insertTile(decodedTileForRequest(*secondIt)));

    const KiriView::DisplayedImageSurface displayedSurface(std::move(surface));
    const std::vector<KiriView::ImageSurfaceDrawEntry> entries
        = KiriView::imageSurfaceDrawEntries(displayedSurface,
            KiriView::ImageSurfaceDrawContext {
                QRectF(0.0, 0.0, 1920.0, 1920.0),
                QSizeF(1920.0, 1920.0),
                QRectF(0.0, 0.0, 1920.0, 1920.0),
                1.0,
                0,
            });
    const auto firstEntry = std::find_if(
        entries.cbegin(), entries.cend(), [&firstIt](const KiriView::ImageSurfaceDrawEntry &entry) {
            return entry.identity.tileKey == firstIt->key;
        });
    const auto secondEntry = std::find_if(entries.cbegin(), entries.cend(),
        [&secondIt](const KiriView::ImageSurfaceDrawEntry &entry) {
            return entry.identity.tileKey == secondIt->key;
        });
    QVERIFY(firstEntry != entries.cend());
    QVERIFY(secondEntry != entries.cend());
    QVERIFY(qAbs(firstEntry->targetRect.right() - secondEntry->targetRect.left()) < 0.000001);
    QVERIFY(firstEntry->targetRect.width() < 360.0);
    QVERIFY(secondEntry->targetRect.left() < 360.0);
}

void TestImageRendering::rotatedFullImageDrawEntriesRotateTextureCoordinates()
{
    const KiriView::DisplayedImageSurface surface(
        KiriView::LegacyFrameSurface { KiriView::TestSupport::testImage(100, 200) });
    const QRectF targetRect(10.0, 20.0, 200.0, 100.0);

    const std::vector<KiriView::ImageSurfaceDrawEntry> clockwiseEntries
        = KiriView::imageSurfaceDrawEntries(surface, targetRect, 90);
    QCOMPARE(clockwiseEntries.size(), std::size_t(1));
    QCOMPARE(clockwiseEntries[0].targetRect, targetRect);
    QCOMPARE(clockwiseEntries[0].textureTransform.origin, QPointF(0.0, 1.0));
    QCOMPARE(clockwiseEntries[0].textureTransform.xAxis, QPointF(0.0, -1.0));
    QCOMPARE(clockwiseEntries[0].textureTransform.yAxis, QPointF(1.0, 0.0));

    const std::vector<KiriView::ImageSurfaceDrawEntry> upsideDownEntries
        = KiriView::imageSurfaceDrawEntries(surface, targetRect, 180);
    QCOMPARE(upsideDownEntries[0].textureTransform.origin, QPointF(1.0, 1.0));
    QCOMPARE(upsideDownEntries[0].textureTransform.xAxis, QPointF(-1.0, 0.0));
    QCOMPARE(upsideDownEntries[0].textureTransform.yAxis, QPointF(0.0, -1.0));

    const std::vector<KiriView::ImageSurfaceDrawEntry> counterclockwiseEntries
        = KiriView::imageSurfaceDrawEntries(surface, targetRect, 270);
    QCOMPARE(counterclockwiseEntries[0].textureTransform.origin, QPointF(1.0, 0.0));
    QCOMPARE(counterclockwiseEntries[0].textureTransform.xAxis, QPointF(0.0, 1.0));
    QCOMPARE(counterclockwiseEntries[0].textureTransform.yAxis, QPointF(-1.0, 0.0));
}

void TestImageRendering::rotatedStaticTileDrawEntriesMapSourceRects()
{
    const QImage sourceImage = KiriView::TestSupport::testImage(1000, 500);
    KiriView::StaticTileSurface surface(KiriView::TestSupport::staticTestImagePayload(sourceImage,
                                            KiriView::TestSupport::testImage(250, 125)),
        testTileCacheByteBudget);
    QVERIFY(surface.insertTile(KiriView::DecodedTile {
        KiriView::TileKey { 0, 0, 0 },
        QSize(1000, 500),
        QRect(250, 100, 500, 200),
        QRect(249, 99, 502, 202),
        KiriView::TestSupport::testImage(502, 202),
    }));

    const KiriView::DisplayedImageSurface displayedSurface(std::move(surface));

    const std::vector<KiriView::ImageSurfaceDrawEntry> clockwiseEntries
        = KiriView::imageSurfaceDrawEntries(
            displayedSurface, QRectF(10.0, 20.0, 500.0, 1000.0), 90);
    QCOMPARE(clockwiseEntries.size(), std::size_t(2));
    QCOMPARE(clockwiseEntries[1].targetRect, QRectF(210.0, 270.0, 200.0, 500.0));
    QVERIFY(clockwiseEntries[1].textureTransform.xAxis.y() < 0.0);
    QVERIFY(clockwiseEntries[1].textureTransform.yAxis.x() > 0.0);

    const std::vector<KiriView::ImageSurfaceDrawEntry> upsideDownEntries
        = KiriView::imageSurfaceDrawEntries(
            displayedSurface, QRectF(10.0, 20.0, 1000.0, 500.0), 180);
    QCOMPARE(upsideDownEntries[1].targetRect, QRectF(260.0, 220.0, 500.0, 200.0));
    QVERIFY(upsideDownEntries[1].textureTransform.xAxis.x() < 0.0);
    QVERIFY(upsideDownEntries[1].textureTransform.yAxis.y() < 0.0);

    const std::vector<KiriView::ImageSurfaceDrawEntry> counterclockwiseEntries
        = KiriView::imageSurfaceDrawEntries(
            displayedSurface, QRectF(10.0, 20.0, 500.0, 1000.0), 270);
    QCOMPARE(counterclockwiseEntries[1].targetRect, QRectF(110.0, 270.0, 200.0, 500.0));
    QVERIFY(counterclockwiseEntries[1].textureTransform.xAxis.y() > 0.0);
    QVERIFY(counterclockwiseEntries[1].textureTransform.yAxis.x() < 0.0);
}

QTEST_GUILESS_MAIN(TestImageRendering)

#include "test_imagerendering.moc"
