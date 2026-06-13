// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/thumbnailimagestore.h"

#include <QColor>
#include <QImage>
#include <QObject>
#include <QString>
#include <QTest>
#include <QtGlobal>

class TestThumbnailImageStore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void evictsLeastRecentlyUsedImagesByByteBudget();
    void imageAccessRefreshesLruOrder();
    void visiblePriorityOutlivesNearbyWhenBudgetIsTight();
    void evictsBackgroundBeforeNearbyBeforeVisible();
    void explicitReleaseRemovesImageAndByteCost();
};

namespace {
QImage testImage(QColor color)
{
    QImage image(QSize(4, 4), QImage::Format_RGBA8888);
    image.fill(color);
    return image;
}
}

void TestThumbnailImageStore::evictsLeastRecentlyUsedImagesByByteBudget()
{
    kiriview::ThumbnailImageStore store(128);

    const QString first = store.insert(testImage(Qt::red));
    const QString second = store.insert(testImage(Qt::green));
    const QString third = store.insert(testImage(Qt::blue));

    QVERIFY(!first.isEmpty());
    QVERIFY(!second.isEmpty());
    QVERIFY(!third.isEmpty());
    QCOMPARE(store.size(), qsizetype(2));
    QCOMPARE(store.byteCost(), qsizetype(128));
    QVERIFY(store.image(first).isNull());
    QVERIFY(!store.image(second).isNull());
    QVERIFY(!store.image(third).isNull());
}

void TestThumbnailImageStore::imageAccessRefreshesLruOrder()
{
    kiriview::ThumbnailImageStore store(128);

    const QString first = store.insert(testImage(Qt::red));
    const QString second = store.insert(testImage(Qt::green));
    QVERIFY(!store.image(first).isNull());
    const QString third = store.insert(testImage(Qt::blue));

    QCOMPARE(store.size(), qsizetype(2));
    QVERIFY(!store.image(first).isNull());
    QVERIFY(store.image(second).isNull());
    QVERIFY(!store.image(third).isNull());
}

void TestThumbnailImageStore::visiblePriorityOutlivesNearbyWhenBudgetIsTight()
{
    kiriview::ThumbnailImageStore store(128);

    const QString visible
        = store.insert(testImage(Qt::red), kiriview::ThumbnailImageRetentionPriority::Visible);
    const QString nearby
        = store.insert(testImage(Qt::green), kiriview::ThumbnailImageRetentionPriority::Nearby);
    const QString newerNearby
        = store.insert(testImage(Qt::blue), kiriview::ThumbnailImageRetentionPriority::Nearby);

    QCOMPARE(store.size(), qsizetype(2));
    QVERIFY(!store.image(visible).isNull());
    QVERIFY(store.image(nearby).isNull());
    QVERIFY(!store.image(newerNearby).isNull());
}

void TestThumbnailImageStore::evictsBackgroundBeforeNearbyBeforeVisible()
{
    kiriview::ThumbnailImageStore store(128);

    const QString background
        = store.insert(testImage(Qt::red), kiriview::ThumbnailImageRetentionPriority::Background);
    const QString nearby
        = store.insert(testImage(Qt::green), kiriview::ThumbnailImageRetentionPriority::Nearby);
    const QString visible
        = store.insert(testImage(Qt::blue), kiriview::ThumbnailImageRetentionPriority::Visible);

    QCOMPARE(store.size(), qsizetype(2));
    QVERIFY(store.image(background).isNull());
    QVERIFY(!store.image(nearby).isNull());
    QVERIFY(!store.image(visible).isNull());

    const QString secondBackground
        = store.insert(testImage(Qt::cyan), kiriview::ThumbnailImageRetentionPriority::Background);
    QCOMPARE(store.size(), qsizetype(2));
    QVERIFY(store.image(secondBackground).isNull());
    QVERIFY(!store.image(nearby).isNull());
    QVERIFY(!store.image(visible).isNull());

    const QString newerNearby
        = store.insert(testImage(Qt::magenta), kiriview::ThumbnailImageRetentionPriority::Nearby);
    QCOMPARE(store.size(), qsizetype(2));
    QVERIFY(store.image(nearby).isNull());
    QVERIFY(!store.image(newerNearby).isNull());
    QVERIFY(!store.image(visible).isNull());
}

void TestThumbnailImageStore::explicitReleaseRemovesImageAndByteCost()
{
    kiriview::ThumbnailImageStore store(128);

    const QString id = store.insert(testImage(Qt::red));
    QVERIFY(!id.isEmpty());
    QCOMPARE(store.size(), qsizetype(1));
    QCOMPARE(store.byteCost(), qsizetype(64));

    store.release(id);
    QCOMPARE(store.size(), qsizetype(0));
    QCOMPARE(store.byteCost(), qsizetype(0));
    QVERIFY(store.image(id).isNull());
}

QTEST_GUILESS_MAIN(TestThumbnailImageStore)

#include "test_thumbnailimagestore.moc"
