// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "thumbnail/thumbnailgeneration.h"

#include <QBuffer>
#include <QColor>
#include <QImage>
#include <QImageWriter>
#include <QObject>
#include <QSize>
#include <QString>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
using Status = KiriView::ThumbnailGenerationStatus;

QByteArray encodedPngData()
{
    QImage image(QSize(4, 3), QImage::Format_RGBA8888);
    image.fill(QColor(Qt::green));

    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    QImageWriter writer(&buffer, QByteArrayLiteral("png"));
    if (!writer.write(image)) {
        return {};
    }
    return data;
}

KiriView::ThumbnailGenerationRequest generationRequest(Bucket bucket = Bucket::Normal)
{
    KiriView::ThumbnailGenerationRequest request;
    request.localPathBytes = QByteArrayLiteral("/missing/source.png");
    request.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/missing/source.png"));
    request.sourceLabel = QStringLiteral("source.png");
    request.requestedBucket = bucket;
    request.cacheInstallEnabled = false;
    return request;
}
}

class TestThumbnailGeneration : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void injectedBytesLoaderProvidesGenerationBytes();
    void injectedDecoderReceivesLoadedBytesAndBucketEdge();
    void openedCollectionIdentityFailureSkipsBytesLoader();
    void injectedCacheHitSkipsBytesLoader();
    void injectedCacheInstallPublishesInstalledPath();
};

void TestThumbnailGeneration::injectedBytesLoaderProvidesGenerationBytes()
{
    KiriView::ThumbnailGenerationDependencies dependencies;
    int bytesLoadCount = 0;
    dependencies.bytesLoader
        = [&bytesLoadCount](const KiriView::ThumbnailGenerationRequest &, QString *) {
              ++bytesLoadCount;
              return encodedPngData();
          };

    const KiriView::ThumbnailGenerationResult result
        = KiriView::generateThumbnail(generationRequest(), std::move(dependencies));

    QCOMPARE(bytesLoadCount, 1);
    QCOMPARE(result.status, Status::Ready);
    QCOMPARE(result.requestedBucket, Bucket::Normal);
    QCOMPARE(result.image.size(), QSize(4, 3));
    QCOMPARE(result.image.format(), QImage::Format_RGBA8888);
    QVERIFY(result.installedCachePath.isEmpty());
}

void TestThumbnailGeneration::injectedDecoderReceivesLoadedBytesAndBucketEdge()
{
    const QByteArray bytes("synthetic bytes");
    QByteArray decodedBytes;
    int decodedMaximumLongEdge = 0;

    KiriView::ThumbnailGenerationDependencies dependencies;
    dependencies.bytesLoader
        = [&bytes](const KiriView::ThumbnailGenerationRequest &, QString *) { return bytes; };
    dependencies.imageDecoder = [&decodedBytes, &decodedMaximumLongEdge](
                                    QByteArray data, int maximumLongEdge, QString *) {
        decodedBytes = std::move(data);
        decodedMaximumLongEdge = maximumLongEdge;
        QImage image(QSize(9, 7), QImage::Format_RGB32);
        image.fill(QColor(Qt::yellow));
        return image;
    };

    const KiriView::ThumbnailGenerationResult result
        = KiriView::generateThumbnail(generationRequest(Bucket::Large), std::move(dependencies));

    QCOMPARE(decodedBytes, bytes);
    QCOMPARE(decodedMaximumLongEdge, 256);
    QCOMPARE(result.status, Status::Ready);
    QCOMPARE(result.requestedBucket, Bucket::Large);
    QCOMPARE(result.image.size(), QSize(9, 7));
    QCOMPARE(result.image.format(), QImage::Format_RGBA8888);
}

void TestThumbnailGeneration::openedCollectionIdentityFailureSkipsBytesLoader()
{
    KiriView::ThumbnailGenerationRequest request = generationRequest();
    request.openedCollectionScope = KiriView::OpenedCollectionScopeLocation::fromUrls(
        QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")),
        QUrl(QStringLiteral("zip:///books/book.cbz")),
        KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    request.sourceUrl = QUrl(QStringLiteral("zip:///books/book.cbz/page.png"));
    request.cacheInstallEnabled = true;

    KiriView::ThumbnailGenerationDependencies dependencies;
    int bytesLoadCount = 0;
    dependencies.bytesLoader
        = [&bytesLoadCount](const KiriView::ThumbnailGenerationRequest &, QString *) {
              ++bytesLoadCount;
              return encodedPngData();
          };
    dependencies.openedCollectionOriginalIdentityLoader
        = [](const KiriView::ThumbnailGenerationRequest &, QString *errorString) {
              if (errorString != nullptr) {
                  *errorString = QStringLiteral("synthetic identity failure");
              }
              return std::optional<KiriView::ThumbnailOriginalIdentity>();
          };

    const KiriView::ThumbnailGenerationResult result
        = KiriView::generateThumbnail(std::move(request), std::move(dependencies));

    QCOMPARE(bytesLoadCount, 0);
    QCOMPARE(result.status, Status::Failed);
    QCOMPARE(result.requestedBucket, Bucket::Normal);
    QCOMPARE(result.errorString, QStringLiteral("synthetic identity failure"));
}

void TestThumbnailGeneration::injectedCacheHitSkipsBytesLoader()
{
    KiriView::ThumbnailGenerationRequest request = generationRequest();
    request.openedCollectionScope = KiriView::OpenedCollectionScopeLocation::fromUrls(
        QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")),
        QUrl(QStringLiteral("zip:///books/book.cbz")),
        KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    request.sourceUrl = QUrl(QStringLiteral("zip:///books/book.cbz/page.png"));
    request.cacheInstallEnabled = true;

    QImage cachedImage(QSize(16, 12), QImage::Format_RGBA8888);
    cachedImage.fill(QColor(Qt::blue));

    KiriView::ThumbnailGenerationDependencies dependencies;
    int bytesLoadCount = 0;
    dependencies.bytesLoader
        = [&bytesLoadCount](const KiriView::ThumbnailGenerationRequest &, QString *) {
              ++bytesLoadCount;
              return encodedPngData();
          };
    dependencies.openedCollectionOriginalIdentityLoader
        = [](const KiriView::ThumbnailGenerationRequest &, QString *) {
              return std::optional<KiriView::ThumbnailOriginalIdentity>(
                  KiriView::ThumbnailOriginalIdentity::fromNonFileUri(
                      QStringLiteral("x-kiriview://thumbnail/test"), 0, 8, QString()));
          };
    dependencies.cacheRepository.lookup
        = [cachedImage](const KiriView::ThumbnailOriginalIdentity &, Bucket bucket) {
              return std::optional<KiriView::ThumbnailCacheLookupResult>(
                  KiriView::ThumbnailCacheLookupResult {
                      KiriView::ThumbnailCacheLookupStatus::Ready,
                      cachedImage,
                      bucket,
                      bucket,
                      QStringLiteral("/cache/hit.png"),
                      {},
                  });
          };

    const KiriView::ThumbnailGenerationResult result
        = KiriView::generateThumbnail(std::move(request), std::move(dependencies));

    QCOMPARE(bytesLoadCount, 0);
    QCOMPARE(result.status, Status::Ready);
    QCOMPARE(result.requestedBucket, Bucket::Normal);
    QCOMPARE(result.image.size(), cachedImage.size());
    QCOMPARE(result.installedCachePath, QStringLiteral("/cache/hit.png"));
}

void TestThumbnailGeneration::injectedCacheInstallPublishesInstalledPath()
{
    KiriView::ThumbnailGenerationRequest request = generationRequest(Bucket::Large);
    request.cacheInstallEnabled = true;

    KiriView::ThumbnailOriginalIdentity installedIdentity;
    QImage installedImage;

    KiriView::ThumbnailGenerationDependencies dependencies;
    dependencies.bytesLoader
        = [](const KiriView::ThumbnailGenerationRequest &, QString *) { return encodedPngData(); };
    dependencies.cacheRepository.install
        = [&installedIdentity, &installedImage](const KiriView::ThumbnailOriginalIdentity &identity,
              Bucket bucket, const QImage &image) {
              installedIdentity = identity;
              installedImage = image.copy();
              return KiriView::ThumbnailGenerationCacheInstallResult {
                  true,
                  bucket,
                  QStringLiteral("/cache/generated.png"),
                  {},
              };
          };

    const KiriView::ThumbnailGenerationResult result
        = KiriView::generateThumbnail(std::move(request), std::move(dependencies));

    QCOMPARE(result.status, Status::Ready);
    QCOMPARE(result.requestedBucket, Bucket::Large);
    QCOMPARE(result.installedCachePath, QStringLiteral("/cache/generated.png"));
    QVERIFY(installedIdentity.isLocalPath());
    QCOMPARE(installedIdentity.localPathBytes, QByteArrayLiteral("/missing/source.png"));
    QCOMPARE(installedImage.size(), QSize(4, 3));
    QCOMPARE(installedImage.format(), QImage::Format_RGBA8888);
}

QTEST_GUILESS_MAIN(TestThumbnailGeneration)

#include "test_thumbnailgeneration.moc"
