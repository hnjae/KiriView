// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagesurface.h"

#include <QObject>
#include <QTest>
#include <optional>

class TestImageSurface : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void staticImagePayloadReportsByteCostWithinBudget();
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

QTEST_GUILESS_MAIN(TestImageSurface)

#include "test_imagesurface.moc"
