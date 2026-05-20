// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "predecode/predecodecache.h"

#include <QImage>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <Qt>
#include <optional>
#include <utility>
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

KiriView::PredecodeActiveLoads activeLoads(std::vector<QUrl> urls)
{
    return KiriView::PredecodeActiveLoads::fromUrls(std::move(urls));
}
}

class TestPredecodeCache : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void queueContainsOnlyMissingWindowImages();
    void queueSkipsAllDisplayedWindowImages();
    void takeNextRequestDiscardsSkippedQueuePrefix();
    void byteBudgetUsesPreferredLimitAndSystemMemoryCap();
    void cacheEligibilityUsesByteBudgetPolicy();
    void cacheStoresAndFindsWindowImages();
    void cacheRetainsDisplayedImagesBeforeAdjacentImages();
    void cacheRetainsRecentDisplayedImagesBeforeAdjacentImages();
    void cacheKeepsOnlyFourRecentDisplayedImages();
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
    cache.enqueueMissingWindowLoads(
        displayedUrl, archiveDocument, KiriView::PredecodeActiveLoads {});

    QVERIFY(cache.isInFlight(secondQueuedUrl, KiriView::PredecodeActiveLoads {}));

    const std::optional<KiriView::PredecodeRequest> request
        = cache.takeNextRequest(activeLoads({ indexedImageUrl(9) }));
    QVERIFY(request.has_value());
    QCOMPARE(request->url, secondQueuedUrl);
    QCOMPARE(request->archiveDocument.rootUrl(), archiveDocument.rootUrl());
    QVERIFY(!cache.takeNextRequest(KiriView::PredecodeActiveLoads {}).has_value());
}

void TestPredecodeCache::queueSkipsAllDisplayedWindowImages()
{
    KiriView::PredecodeCache cache;
    const QUrl primaryDisplayedUrl = indexedImageUrl(0);
    const QUrl secondaryDisplayedUrl = indexedImageUrl(1);
    const QUrl queuedUrl = indexedImageUrl(2);
    const KiriView::ArchiveDocumentLocation archiveDocument = comicBookArchiveDocument();

    cache.setDisplayedUrls({ primaryDisplayedUrl, secondaryDisplayedUrl });
    cache.setWindowUrls({ primaryDisplayedUrl, secondaryDisplayedUrl, queuedUrl });
    cache.enqueueMissingWindowLoads(
        primaryDisplayedUrl, archiveDocument, KiriView::PredecodeActiveLoads {});

    const std::optional<KiriView::PredecodeRequest> request
        = cache.takeNextRequest(KiriView::PredecodeActiveLoads {});
    QVERIFY(request.has_value());
    QCOMPARE(request->url, queuedUrl);
    QVERIFY(!cache.takeNextRequest(KiriView::PredecodeActiveLoads {}).has_value());
}

void TestPredecodeCache::takeNextRequestDiscardsSkippedQueuePrefix()
{
    KiriView::PredecodeCache cache;
    const QUrl cachedQueuedUrl = indexedImageUrl(0);
    const QUrl firstRequestUrl = indexedImageUrl(1);
    const QUrl secondRequestUrl = indexedImageUrl(2);
    const KiriView::ArchiveDocumentLocation archiveDocument = comicBookArchiveDocument();

    cache.setWindowUrls({ cachedQueuedUrl, firstRequestUrl, secondRequestUrl });
    cache.enqueueMissingWindowLoads(
        indexedImageUrl(9), archiveDocument, KiriView::PredecodeActiveLoads {});
    cache.cacheImage(cachedQueuedUrl, archiveDocument, staticTestImagePayload(cacheImage()));

    const std::optional<KiriView::PredecodeRequest> firstRequest
        = cache.takeNextRequest(KiriView::PredecodeActiveLoads {});
    QVERIFY(firstRequest.has_value());
    QCOMPARE(firstRequest->url, firstRequestUrl);

    const std::optional<KiriView::PredecodeRequest> secondRequest
        = cache.takeNextRequest(KiriView::PredecodeActiveLoads {});
    QVERIFY(secondRequest.has_value());
    QCOMPARE(secondRequest->url, secondRequestUrl);
    QVERIFY(!cache.takeNextRequest(KiriView::PredecodeActiveLoads {}).has_value());
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

void TestPredecodeCache::cacheRetainsDisplayedImagesBeforeAdjacentImages()
{
    KiriView::PredecodeCache cache(160);
    const QUrl primaryDisplayedUrl = indexedImageUrl(0);
    const QUrl secondaryDisplayedUrl = indexedImageUrl(1);
    const QUrl adjacentUrl = indexedImageUrl(2);
    const KiriView::ArchiveDocumentLocation archiveDocument = comicBookArchiveDocument();
    const QImage image = cacheImage();

    cache.setDisplayedUrls({ primaryDisplayedUrl, secondaryDisplayedUrl });
    cache.setWindowUrls({ primaryDisplayedUrl, secondaryDisplayedUrl, adjacentUrl });
    cache.cacheDisplayedImage(
        true, secondaryDisplayedUrl, archiveDocument, staticTestImagePayload(image));
    cache.cacheImage(adjacentUrl, archiveDocument, staticTestImagePayload(image));
    cache.cacheDisplayedImage(
        true, primaryDisplayedUrl, archiveDocument, staticTestImagePayload(image));

    QVERIFY(cache.hasImage(primaryDisplayedUrl));
    QVERIFY(cache.hasImage(secondaryDisplayedUrl));
    QVERIFY(!cache.hasImage(adjacentUrl));
}

void TestPredecodeCache::cacheRetainsRecentDisplayedImagesBeforeAdjacentImages()
{
    KiriView::PredecodeCache cache(160);
    const QUrl recentDisplayedUrl = indexedImageUrl(0);
    const QUrl currentDisplayedUrl = indexedImageUrl(1);
    const QUrl adjacentUrl = indexedImageUrl(2);
    const KiriView::ArchiveDocumentLocation archiveDocument = comicBookArchiveDocument();
    const QImage image = cacheImage();

    cache.setDisplayedUrls({ recentDisplayedUrl });
    cache.cacheDisplayedImage(
        true, recentDisplayedUrl, archiveDocument, staticTestImagePayload(image));
    cache.setDisplayedUrls({ currentDisplayedUrl });
    cache.setWindowUrls({ currentDisplayedUrl, adjacentUrl });
    cache.cacheImage(adjacentUrl, archiveDocument, staticTestImagePayload(image));
    cache.cacheDisplayedImage(
        true, currentDisplayedUrl, archiveDocument, staticTestImagePayload(image));

    QVERIFY(cache.hasImage(currentDisplayedUrl));
    QVERIFY(cache.hasImage(recentDisplayedUrl));
    QVERIFY(!cache.hasImage(adjacentUrl));
}

void TestPredecodeCache::cacheKeepsOnlyFourRecentDisplayedImages()
{
    KiriView::PredecodeCache cache(1000);
    const KiriView::ArchiveDocumentLocation archiveDocument = comicBookArchiveDocument();
    const QImage image = cacheImage();

    for (int index = 0; index < 6; ++index) {
        const QUrl url = indexedImageUrl(index);
        cache.setDisplayedUrls({ url });
        cache.cacheDisplayedImage(true, url, archiveDocument, staticTestImagePayload(image));
    }

    QVERIFY(cache.hasImage(indexedImageUrl(5)));
    QVERIFY(cache.hasImage(indexedImageUrl(4)));
    QVERIFY(cache.hasImage(indexedImageUrl(3)));
    QVERIFY(cache.hasImage(indexedImageUrl(2)));
    QVERIFY(cache.hasImage(indexedImageUrl(1)));
    QVERIFY(!cache.hasImage(indexedImageUrl(0)));
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
