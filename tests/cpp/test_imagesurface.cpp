// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagesurface.h"

#include <QObject>
#include <QTest>
#include <optional>
#include <utility>

class TestImageSurface : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void staticImagePayloadReportsByteCostWithinBudget();
    void displayedImageSurfaceExposesOnlyActivePayload();
};

void TestImageSurface::staticImagePayloadReportsByteCostWithinBudget()
{
    const KiriView::StaticImagePayload image
        = KiriView::TestSupport::staticDecodedTestImage(KiriView::TestSupport::testImage(2, 1))
              .staticImage;
    const qsizetype byteCost = image.byteCost();

    const std::optional<qsizetype> acceptedByteCost = image.byteCostWithinBudget(byteCost);
    QVERIFY(acceptedByteCost.has_value());
    QCOMPARE(*acceptedByteCost, byteCost);
    QVERIFY(!image.byteCostWithinBudget(byteCost - 1).has_value());
    QVERIFY(!image.byteCostWithinBudget(0).has_value());
    QVERIFY(!KiriView::StaticImagePayload().byteCostWithinBudget(byteCost).has_value());
}

void TestImageSurface::displayedImageSurfaceExposesOnlyActivePayload()
{
    const QImage legacyImage = KiriView::TestSupport::testImage(2, 1);
    const KiriView::DisplayedImageSurface legacySurface(
        KiriView::LegacyFrameSurface { legacyImage });

    QVERIFY(!legacySurface.isNull());
    QCOMPARE(legacySurface.imageSize(), QSize(2, 1));
    QCOMPARE(KiriView::displayedImageSurfaceSize(legacySurface), QSize(2, 1));
    QVERIFY(!KiriView::displayedImageSurfaceIsNull(legacySurface));
    QVERIFY(legacySurface.legacyFrameSurface() != nullptr);
    QVERIFY(legacySurface.staticTileSurface() == nullptr);

    KiriView::StaticImagePayload staticImage
        = KiriView::TestSupport::staticDecodedTestImage(KiriView::TestSupport::testImage(3, 2))
              .staticImage;
    const KiriView::DisplayedImageSurface staticSurface(
        KiriView::StaticTileSurface { std::move(staticImage) });

    QVERIFY(!staticSurface.isNull());
    QCOMPARE(staticSurface.imageSize(), QSize(3, 2));
    QVERIFY(staticSurface.legacyFrameSurface() == nullptr);
    QVERIFY(staticSurface.staticTileSurface() != nullptr);
}

QTEST_GUILESS_MAIN(TestImageSurface)

#include "test_imagesurface.moc"
