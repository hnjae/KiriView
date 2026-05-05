// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagebytecost.h"

#include <QImage>
#include <QObject>
#include <QSize>
#include <QTest>
#include <Qt>
#include <limits>

class TestImageByteCost : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageByteCostUsesQtImageStorageSize();
    void estimatedRgbaByteCostHandlesEmptyAndOverflow();
    void systemMemoryCappedByteBudgetCapsPreferredBudget();
    void defaultSystemMemoryCappedByteBudgetReturnsPositiveCappedBudget();
};

void TestImageByteCost::imageByteCostUsesQtImageStorageSize()
{
    QImage image(10, 3, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);

    QCOMPARE(KiriView::imageByteCost(QImage()), qsizetype(0));
    QCOMPARE(KiriView::imageByteCost(image), image.sizeInBytes());
}

void TestImageByteCost::estimatedRgbaByteCostHandlesEmptyAndOverflow()
{
    QCOMPARE(KiriView::estimatedRgbaByteCost(QSize()), qsizetype(0));
    QCOMPARE(KiriView::estimatedRgbaByteCost(QSize(10, 3)), qsizetype(120));
    QCOMPARE(KiriView::estimatedRgbaByteCost(
                 QSize(std::numeric_limits<int>::max(), std::numeric_limits<int>::max())),
        std::numeric_limits<qsizetype>::max());
}

void TestImageByteCost::systemMemoryCappedByteBudgetCapsPreferredBudget()
{
    constexpr qsizetype preferredByteBudget = 1024;

    QCOMPARE(
        KiriView::systemMemoryCappedByteBudget(preferredByteBudget, 0, 8), preferredByteBudget);
    QCOMPARE(KiriView::systemMemoryCappedByteBudget(preferredByteBudget, preferredByteBudget, 8),
        preferredByteBudget / 8);
    QCOMPARE(
        KiriView::systemMemoryCappedByteBudget(preferredByteBudget, preferredByteBudget * 16, 8),
        preferredByteBudget);
    QCOMPARE(KiriView::systemMemoryCappedByteBudget(preferredByteBudget, preferredByteBudget, 0),
        preferredByteBudget);
    QCOMPARE(KiriView::systemMemoryCappedByteBudget(-1, 0, 8), qsizetype(0));
}

void TestImageByteCost::defaultSystemMemoryCappedByteBudgetReturnsPositiveCappedBudget()
{
    constexpr qsizetype preferredByteBudget = 1024;

    const qsizetype byteBudget
        = KiriView::defaultSystemMemoryCappedByteBudget(preferredByteBudget, 8);

    QVERIFY(byteBudget > 0);
    QVERIFY(byteBudget <= preferredByteBudget);
}

QTEST_GUILESS_MAIN(TestImageByteCost)

#include "test_imagebytecost.moc"
