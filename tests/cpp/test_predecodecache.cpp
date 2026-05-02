// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodecache.h"

#include <QImage>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <Qt>
#include <optional>
#include <vector>

namespace {
QUrl imageUrl(int index)
{
    return QUrl(QStringLiteral("file:///images/%1.png").arg(index, 2, 10, QLatin1Char('0')));
}

QImage cacheImage()
{
    QImage image(10, 1, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

QImage tooLargeImage()
{
    QImage image(30, 1, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

QUrl comicBookRootUrl() { return QUrl(QStringLiteral("zip:///books/book.cbz/")); }
}

class TestPredecodeCache : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void queueContainsOnlyMissingWindowImages();
    void cacheStoresAndFindsWindowImages();
    void cacheRejectsUncacheableAndOversizedImages();
    void cacheEvictsLowestPriorityImagesWhenBudgetIsExceeded();
};

void TestPredecodeCache::queueContainsOnlyMissingWindowImages()
{
    KiriView::PredecodeCache cache;
    const QUrl displayedUrl = imageUrl(0);
    const QUrl firstQueuedUrl = imageUrl(1);
    const QUrl secondQueuedUrl = imageUrl(2);

    cache.setWindowUrls({ displayedUrl, firstQueuedUrl, firstQueuedUrl, QUrl(), secondQueuedUrl });
    cache.cacheImage(firstQueuedUrl, comicBookRootUrl(), cacheImage());
    cache.enqueueMissingWindowLoads(displayedUrl, comicBookRootUrl(), QUrl());

    QVERIFY(cache.isInFlight(secondQueuedUrl, QUrl()));
    QVERIFY(!cache.takeNextRequest(imageUrl(9)).has_value());

    const std::optional<KiriView::PredecodeRequest> request = cache.takeNextRequest(QUrl());
    QVERIFY(request.has_value());
    QCOMPARE(request->url, secondQueuedUrl);
    QCOMPARE(request->comicBookRootUrl, comicBookRootUrl());
    QVERIFY(!cache.takeNextRequest(QUrl()).has_value());
}

void TestPredecodeCache::cacheStoresAndFindsWindowImages()
{
    KiriView::PredecodeCache cache(80);
    const QUrl url = imageUrl(0);
    const QUrl rootUrl = comicBookRootUrl();
    const QImage image = cacheImage();

    cache.setWindowUrls({ url });
    cache.cacheImage(url, rootUrl, image);

    QImage foundImage;
    QUrl foundRootUrl;
    QVERIFY(cache.findImage(url, &foundImage, &foundRootUrl));
    QCOMPARE(foundImage.size(), image.size());
    QCOMPARE(foundRootUrl, rootUrl);
}

void TestPredecodeCache::cacheRejectsUncacheableAndOversizedImages()
{
    KiriView::PredecodeCache cache(80);
    const QUrl url = imageUrl(0);
    const QUrl outsideWindowUrl = imageUrl(9);

    cache.setWindowUrls({ url });
    cache.cacheDisplayedImage(false, url, comicBookRootUrl(), cacheImage());
    QVERIFY(!cache.hasImage(url));

    cache.cacheDisplayedImage(true, url, comicBookRootUrl(), QImage());
    QVERIFY(!cache.hasImage(url));

    cache.cacheImage(outsideWindowUrl, comicBookRootUrl(), cacheImage());
    QVERIFY(!cache.hasImage(outsideWindowUrl));

    cache.cacheImage(url, comicBookRootUrl(), tooLargeImage());
    QVERIFY(!cache.hasImage(url));
}

void TestPredecodeCache::cacheEvictsLowestPriorityImagesWhenBudgetIsExceeded()
{
    KiriView::PredecodeCache cache(80);
    const QUrl firstUrl = imageUrl(0);
    const QUrl secondUrl = imageUrl(1);
    const QUrl thirdUrl = imageUrl(2);
    const QUrl rootUrl = comicBookRootUrl();

    cache.setWindowUrls({ firstUrl, secondUrl, thirdUrl });
    cache.cacheImage(thirdUrl, rootUrl, cacheImage());
    cache.cacheImage(firstUrl, rootUrl, cacheImage());
    cache.cacheImage(secondUrl, rootUrl, cacheImage());

    QVERIFY(cache.hasImage(firstUrl));
    QVERIFY(cache.hasImage(secondUrl));
    QVERIFY(!cache.hasImage(thirdUrl));
}

QTEST_GUILESS_MAIN(TestPredecodeCache)

#include "test_predecodecache.moc"
