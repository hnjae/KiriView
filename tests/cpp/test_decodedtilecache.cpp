// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedtilecache.h"

#include <QImage>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QTest>
#include <Qt>
#include <cstddef>

namespace {
KiriView::DecodedTile decodedTile(KiriView::TileKey key, QSize size)
{
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return KiriView::DecodedTile { key, size, QRect(QPoint(0, 0), size), QRect(QPoint(0, 0), size),
        image };
}
}

class TestDecodedTileCache : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void evictsLeastRecentlyUsedTilesToBudget();
    void replacesExistingTileWithoutDoubleCounting();
    void reportsRejectedTiles();
    void clearRemovesTilesAndByteCost();
};

void TestDecodedTileCache::evictsLeastRecentlyUsedTilesToBudget()
{
    KiriView::DecodedTileCache cache(32);
    const KiriView::TileKey first { 0, 0, 0 };
    const KiriView::TileKey second { 0, 1, 0 };
    const KiriView::TileKey third { 0, 2, 0 };

    QVERIFY(cache.insert(decodedTile(first, QSize(2, 2))));
    QVERIFY(cache.insert(decodedTile(second, QSize(2, 2))));
    QVERIFY(cache.find(first).has_value());
    QVERIFY(cache.insert(decodedTile(third, QSize(2, 2))));

    QVERIFY(cache.contains(first));
    QVERIFY(!cache.contains(second));
    QVERIFY(cache.contains(third));
    QVERIFY(cache.byteCost() <= cache.byteBudget());
}

void TestDecodedTileCache::replacesExistingTileWithoutDoubleCounting()
{
    KiriView::DecodedTileCache cache(80);
    const KiriView::TileKey key { 0, 0, 0 };

    QVERIFY(cache.insert(decodedTile(key, QSize(2, 2))));
    const qsizetype firstByteCost = cache.byteCost();
    QVERIFY(cache.insert(decodedTile(key, QSize(3, 1))));

    QCOMPARE(cache.tiles().size(), std::size_t(1));
    QVERIFY(cache.contains(key));
    QVERIFY(cache.byteCost() > 0);
    QVERIFY(cache.byteCost() != firstByteCost);
    QVERIFY(cache.byteCost() <= cache.byteBudget());
}

void TestDecodedTileCache::reportsRejectedTiles()
{
    KiriView::DecodedTileCache cache(1);
    const KiriView::TileKey key { 0, 0, 0 };

    QVERIFY(!cache.insert(decodedTile(key, QSize(1, 1))));

    QVERIFY(!cache.contains(key));
    QCOMPARE(cache.byteCost(), qsizetype(0));
}

void TestDecodedTileCache::clearRemovesTilesAndByteCost()
{
    KiriView::DecodedTileCache cache(32);
    const KiriView::TileKey key { 0, 0, 0 };

    QVERIFY(cache.insert(decodedTile(key, QSize(2, 2))));
    QVERIFY(cache.contains(key));

    cache.clear();

    QVERIFY(!cache.contains(key));
    QVERIFY(cache.tiles().empty());
    QCOMPARE(cache.byteCost(), qsizetype(0));
}

QTEST_GUILESS_MAIN(TestDecodedTileCache)

#include "test_decodedtilecache.moc"
