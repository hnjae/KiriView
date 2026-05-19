// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerendering.h"

#include "image_test_support.h"

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QTest>
#include <limits>
#include <utility>
#include <vector>

namespace {
constexpr qsizetype testTileCacheByteBudget = KiriView::imageFullDecodeFallbackByteLimit;
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
        = KiriView::imageSurfaceDrawEntries(displayedSurface, targetRect);

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
