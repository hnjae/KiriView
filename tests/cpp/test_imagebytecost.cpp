// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "cache/imagebytecost.h"

#include "cache/imagebyteaccounting.h"

#include <QImage>
#include <QObject>
#include <QSize>
#include <QTest>
#include <Qt>
#include <cstdint>
#include <limits>

class TestImageByteCost : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void byteAccountingSaturatesProductsSumsAndQtSizes();
    void imageByteCostUsesQtImageStorageSize();
    void estimatedRgbaByteCostHandlesEmptyAndOverflow();
};

void TestImageByteCost::byteAccountingSaturatesProductsSumsAndQtSizes()
{
    QCOMPARE(KiriView::saturatedQtByteSize(-5), qsizetype(-5));
    QCOMPARE(KiriView::saturatedQtByteSize(std::numeric_limits<std::int64_t>::max()),
        std::numeric_limits<qsizetype>::max());
    QCOMPARE(KiriView::saturatedPositiveByteProduct(0, 10), std::int64_t(0));
    QCOMPARE(KiriView::saturatedPositiveByteProduct(-1, 10), std::int64_t(0));
    QCOMPARE(KiriView::saturatedPositiveByteProduct(10, 3), std::int64_t(30));
    QCOMPARE(KiriView::saturatedPositiveByteProduct(std::numeric_limits<std::int64_t>::max(), 2),
        std::numeric_limits<std::int64_t>::max());
    QCOMPARE(
        KiriView::saturatedQtByteProduct(std::numeric_limits<std::int64_t>::max(), std::int64_t(2)),
        std::numeric_limits<qsizetype>::max());
    QCOMPARE(KiriView::saturatedQtByteSum(10, 3), qsizetype(13));
    QCOMPARE(KiriView::saturatedQtByteSum(-5, 3), qsizetype(3));
    QCOMPARE(KiriView::saturatedQtByteSum(std::numeric_limits<qsizetype>::max(), 1),
        std::numeric_limits<qsizetype>::max());
}

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

QTEST_GUILESS_MAIN(TestImageByteCost)

#include "test_imagebytecost.moc"
