// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "predecodecache.h"

#include <QImage>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <Qt>
#include <optional>
#include <vector>

namespace {
using KiriView::TestSupport::indexedImageUrl;
using KiriView::TestSupport::staticTestImagePayload;

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

KiriView::ArchiveDocumentLocation comicBookArchiveDocument()
{
    return KiriView::ArchiveDocumentLocation::fromUrls(
        QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")),
        QUrl(QStringLiteral("zip:///books/book.cbz/")), KiriView::ArchiveDocumentKind::ComicBook);
}
}

class TestPredecodeCache : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void queueContainsOnlyMissingWindowImages();
    void takeNextRequestDiscardsSkippedQueuePrefix();
    void schedulingPredicatesGatePredecodeWork();
    void byteBudgetUsesPreferredLimitAndSystemMemoryCap();
    void cacheEligibilityUsesByteBudgetPolicy();
    void cacheStoresAndFindsWindowImages();
    void cacheRejectsUncacheableAndOversizedImages();
    void cacheEvictsLowestPriorityImagesWhenBudgetIsExceeded();
};

void TestPredecodeCache::queueContainsOnlyMissingWindowImages()
{
    KiriView::PredecodeCache cache;
    const QUrl displayedUrl = indexedImageUrl(0);
    const QUrl firstQueuedUrl = indexedImageUrl(1);
    const QUrl secondQueuedUrl = indexedImageUrl(2);

    cache.setWindowUrls({ displayedUrl, firstQueuedUrl, firstQueuedUrl, QUrl(), secondQueuedUrl });
    const KiriView::ArchiveDocumentLocation archiveDocument = comicBookArchiveDocument();
    const QImage firstImage = cacheImage();
    cache.cacheImage(firstQueuedUrl, archiveDocument, staticTestImagePayload(firstImage));
    cache.enqueueMissingWindowLoads(displayedUrl, archiveDocument, QUrl());

    QVERIFY(cache.isInFlight(secondQueuedUrl, QUrl()));
    QVERIFY(!cache.takeNextRequest(indexedImageUrl(9)).has_value());

    const std::optional<KiriView::PredecodeRequest> request = cache.takeNextRequest(QUrl());
    QVERIFY(request.has_value());
    QCOMPARE(request->url, secondQueuedUrl);
    QCOMPARE(request->archiveDocument.rootUrl(), archiveDocument.rootUrl());
    QVERIFY(!cache.takeNextRequest(QUrl()).has_value());
}

void TestPredecodeCache::takeNextRequestDiscardsSkippedQueuePrefix()
{
    KiriView::PredecodeCache cache;
    const QUrl cachedQueuedUrl = indexedImageUrl(0);
    const QUrl firstRequestUrl = indexedImageUrl(1);
    const QUrl secondRequestUrl = indexedImageUrl(2);
    const KiriView::ArchiveDocumentLocation archiveDocument = comicBookArchiveDocument();

    cache.setWindowUrls({ cachedQueuedUrl, firstRequestUrl, secondRequestUrl });
    cache.enqueueMissingWindowLoads(indexedImageUrl(9), archiveDocument, QUrl());
    cache.cacheImage(cachedQueuedUrl, archiveDocument, staticTestImagePayload(cacheImage()));

    const std::optional<KiriView::PredecodeRequest> firstRequest = cache.takeNextRequest(QUrl());
    QVERIFY(firstRequest.has_value());
    QCOMPARE(firstRequest->url, firstRequestUrl);

    const std::optional<KiriView::PredecodeRequest> secondRequest = cache.takeNextRequest(QUrl());
    QVERIFY(secondRequest.has_value());
    QCOMPARE(secondRequest->url, secondRequestUrl);
    QVERIFY(!cache.takeNextRequest(QUrl()).has_value());
}

void TestPredecodeCache::schedulingPredicatesGatePredecodeWork()
{
    QVERIFY(KiriView::predecodeDisplayedImageCanSchedule(true, false, true));
    QVERIFY(!KiriView::predecodeDisplayedImageCanSchedule(false, false, true));
    QVERIFY(!KiriView::predecodeDisplayedImageCanSchedule(true, true, true));
    QVERIFY(!KiriView::predecodeDisplayedImageCanSchedule(true, false, false));

    QVERIFY(KiriView::predecodeContextCanSchedule(false, true));
    QVERIFY(!KiriView::predecodeContextCanSchedule(true, true));
    QVERIFY(!KiriView::predecodeContextCanSchedule(false, false));

    QVERIFY(KiriView::predecodeRequestCanStart(true, false, false, true));
    QVERIFY(!KiriView::predecodeRequestCanStart(false, false, false, true));
    QVERIFY(!KiriView::predecodeRequestCanStart(true, true, false, true));
    QVERIFY(!KiriView::predecodeRequestCanStart(true, false, true, true));
    QVERIFY(!KiriView::predecodeRequestCanStart(true, false, false, false));
}

void TestPredecodeCache::byteBudgetUsesPreferredLimitAndSystemMemoryCap()
{
    const qsizetype preferredByteBudget = 1024 * 1024 * 1024;
    QCOMPARE(KiriView::PredecodeCache::preferredByteBudget(), preferredByteBudget);
    QCOMPARE(KiriView::PredecodeCache::byteBudgetForSystemMemory(0), preferredByteBudget);
    QCOMPARE(KiriView::PredecodeCache::byteBudgetForSystemMemory(preferredByteBudget),
        preferredByteBudget / 8);
    QCOMPARE(KiriView::PredecodeCache::byteBudgetForSystemMemory(preferredByteBudget * 16),
        preferredByteBudget);
    QVERIFY(KiriView::PredecodeCache::defaultByteBudget() > 0);
    QVERIFY(KiriView::PredecodeCache::defaultByteBudget() <= preferredByteBudget);
}

void TestPredecodeCache::cacheEligibilityUsesByteBudgetPolicy()
{
    const KiriView::StaticImagePayload image = staticTestImagePayload(cacheImage());
    const qsizetype byteCost = image.byteCost();

    QVERIFY(KiriView::PredecodeCache::canCacheImage(image));
    QVERIFY(KiriView::PredecodeCache::canCacheImage(image, byteCost));
    QVERIFY(!KiriView::PredecodeCache::canCacheImage(image, byteCost - 1));
    QVERIFY(!KiriView::PredecodeCache::canCacheImage(KiriView::StaticImagePayload {}, byteCost));
    QVERIFY(!KiriView::PredecodeCache::canCacheImage(image, 0));
}

void TestPredecodeCache::cacheStoresAndFindsWindowImages()
{
    KiriView::PredecodeCache cache(80);
    const QUrl url = indexedImageUrl(0);
    const KiriView::ArchiveDocumentLocation archiveDocument = comicBookArchiveDocument();
    const QImage image = cacheImage();

    cache.setWindowUrls({ url });
    cache.cacheImage(url, archiveDocument,
        staticTestImagePayload(image, KiriView::StaticImageDisplayHints { 0.5 }));

    const std::optional<KiriView::PredecodedImage> found = cache.findImage(url);
    QVERIFY(found.has_value());
    QCOMPARE(found->staticImage.preview.size(), image.size());
    QCOMPARE(found->location.imageUrl(), url);
    QCOMPARE(found->location.archiveDocumentRootUrl(), archiveDocument.rootUrl());
    QCOMPARE(found->staticImage.displayHints.firstDisplayPixelsPerSourcePixel, 0.5);
}

void TestPredecodeCache::cacheRejectsUncacheableAndOversizedImages()
{
    KiriView::PredecodeCache cache(160);
    const QUrl url = indexedImageUrl(0);
    const QUrl outsideWindowUrl = indexedImageUrl(9);
    const KiriView::ArchiveDocumentLocation archiveDocument = comicBookArchiveDocument();

    cache.setWindowUrls({ url });
    const QImage image = cacheImage();
    cache.cacheDisplayedImage(false, url, archiveDocument, staticTestImagePayload(image));
    QVERIFY(!cache.hasImage(url));

    cache.cacheDisplayedImage(true, url, archiveDocument, KiriView::StaticImagePayload {});
    QVERIFY(!cache.hasImage(url));

    cache.cacheImage(outsideWindowUrl, archiveDocument, staticTestImagePayload(image));
    QVERIFY(!cache.hasImage(outsideWindowUrl));

    const QImage largeImage = tooLargeImage();
    cache.cacheImage(url, archiveDocument, staticTestImagePayload(largeImage));
    QVERIFY(!cache.hasImage(url));
}

void TestPredecodeCache::cacheEvictsLowestPriorityImagesWhenBudgetIsExceeded()
{
    KiriView::PredecodeCache cache(160);
    const QUrl firstUrl = indexedImageUrl(0);
    const QUrl secondUrl = indexedImageUrl(1);
    const QUrl thirdUrl = indexedImageUrl(2);
    const KiriView::ArchiveDocumentLocation archiveDocument = comicBookArchiveDocument();

    cache.setWindowUrls({ firstUrl, secondUrl, thirdUrl });
    const QImage image = cacheImage();
    cache.cacheImage(thirdUrl, archiveDocument, staticTestImagePayload(image));
    cache.cacheImage(firstUrl, archiveDocument, staticTestImagePayload(image));
    cache.cacheImage(secondUrl, archiveDocument, staticTestImagePayload(image));

    QVERIFY(cache.hasImage(firstUrl));
    QVERIFY(cache.hasImage(secondUrl));
    QVERIFY(!cache.hasImage(thirdUrl));
}

QTEST_GUILESS_MAIN(TestPredecodeCache)

#include "test_predecodecache.moc"
