// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "cache/imagecachepolicy.h"
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
using KiriView::TestSupport::staticDisplayTestImagePayload;

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

KiriView::OpenedCollectionScopeLocation comicBookArchiveCollection()
{
    return KiriView::OpenedCollectionScopeLocation::fromUrls(
        QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")),
        QUrl(QStringLiteral("zip:///books/book.cbz/")),
        KiriView::OpenedCollectionScopeKind::ComicBookArchive);
}

KiriView::PredecodeActiveLoads activeLoads(std::vector<QUrl> urls)
{
    return KiriView::PredecodeActiveLoads::fromUrls(std::move(urls));
}

KiriView::StaticDisplayImagePayload cacheDisplayImage(
    const QImage &image, qreal firstDisplayPixelsPerSourcePixel = 0.0)
{
    const KiriView::DisplayImageQuality quality = firstDisplayPixelsPerSourcePixel > 0.0
        ? KiriView::DisplayImageQuality::FirstDisplay
        : KiriView::DisplayImageQuality::Exact;
    return staticDisplayTestImagePayload(image, image, firstDisplayPixelsPerSourcePixel, quality);
}

KiriView::PredecodeCache defaultCache()
{
    return KiriView::PredecodeCache(KiriView::predecodeCachePreferredByteBudget());
}
}

class TestPredecodeCache : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void queueContainsOnlyMissingWindowImages();
    void queueSkipsAllDisplayedWindowImages();
    void takeNextRequestDiscardsSkippedQueuePrefix();
    void cacheEligibilityUsesByteBudgetPolicy();
    void cacheStoresAndFindsWindowImages();
    void cacheFindsImagesByUrlAndOpenedCollectionScope();
    void cacheRetainsDisplayedImagesBeforeAdjacentImages();
    void cacheRetainsRecentDisplayedImagesBeforeAdjacentImages();
    void cacheKeepsOnlyFourRecentDisplayedImages();
    void cacheRejectsUncacheableAndOversizedImages();
    void cacheEvictsLowestPriorityImagesWhenBudgetIsExceeded();
};

void TestPredecodeCache::queueContainsOnlyMissingWindowImages()
{
    KiriView::PredecodeCache cache = defaultCache();
    const QUrl displayedUrl = indexedImageUrl(0);
    const QUrl firstQueuedUrl = indexedImageUrl(1);
    const QUrl secondQueuedUrl = indexedImageUrl(2);

    cache.setWindowUrls({ displayedUrl, firstQueuedUrl, firstQueuedUrl, QUrl(), secondQueuedUrl });
    const KiriView::OpenedCollectionScopeLocation openedCollectionScope
        = comicBookArchiveCollection();
    const QImage firstImage = cacheImage();
    cache.cacheImage(firstQueuedUrl, openedCollectionScope, cacheDisplayImage(firstImage));
    cache.enqueueMissingWindowLoads(
        displayedUrl, openedCollectionScope, KiriView::PredecodeActiveLoads {});

    QVERIFY(cache.isInFlight(secondQueuedUrl, KiriView::PredecodeActiveLoads {}));

    const std::optional<KiriView::PredecodeRequest> request
        = cache.takeNextRequest(activeLoads({ indexedImageUrl(9) }));
    QVERIFY(request.has_value());
    QCOMPARE(request->url, secondQueuedUrl);
    QCOMPARE(request->openedCollectionScope.rootUrl(), openedCollectionScope.rootUrl());
    QVERIFY(!cache.takeNextRequest(KiriView::PredecodeActiveLoads {}).has_value());
}

void TestPredecodeCache::queueSkipsAllDisplayedWindowImages()
{
    KiriView::PredecodeCache cache = defaultCache();
    const QUrl primaryDisplayedUrl = indexedImageUrl(0);
    const QUrl secondaryDisplayedUrl = indexedImageUrl(1);
    const QUrl queuedUrl = indexedImageUrl(2);
    const KiriView::OpenedCollectionScopeLocation openedCollectionScope
        = comicBookArchiveCollection();

    cache.setDisplayedUrls({ primaryDisplayedUrl, secondaryDisplayedUrl });
    cache.setWindowUrls({ primaryDisplayedUrl, secondaryDisplayedUrl, queuedUrl });
    cache.enqueueMissingWindowLoads(
        primaryDisplayedUrl, openedCollectionScope, KiriView::PredecodeActiveLoads {});

    const std::optional<KiriView::PredecodeRequest> request
        = cache.takeNextRequest(KiriView::PredecodeActiveLoads {});
    QVERIFY(request.has_value());
    QCOMPARE(request->url, queuedUrl);
    QVERIFY(!cache.takeNextRequest(KiriView::PredecodeActiveLoads {}).has_value());
}

void TestPredecodeCache::takeNextRequestDiscardsSkippedQueuePrefix()
{
    KiriView::PredecodeCache cache = defaultCache();
    const QUrl cachedQueuedUrl = indexedImageUrl(0);
    const QUrl firstRequestUrl = indexedImageUrl(1);
    const QUrl secondRequestUrl = indexedImageUrl(2);
    const KiriView::OpenedCollectionScopeLocation openedCollectionScope
        = comicBookArchiveCollection();

    cache.setWindowUrls({ cachedQueuedUrl, firstRequestUrl, secondRequestUrl });
    cache.enqueueMissingWindowLoads(
        indexedImageUrl(9), openedCollectionScope, KiriView::PredecodeActiveLoads {});
    cache.cacheImage(cachedQueuedUrl, openedCollectionScope, cacheDisplayImage(cacheImage()));

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

void TestPredecodeCache::cacheEligibilityUsesByteBudgetPolicy()
{
    const KiriView::StaticDisplayImagePayload image = cacheDisplayImage(cacheImage());
    const qsizetype byteCost = image.byteCost();

    QVERIFY(KiriView::PredecodeCache::canCacheImage(
        image, KiriView::predecodeCachePreferredByteBudget()));
    QVERIFY(KiriView::PredecodeCache::canCacheImage(image, byteCost));
    QVERIFY(!KiriView::PredecodeCache::canCacheImage(image, byteCost - 1));
    QVERIFY(
        !KiriView::PredecodeCache::canCacheImage(KiriView::StaticDisplayImagePayload {}, byteCost));
    QVERIFY(!KiriView::PredecodeCache::canCacheImage(image, 0));
}

void TestPredecodeCache::cacheStoresAndFindsWindowImages()
{
    KiriView::PredecodeCache cache(80);
    const QUrl url = indexedImageUrl(0);
    const KiriView::OpenedCollectionScopeLocation openedCollectionScope
        = comicBookArchiveCollection();
    const QImage image = cacheImage();

    cache.setWindowUrls({ url });
    KiriView::StaticDisplayImagePayload payload = cacheDisplayImage(image, 0.5);
    payload.sourceIdentity = QStringLiteral("file:///tmp/predecode-source.jpg");
    payload.imageReaderTransform.transformations = QImageIOHandler::TransformationRotate90;
    payload.embeddedMetadata.cameraMake = QStringLiteral("Kiri Camera");
    cache.cacheImage(url, openedCollectionScope, std::move(payload));

    const std::optional<KiriView::PredecodedImage> found = cache.findImage(url);
    QVERIFY(found.has_value());
    QCOMPARE(found->displayImage.image.size(), image.size());
    QCOMPARE(found->displayImage.originalSize, image.size());
    QCOMPARE(
        found->displayImage.sourceIdentity, QStringLiteral("file:///tmp/predecode-source.jpg"));
    QCOMPARE(found->displayImage.quality, KiriView::DisplayImageQuality::FirstDisplay);
    QCOMPARE(found->displayImage.displayPixelsPerSourcePixel, 0.5);
    QCOMPARE(found->displayImage.imageReaderTransform.transformations,
        QImageIOHandler::TransformationRotate90);
    QCOMPARE(found->displayImage.embeddedMetadata.cameraMake, QStringLiteral("Kiri Camera"));
    QCOMPARE(found->embeddedMetadata.cameraMake, QStringLiteral("Kiri Camera"));
    QCOMPARE(found->location.imageUrl(), url);
    QCOMPARE(found->location.openedCollectionScopeRootUrl(), openedCollectionScope.rootUrl());
    QVERIFY(found->displayImage.refinementSource != nullptr);
}

void TestPredecodeCache::cacheFindsImagesByUrlAndOpenedCollectionScope()
{
    KiriView::PredecodeCache cache(160);
    const QUrl url = indexedImageUrl(0);
    const KiriView::OpenedCollectionScopeLocation openedCollectionScope
        = comicBookArchiveCollection();
    const KiriView::DisplayedImageLocation directLocation
        = KiriView::DisplayedImageLocation::fromUrl(url);
    const KiriView::DisplayedImageLocation openedCollectionLocation
        = KiriView::DisplayedImageLocation::fromOpenedCollectionScope(url, openedCollectionScope);

    cache.setWindowUrls({ url });
    cache.cacheImage(url, KiriView::OpenedCollectionScopeLocation::none(),
        cacheDisplayImage(cacheImage(), 0.25));
    cache.cacheImage(url, openedCollectionScope, cacheDisplayImage(cacheImage(), 0.75));

    const std::optional<KiriView::PredecodedImage> direct = cache.findImage(directLocation);
    QVERIFY(direct.has_value());
    QVERIFY(direct->location == directLocation);
    QCOMPARE(direct->displayImage.displayPixelsPerSourcePixel, 0.25);

    const std::optional<KiriView::PredecodedImage> openedCollection
        = cache.findImage(openedCollectionLocation);
    QVERIFY(openedCollection.has_value());
    QVERIFY(openedCollection->location == openedCollectionLocation);
    QCOMPARE(openedCollection->displayImage.displayPixelsPerSourcePixel, 0.75);
}

void TestPredecodeCache::cacheRetainsDisplayedImagesBeforeAdjacentImages()
{
    KiriView::PredecodeCache cache(160);
    const QUrl primaryDisplayedUrl = indexedImageUrl(0);
    const QUrl secondaryDisplayedUrl = indexedImageUrl(1);
    const QUrl adjacentUrl = indexedImageUrl(2);
    const KiriView::OpenedCollectionScopeLocation openedCollectionScope
        = comicBookArchiveCollection();
    const QImage image = cacheImage();

    cache.setDisplayedUrls({ primaryDisplayedUrl, secondaryDisplayedUrl });
    cache.setWindowUrls({ primaryDisplayedUrl, secondaryDisplayedUrl, adjacentUrl });
    cache.cacheDisplayedImage(
        true, secondaryDisplayedUrl, openedCollectionScope, cacheDisplayImage(image));
    cache.cacheImage(adjacentUrl, openedCollectionScope, cacheDisplayImage(image));
    cache.cacheDisplayedImage(
        true, primaryDisplayedUrl, openedCollectionScope, cacheDisplayImage(image));

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
    const KiriView::OpenedCollectionScopeLocation openedCollectionScope
        = comicBookArchiveCollection();
    const QImage image = cacheImage();

    cache.setDisplayedUrls({ recentDisplayedUrl });
    cache.cacheDisplayedImage(
        true, recentDisplayedUrl, openedCollectionScope, cacheDisplayImage(image));
    cache.setDisplayedUrls({ currentDisplayedUrl });
    cache.setWindowUrls({ currentDisplayedUrl, adjacentUrl });
    cache.cacheImage(adjacentUrl, openedCollectionScope, cacheDisplayImage(image));
    cache.cacheDisplayedImage(
        true, currentDisplayedUrl, openedCollectionScope, cacheDisplayImage(image));

    QVERIFY(cache.hasImage(currentDisplayedUrl));
    QVERIFY(cache.hasImage(recentDisplayedUrl));
    QVERIFY(!cache.hasImage(adjacentUrl));
}

void TestPredecodeCache::cacheKeepsOnlyFourRecentDisplayedImages()
{
    KiriView::PredecodeCache cache(1000);
    const KiriView::OpenedCollectionScopeLocation openedCollectionScope
        = comicBookArchiveCollection();
    const QImage image = cacheImage();

    for (int index = 0; index < 6; ++index) {
        const QUrl url = indexedImageUrl(index);
        cache.setDisplayedUrls({ url });
        cache.cacheDisplayedImage(true, url, openedCollectionScope, cacheDisplayImage(image));
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
    const KiriView::OpenedCollectionScopeLocation openedCollectionScope
        = comicBookArchiveCollection();

    cache.setWindowUrls({ url });
    const QImage image = cacheImage();
    cache.cacheDisplayedImage(false, url, openedCollectionScope, cacheDisplayImage(image));
    QVERIFY(!cache.hasImage(url));

    cache.cacheDisplayedImage(
        true, url, openedCollectionScope, KiriView::StaticDisplayImagePayload {});
    QVERIFY(!cache.hasImage(url));

    cache.cacheImage(outsideWindowUrl, openedCollectionScope, cacheDisplayImage(image));
    QVERIFY(!cache.hasImage(outsideWindowUrl));

    const QImage largeImage = tooLargeImage();
    cache.cacheImage(url, openedCollectionScope, cacheDisplayImage(largeImage));
    QVERIFY(!cache.hasImage(url));
}

void TestPredecodeCache::cacheEvictsLowestPriorityImagesWhenBudgetIsExceeded()
{
    KiriView::PredecodeCache cache(160);
    const QUrl firstUrl = indexedImageUrl(0);
    const QUrl secondUrl = indexedImageUrl(1);
    const QUrl thirdUrl = indexedImageUrl(2);
    const KiriView::OpenedCollectionScopeLocation openedCollectionScope
        = comicBookArchiveCollection();

    cache.setWindowUrls({ firstUrl, secondUrl, thirdUrl });
    const QImage image = cacheImage();
    cache.cacheImage(thirdUrl, openedCollectionScope, cacheDisplayImage(image));
    cache.cacheImage(firstUrl, openedCollectionScope, cacheDisplayImage(image));
    cache.cacheImage(secondUrl, openedCollectionScope, cacheDisplayImage(image));

    QVERIFY(cache.hasImage(firstUrl));
    QVERIFY(cache.hasImage(secondUrl));
    QVERIFY(!cache.hasImage(thirdUrl));
}

QTEST_GUILESS_MAIN(TestPredecodeCache)

#include "test_predecodecache.moc"
