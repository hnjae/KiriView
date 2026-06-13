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
    QCOMPARE(kiriview::saturatedQtByteSize(-5), qsizetype(-5));
    QCOMPARE(kiriview::saturatedQtByteSize(std::numeric_limits<std::int64_t>::max()),
        std::numeric_limits<qsizetype>::max());
    QCOMPARE(kiriview::saturatedPositiveByteProduct(0, 10), std::int64_t(0));
    QCOMPARE(kiriview::saturatedPositiveByteProduct(-1, 10), std::int64_t(0));
    QCOMPARE(kiriview::saturatedPositiveByteProduct(10, 3), std::int64_t(30));
    QCOMPARE(kiriview::saturatedPositiveByteProduct(std::numeric_limits<std::int64_t>::max(), 2),
        std::numeric_limits<std::int64_t>::max());
    QCOMPARE(
        kiriview::saturatedQtByteProduct(std::numeric_limits<std::int64_t>::max(), std::int64_t(2)),
        std::numeric_limits<qsizetype>::max());
    QCOMPARE(kiriview::saturatedQtByteSum(10, 3), qsizetype(13));
    QCOMPARE(kiriview::saturatedQtByteSum(-5, 3), qsizetype(3));
    QCOMPARE(kiriview::saturatedQtByteSum(std::numeric_limits<qsizetype>::max(), 1),
        std::numeric_limits<qsizetype>::max());
}

void TestImageByteCost::imageByteCostUsesQtImageStorageSize()
{
    QImage image(10, 3, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);

    QCOMPARE(kiriview::imageByteCost(QImage()), qsizetype(0));
    QCOMPARE(kiriview::imageByteCost(image), image.sizeInBytes());
}

void TestImageByteCost::estimatedRgbaByteCostHandlesEmptyAndOverflow()
{
    QCOMPARE(kiriview::estimatedRgbaByteCost(QSize()), qsizetype(0));
    QCOMPARE(kiriview::estimatedRgbaByteCost(QSize(10, 3)), qsizetype(120));
    QCOMPARE(kiriview::estimatedRgbaByteCost(
                 QSize(std::numeric_limits<int>::max(), std::numeric_limits<int>::max())),
        std::numeric_limits<qsizetype>::max());
}

QTEST_GUILESS_MAIN(TestImageByteCost)

#include "test_imagebytecost.moc"
