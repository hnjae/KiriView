// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/qimagereaderscaledlevelcache.h"

#include <QColor>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QTest>
#include <Qt>

namespace {
QImage cacheImage(QSize size, QColor color = Qt::transparent)
{
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    image.fill(color);
    return image;
}
}

class TestQImageReaderScaledLevelCache : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void evictsLeastRecentlyUsedLevelsToBudget();
    void replacesExistingLevelWithoutDoubleCounting();
    void rejectsNullAndOversizedImages();
    void clearRemovesImagesAndByteCost();
};

void TestQImageReaderScaledLevelCache::evictsLeastRecentlyUsedLevelsToBudget()
{
    kiriview::QImageReaderScaledLevelCache cache(32);

    QVERIFY(cache.insert(0, cacheImage(QSize(2, 2), Qt::red)));
    QVERIFY(cache.insert(1, cacheImage(QSize(2, 2), Qt::green)));
    QVERIFY(cache.find(0).has_value());
    QVERIFY(cache.insert(2, cacheImage(QSize(2, 2), Qt::blue)));

    QVERIFY(cache.contains(0));
    QVERIFY(!cache.contains(1));
    QVERIFY(cache.contains(2));
    QVERIFY(cache.byteCost() <= cache.byteBudget());
}

void TestQImageReaderScaledLevelCache::replacesExistingLevelWithoutDoubleCounting()
{
    kiriview::QImageReaderScaledLevelCache cache(80);

    QVERIFY(cache.insert(0, cacheImage(QSize(2, 2))));
    const qsizetype firstByteCost = cache.byteCost();
    QVERIFY(cache.insert(0, cacheImage(QSize(3, 1))));

    QVERIFY(cache.contains(0));
    QVERIFY(cache.byteCost() > 0);
    QVERIFY(cache.byteCost() != firstByteCost);
    QVERIFY(cache.byteCost() <= cache.byteBudget());
}

void TestQImageReaderScaledLevelCache::rejectsNullAndOversizedImages()
{
    kiriview::QImageReaderScaledLevelCache cache(1);

    QVERIFY(!cache.insert(0, QImage()));
    QVERIFY(!cache.insert(1, cacheImage(QSize(1, 1))));

    QVERIFY(!cache.contains(0));
    QVERIFY(!cache.contains(1));
    QCOMPARE(cache.byteCost(), qsizetype(0));
}

void TestQImageReaderScaledLevelCache::clearRemovesImagesAndByteCost()
{
    kiriview::QImageReaderScaledLevelCache cache(16);

    QVERIFY(cache.insert(0, cacheImage(QSize(2, 2))));
    QVERIFY(cache.contains(0));

    cache.clear();

    QVERIFY(!cache.contains(0));
    QVERIFY(!cache.find(0).has_value());
    QCOMPARE(cache.byteCost(), qsizetype(0));
}

QTEST_GUILESS_MAIN(TestQImageReaderScaledLevelCache)

#include "test_qimagereaderscaledlevelcache.moc"
