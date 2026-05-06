// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerendering.h"

#include "image_test_support.h"

#include <QObject>
#include <QRectF>
#include <QTest>
#include <utility>
#include <vector>

class TestImageRendering : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void staticSurfaceDrawEntriesKeepPreviewAndTileRectsSeparate();
};

void TestImageRendering::staticSurfaceDrawEntriesKeepPreviewAndTileRectsSeparate()
{
    const QImage sourceImage = KiriView::TestSupport::testImage(1024, 1024);
    KiriView::StaticTileSurface surface(KiriView::TestSupport::staticTestImagePayload(
        sourceImage, KiriView::TestSupport::testImage(256, 256)));
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

QTEST_GUILESS_MAIN(TestImageRendering)

#include "test_imagerendering.moc"
