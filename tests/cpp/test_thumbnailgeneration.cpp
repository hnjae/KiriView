// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "thumbnail/thumbnailgeneration.h"
#include "thumbnail/videothumbnailextractor.h"

#include <QBuffer>
#include <QColor>
#include <QImage>
#include <QImageWriter>
#include <QObject>
#include <QSize>
#include <QString>
#include <QTest>
#include <QUrl>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
using Status = kiriview::ThumbnailGenerationStatus;

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

kiriview::ThumbnailGenerationRequest generationRequest(Bucket bucket = Bucket::Normal)
{
    kiriview::ThumbnailGenerationRequest request;
    request.localPathBytes = QByteArrayLiteral("/missing/source.png");
    request.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/missing/source.png"));
    request.sourceLabel = QStringLiteral("source.png");
    request.requestedBucket = bucket;
    request.cacheInstallEnabled = false;
    return request;
}

struct ManualVideoExtraction {
    QObject *object = nullptr;
    kiriview::VideoThumbnailExtractionRequest request;
    kiriview::VideoThumbnailExtractionCallback callback;
    kiriview::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualVideoExtractionProvider
{
public:
    kiriview::VideoThumbnailExtractionProvider provider()
    {
        return [this](QObject *receiver, kiriview::VideoThumbnailExtractionRequest request,
                   kiriview::VideoThumbnailExtractionCallback callback) {
            auto extraction = std::make_shared<ManualVideoExtraction>();
            extraction->object = new QObject(receiver);
            extraction->request = std::move(request);
            extraction->callback = std::move(callback);
            std::weak_ptr<ManualVideoExtraction> weakExtraction = extraction;
            kiriview::ImageIoJob job(extraction->object, [weakExtraction](QObject *object) {
                if (std::shared_ptr<ManualVideoExtraction> extraction = weakExtraction.lock()) {
                    extraction->canceled = true;
                    extraction->object = nullptr;
                }
                if (object != nullptr) {
                    object->deleteLater();
                }
            });
            extraction->completion = job.completion();
            m_extractions.push_back(std::move(extraction));
            return job;
        };
    }

    std::size_t extractionCount() const { return m_extractions.size(); }

    ManualVideoExtraction &extractionAt(std::size_t index) { return *m_extractions.at(index); }

    void finish(std::size_t index, kiriview::VideoThumbnailExtractionResult result)
    {
        std::shared_ptr<ManualVideoExtraction> extraction = m_extractions.at(index);
        QObject *object = extraction->object;
        extraction->completion.claimAndRun(
            [extraction, object, result = std::move(result)]() mutable {
                extraction->object = nullptr;
                if (extraction->callback) {
                    extraction->callback(std::move(result));
                }
                if (object != nullptr) {
                    object->deleteLater();
                }
            });
    }

private:
    std::vector<std::shared_ptr<ManualVideoExtraction>> m_extractions;
};
}

class TestThumbnailGeneration : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void injectedBytesLoaderProvidesGenerationBytes();
    void injectedDecoderReceivesLoadedBytesAndBucketEdge();
    void injectedScalingPolicySuppliesDecoderEdge();
    void openedCollectionIdentityFailureSkipsBytesLoader();
    void injectedCacheHitSkipsBytesLoader();
    void injectedCacheInstallPublishesInstalledPath();
    void directVideoProviderUsesExtractorAndInstallsCache();
    void directVideoExtractorFailurePublishesFailure();
};

void TestThumbnailGeneration::injectedBytesLoaderProvidesGenerationBytes()
{
    kiriview::ThumbnailGenerationDependencies dependencies;
    int bytesLoadCount = 0;
    dependencies.bytesLoader
        = [&bytesLoadCount](const kiriview::ThumbnailGenerationRequest &, QString *) {
              ++bytesLoadCount;
              return encodedPngData();
          };

    const kiriview::ThumbnailGenerationResult result
        = kiriview::generateThumbnail(generationRequest(), std::move(dependencies));

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

    kiriview::ThumbnailGenerationDependencies dependencies;
    dependencies.bytesLoader
        = [&bytes](const kiriview::ThumbnailGenerationRequest &, QString *) { return bytes; };
    dependencies.imageDecoder = [&decodedBytes, &decodedMaximumLongEdge](
                                    QByteArray data, int maximumLongEdge, QString *) {
        decodedBytes = std::move(data);
        decodedMaximumLongEdge = maximumLongEdge;
        QImage image(QSize(9, 7), QImage::Format_RGB32);
        image.fill(QColor(Qt::yellow));
        return image;
    };

    const kiriview::ThumbnailGenerationResult result
        = kiriview::generateThumbnail(generationRequest(Bucket::Large), std::move(dependencies));

    QCOMPARE(decodedBytes, bytes);
    QCOMPARE(decodedMaximumLongEdge, 256);
    QCOMPARE(result.status, Status::Ready);
    QCOMPARE(result.requestedBucket, Bucket::Large);
    QCOMPARE(result.image.size(), QSize(9, 7));
    QCOMPARE(result.image.format(), QImage::Format_RGBA8888);
}

void TestThumbnailGeneration::injectedScalingPolicySuppliesDecoderEdge()
{
    const QByteArray bytes("synthetic bytes");
    Bucket policyBucket = Bucket::None;
    int decodedMaximumLongEdge = 0;

    kiriview::ThumbnailGenerationDependencies dependencies;
    dependencies.bytesLoader
        = [&bytes](const kiriview::ThumbnailGenerationRequest &, QString *) { return bytes; };
    dependencies.maximumLongEdgeForBucket = [&policyBucket](Bucket bucket) {
        policyBucket = bucket;
        return 777;
    };
    dependencies.imageDecoder
        = [&decodedMaximumLongEdge](QByteArray, int maximumLongEdge, QString *) {
              decodedMaximumLongEdge = maximumLongEdge;
              QImage image(QSize(6, 5), QImage::Format_RGB32);
              image.fill(QColor(Qt::cyan));
              return image;
          };

    const kiriview::ThumbnailGenerationResult result
        = kiriview::generateThumbnail(generationRequest(Bucket::XLarge), std::move(dependencies));

    QCOMPARE(policyBucket, Bucket::XLarge);
    QCOMPARE(decodedMaximumLongEdge, 777);
    QCOMPARE(result.status, Status::Ready);
    QCOMPARE(result.requestedBucket, Bucket::XLarge);
}

void TestThumbnailGeneration::openedCollectionIdentityFailureSkipsBytesLoader()
{
    kiriview::ThumbnailGenerationRequest request = generationRequest();
    request.openedCollectionScope = kiriview::OpenedCollectionScopeLocation::fromUrls(
        QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")),
        QUrl(QStringLiteral("zip:///books/book.cbz")),
        kiriview::OpenedCollectionScopeKind::ComicBookArchive);
    request.sourceUrl = QUrl(QStringLiteral("zip:///books/book.cbz/page.png"));
    request.cacheInstallEnabled = true;

    kiriview::ThumbnailGenerationDependencies dependencies;
    int bytesLoadCount = 0;
    dependencies.bytesLoader
        = [&bytesLoadCount](const kiriview::ThumbnailGenerationRequest &, QString *) {
              ++bytesLoadCount;
              return encodedPngData();
          };
    dependencies.openedCollectionOriginalIdentityLoader
        = [](const kiriview::ThumbnailGenerationRequest &, QString *errorString) {
              if (errorString != nullptr) {
                  *errorString = QStringLiteral("synthetic identity failure");
              }
              return std::optional<kiriview::ThumbnailOriginalIdentity>();
          };

    const kiriview::ThumbnailGenerationResult result
        = kiriview::generateThumbnail(std::move(request), std::move(dependencies));

    QCOMPARE(bytesLoadCount, 0);
    QCOMPARE(result.status, Status::Failed);
    QCOMPARE(result.requestedBucket, Bucket::Normal);
    QCOMPARE(result.errorString, QStringLiteral("synthetic identity failure"));
}

void TestThumbnailGeneration::injectedCacheHitSkipsBytesLoader()
{
    kiriview::ThumbnailGenerationRequest request = generationRequest();
    request.openedCollectionScope = kiriview::OpenedCollectionScopeLocation::fromUrls(
        QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")),
        QUrl(QStringLiteral("zip:///books/book.cbz")),
        kiriview::OpenedCollectionScopeKind::ComicBookArchive);
    request.sourceUrl = QUrl(QStringLiteral("zip:///books/book.cbz/page.png"));
    request.cacheInstallEnabled = true;

    QImage cachedImage(QSize(16, 12), QImage::Format_RGBA8888);
    cachedImage.fill(QColor(Qt::blue));

    kiriview::ThumbnailGenerationDependencies dependencies;
    int bytesLoadCount = 0;
    dependencies.bytesLoader
        = [&bytesLoadCount](const kiriview::ThumbnailGenerationRequest &, QString *) {
              ++bytesLoadCount;
              return encodedPngData();
          };
    dependencies.openedCollectionOriginalIdentityLoader
        = [](const kiriview::ThumbnailGenerationRequest &, QString *) {
              return std::optional<kiriview::ThumbnailOriginalIdentity>(
                  kiriview::ThumbnailOriginalIdentity::fromNonFileUri(
                      QStringLiteral("x-kiriview://thumbnail/test"), 0, 8, QString()));
          };
    dependencies.cacheRepository.lookup
        = [cachedImage](const kiriview::ThumbnailOriginalIdentity &, Bucket bucket) {
              return std::optional<kiriview::ThumbnailCacheLookupResult>(
                  kiriview::ThumbnailCacheLookupResult {
                      kiriview::ThumbnailCacheLookupStatus::Ready,
                      cachedImage,
                      bucket,
                      bucket,
                      QStringLiteral("/cache/hit.png"),
                      {},
                  });
          };

    const kiriview::ThumbnailGenerationResult result
        = kiriview::generateThumbnail(std::move(request), std::move(dependencies));

    QCOMPARE(bytesLoadCount, 0);
    QCOMPARE(result.status, Status::Ready);
    QCOMPARE(result.requestedBucket, Bucket::Normal);
    QCOMPARE(result.image.size(), cachedImage.size());
    QCOMPARE(result.installedCachePath, QStringLiteral("/cache/hit.png"));
}

void TestThumbnailGeneration::injectedCacheInstallPublishesInstalledPath()
{
    kiriview::ThumbnailGenerationRequest request = generationRequest(Bucket::Large);
    request.cacheInstallEnabled = true;

    kiriview::ThumbnailOriginalIdentity installedIdentity;
    QImage installedImage;

    kiriview::ThumbnailGenerationDependencies dependencies;
    dependencies.bytesLoader
        = [](const kiriview::ThumbnailGenerationRequest &, QString *) { return encodedPngData(); };
    dependencies.cacheRepository.install
        = [&installedIdentity, &installedImage](const kiriview::ThumbnailOriginalIdentity &identity,
              Bucket bucket, const QImage &image) {
              installedIdentity = identity;
              installedImage = image.copy();
              return kiriview::ThumbnailGenerationCacheInstallResult {
                  true,
                  bucket,
                  QStringLiteral("/cache/generated.png"),
                  {},
              };
          };

    const kiriview::ThumbnailGenerationResult result
        = kiriview::generateThumbnail(std::move(request), std::move(dependencies));

    QCOMPARE(result.status, Status::Ready);
    QCOMPARE(result.requestedBucket, Bucket::Large);
    QCOMPARE(result.installedCachePath, QStringLiteral("/cache/generated.png"));
    QVERIFY(installedIdentity.isLocalPath());
    QCOMPARE(installedIdentity.localPathBytes, QByteArrayLiteral("/missing/source.png"));
    QCOMPARE(installedImage.size(), QSize(4, 3));
    QCOMPARE(installedImage.format(), QImage::Format_RGBA8888);
}

void TestThumbnailGeneration::directVideoProviderUsesExtractorAndInstallsCache()
{
    QObject owner;
    kiriview::ThumbnailGenerationRequest request = generationRequest(Bucket::Large);
    request.localPathBytes = QByteArrayLiteral("/media/clip.mp4");
    request.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/clip.mp4"));
    request.sourceLabel = QStringLiteral("clip.mp4");
    request.sourceKind = kiriview::ThumbnailSourceKind::DirectVideo;
    request.cacheInstallEnabled = true;

    ManualVideoExtractionProvider extractionProvider;
    int bytesLoadCount = 0;
    kiriview::ThumbnailOriginalIdentity installedIdentity;
    QImage installedImage;
    kiriview::ThumbnailGenerationDependencies dependencies;
    dependencies.bytesLoader
        = [&bytesLoadCount](const kiriview::ThumbnailGenerationRequest &, QString *) {
              ++bytesLoadCount;
              return encodedPngData();
          };
    dependencies.videoExtractor = extractionProvider.provider();
    dependencies.cacheRepository.install
        = [&installedIdentity, &installedImage](const kiriview::ThumbnailOriginalIdentity &identity,
              Bucket bucket, const QImage &image) {
              installedIdentity = identity;
              installedImage = image.copy();
              return kiriview::ThumbnailGenerationCacheInstallResult {
                  true,
                  bucket,
                  QStringLiteral("/cache/video.png"),
                  {},
              };
          };

    kiriview::ThumbnailGenerationResult delivered;
    bool deliveredResult = false;
    kiriview::ThumbnailGenerationProvider provider
        = kiriview::defaultThumbnailGenerationProvider({}, std::move(dependencies));
    kiriview::ImageIoJob job = provider(&owner, std::move(request),
        [&delivered, &deliveredResult](kiriview::ThumbnailGenerationResult result) {
            delivered = std::move(result);
            deliveredResult = true;
        });

    QCOMPARE(bytesLoadCount, 0);
    QCOMPARE(extractionProvider.extractionCount(), std::size_t(1));
    QCOMPARE(extractionProvider.extractionAt(0).request.localPathBytes,
        QByteArrayLiteral("/media/clip.mp4"));
    QCOMPARE(extractionProvider.extractionAt(0).request.sourceUrl,
        QUrl::fromLocalFile(QStringLiteral("/media/clip.mp4")));
    QCOMPARE(extractionProvider.extractionAt(0).request.requestedBucket, Bucket::Large);
    QCOMPARE(extractionProvider.extractionAt(0).request.maximumLongEdge, 256);
    QVERIFY(job.isActive());

    QImage frame(QSize(12, 8), QImage::Format_RGB32);
    frame.fill(QColor(Qt::red));
    extractionProvider.finish(0,
        kiriview::VideoThumbnailExtractionResult {
            kiriview::ThumbnailGenerationStatus::Ready,
            frame,
            {},
        });

    QVERIFY(deliveredResult);
    QCOMPARE(delivered.status, Status::Ready);
    QCOMPARE(delivered.requestedBucket, Bucket::Large);
    QCOMPARE(delivered.installedCachePath, QStringLiteral("/cache/video.png"));
    QVERIFY(installedIdentity.isLocalPath());
    QCOMPARE(installedIdentity.localPathBytes, QByteArrayLiteral("/media/clip.mp4"));
    QCOMPARE(installedImage.size(), QSize(12, 8));
    QCOMPARE(installedImage.format(), QImage::Format_RGBA8888);
}

void TestThumbnailGeneration::directVideoExtractorFailurePublishesFailure()
{
    QObject owner;
    kiriview::ThumbnailGenerationRequest request = generationRequest(Bucket::Normal);
    request.localPathBytes = QByteArrayLiteral("/media/clip.mp4");
    request.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/clip.mp4"));
    request.sourceKind = kiriview::ThumbnailSourceKind::DirectVideo;
    request.cacheInstallEnabled = true;

    ManualVideoExtractionProvider extractionProvider;
    int cacheInstallCount = 0;
    kiriview::ThumbnailGenerationDependencies dependencies;
    dependencies.videoExtractor = extractionProvider.provider();
    dependencies.cacheRepository.install =
        [&cacheInstallCount](const kiriview::ThumbnailOriginalIdentity &, Bucket, const QImage &) {
            ++cacheInstallCount;
            return kiriview::ThumbnailGenerationCacheInstallResult {};
        };

    kiriview::ThumbnailGenerationResult delivered;
    bool deliveredResult = false;
    kiriview::ThumbnailGenerationProvider provider
        = kiriview::defaultThumbnailGenerationProvider({}, std::move(dependencies));
    kiriview::ImageIoJob job = provider(&owner, std::move(request),
        [&delivered, &deliveredResult](kiriview::ThumbnailGenerationResult result) {
            delivered = std::move(result);
            deliveredResult = true;
        });

    QCOMPARE(extractionProvider.extractionCount(), std::size_t(1));
    extractionProvider.finish(0,
        kiriview::VideoThumbnailExtractionResult {
            kiriview::ThumbnailGenerationStatus::Failed,
            {},
            QStringLiteral("synthetic extractor failure"),
        });

    QVERIFY(deliveredResult);
    QCOMPARE(delivered.status, Status::Failed);
    QCOMPARE(delivered.requestedBucket, Bucket::Normal);
    QCOMPARE(delivered.errorString, QStringLiteral("synthetic extractor failure"));
    QCOMPARE(cacheInstallCount, 0);
}

QTEST_GUILESS_MAIN(TestThumbnailGeneration)

#include "test_thumbnailgeneration.moc"
