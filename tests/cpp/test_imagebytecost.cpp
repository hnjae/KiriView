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

QTEST_GUILESS_MAIN(TestImageByteCost)

#include "test_imagebytecost.moc"
