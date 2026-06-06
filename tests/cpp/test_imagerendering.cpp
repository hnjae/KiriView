// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/imagerendering.h"

#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <limits>

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

QTEST_GUILESS_MAIN(TestImageRendering)

#include "test_imagerendering.moc"
