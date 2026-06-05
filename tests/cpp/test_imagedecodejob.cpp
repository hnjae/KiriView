// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/imagedecodejob.h"
#include "decoding/rawthumbnailpreview.h"
#include "image_test_support.h"
#include "session/thumbnailcachelookup.h"

#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QFile>
#include <QImage>
#include <QImageWriter>
#include <QObject>
#include <QSemaphore>
#include <QSize>
#include <QTest>
#include <QThreadPool>
#include <QUrl>
#include <atomic>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::TestSupport::imageDecodeDependenciesFor;
using KiriView::TestSupport::indexedImageUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticImageDataDecoderRejectingBadData;
using KiriView::TestSupport::testImageDecodeFailureString;

KiriView::ImageDecodeJob::Callbacks decodeJobCallbacks(
    KiriView::ImageDecodeJob::DecodedCallback decoded = {},
    KiriView::ImageDecodeJob::LoadErrorCallback loadError = {},
    KiriView::ImageDecodeJob::ThumbnailPreviewCallback thumbnailPreview = {})
{
    return KiriView::ImageDecodeJob::Callbacks {
        std::move(decoded),
        std::move(loadError),
        std::move(thumbnailPreview),
    };
}

QByteArray encodedPngData(const QSize &size)
{
    QImage image(size, QImage::Format_RGBA8888);
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

QImage thumbnailImage(const QSize &size = QSize(400, 300))
{
    QImage image(size, QImage::Format_RGBA8888);
    image.fill(QColor(Qt::blue));
    return image;
}

KiriView::ThumbnailCacheLookupResult readyThumbnailLookup(QImage image = thumbnailImage())
{
    return KiriView::ThumbnailCacheLookupResult {
        KiriView::ThumbnailCacheLookupStatus::Ready,
        std::move(image),
        KiriView::ActiveNavigationThumbnailDemandBucket::XXLarge,
        KiriView::ActiveNavigationThumbnailDemandBucket::XXLarge,
        QStringLiteral("/cache/photo.png"),
        {},
    };
}

KiriView::ThumbnailCacheLookupResult missingThumbnailLookup()
{
    return KiriView::ThumbnailCacheLookupResult {
        KiriView::ThumbnailCacheLookupStatus::Missing,
        {},
        KiriView::ActiveNavigationThumbnailDemandBucket::XXLarge,
        KiriView::ActiveNavigationThumbnailDemandBucket::None,
        {},
        {},
    };
}

QByteArray rawFixtureData()
{
    QFile file(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../fixtures/raw-cfa-smoke.dng"));
    if (!file.open(QFile::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

class ManualThumbnailLookupProvider
{
public:
    KiriView::ThumbnailCacheLookupProvider provider()
    {
        return [this](QObject *, KiriView::ThumbnailCacheLookupRequest request,
                   KiriView::ThumbnailCacheLookupCallback callback) {
            requests.push_back(std::move(request));
            callbacks.push_back(std::move(callback));
            return KiriView::ImageIoJob();
        };
    }

    void finish(std::size_t index, KiriView::ThumbnailCacheLookupResult result)
    {
        if (index >= callbacks.size() || !callbacks.at(index)) {
            return;
        }

        callbacks.at(index)(std::move(result));
    }

    std::vector<KiriView::ThumbnailCacheLookupRequest> requests;
    std::vector<KiriView::ThumbnailCacheLookupCallback> callbacks;
};

class SemaphoreReleaseOnExit
{
public:
    explicit SemaphoreReleaseOnExit(QSemaphore &semaphore)
        : m_semaphore(&semaphore)
    {
    }

    ~SemaphoreReleaseOnExit()
    {
        if (m_semaphore != nullptr) {
            m_semaphore->release();
        }
    }

    void dismiss() { m_semaphore = nullptr; }

private:
    QSemaphore *m_semaphore = nullptr;
};

class MinimumThreadPoolConcurrency
{
public:
    explicit MinimumThreadPoolConcurrency(int minimumThreadCount)
        : m_previousThreadCount(QThreadPool::globalInstance()->maxThreadCount())
    {
        if (m_previousThreadCount < minimumThreadCount) {
            QThreadPool::globalInstance()->setMaxThreadCount(minimumThreadCount);
        }
    }

    ~MinimumThreadPoolConcurrency()
    {
        QThreadPool::globalInstance()->setMaxThreadCount(m_previousThreadCount);
    }

private:
    int m_previousThreadCount = 0;
};

class ManualRawEmbeddedThumbnailPreviewExtractor
{
public:
    KiriView::RawEmbeddedThumbnailPreviewExtractor extractor()
    {
        return [this](const QByteArray &data, const KiriView::ImageDecodeRequest &request) {
            if (mayReturn != nullptr) {
                mayReturn->acquire();
            }

            std::lock_guard lock(m_mutex);
            ++m_callCount;
            m_requests.push_back(request);
            m_dataSizes.push_back(data.size());
            return result;
        };
    }

    int callCount() const
    {
        std::lock_guard lock(m_mutex);
        return m_callCount;
    }

    std::vector<KiriView::ImageDecodeRequest> requests() const
    {
        std::lock_guard lock(m_mutex);
        return m_requests;
    }

    std::vector<qsizetype> dataSizes() const
    {
        std::lock_guard lock(m_mutex);
        return m_dataSizes;
    }

    KiriView::RawEmbeddedThumbnailPreviewResult result {
        KiriView::RawEmbeddedThumbnailPreviewStatus::Ready,
        thumbnailImage(QSize(16, 16)),
        QSize(32, 32),
        {},
    };
    QSemaphore *mayReturn = nullptr;

private:
    mutable std::mutex m_mutex;
    int m_callCount = 0;
    std::vector<KiriView::ImageDecodeRequest> m_requests;
    std::vector<qsizetype> m_dataSizes;
};

KiriView::ImageDecodeDependencies imageDecodeDependenciesForThumbnailPreview(
    ManualImageDataLoader &dataLoader, KiriView::ImageDataDecoder dataDecoder,
    ManualThumbnailLookupProvider &thumbnailLookup,
    ManualRawEmbeddedThumbnailPreviewExtractor *rawExtractor = nullptr)
{
    KiriView::ImageDecodeDependencies dependencies
        = imageDecodeDependenciesFor(dataLoader, std::move(dataDecoder));
    dependencies.thumbnailPreviewLookupProvider = thumbnailLookup.provider();
    if (rawExtractor != nullptr) {
        dependencies.rawEmbeddedThumbnailPreviewExtractor = rawExtractor->extractor();
    }
    return dependencies;
}
}

class TestImageDecodeJob : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cancelSuppressesPendingLoad();
    void staleLoadResultIsIgnored();
    void restartedSameRequestIgnoresStaleLoadResult();
    void loadErrorsAreDeliveredForCurrentRequest();
    void decodeErrorsAreDeliveredAsResults();
    void decodeRequestIsPassedToDecoder();
    void xdgThumbnailPreviewIsDeliveredBeforeDecodeCompletes();
    void staleXdgThumbnailPreviewIsIgnored();
    void nonRawXdgMissDoesNotRunRawEmbeddedPreview();
    void rawEmbeddedPreviewIsDeliveredAfterXdgMiss();
    void rawEmbeddedPreviewMissDoesNotPublish();
    void lateRawEmbeddedPreviewAfterDecodeIsIgnored();
    void readyXdgThumbnailPreviewSuppressesRawEmbeddedPreview();
};

void TestImageDecodeJob::cancelSuppressesPendingLoad()
{
    ManualImageDataLoader dataLoader;
    int decodedCount = 0;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        decodeJobCallbacks([&decodedCount](KiriView::ImageDecodeRequest,
                               KiriView::DecodedImageResult) { ++decodedCount; }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(1, indexedImageUrl(1)));
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    decodeJob.cancel();

    QVERIFY(dataLoader.frontLoad().canceled);
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTest::qWait(50);
    QCOMPARE(decodedCount, 0);
}

void TestImageDecodeJob::staleLoadResultIsIgnored()
{
    ManualImageDataLoader dataLoader;
    std::vector<KiriView::ImageDecodeRequest> decodedRequests;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        decodeJobCallbacks(
            [&decodedRequests](KiriView::ImageDecodeRequest request, KiriView::DecodedImageResult) {
                decodedRequests.push_back(request);
            }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(1, indexedImageUrl(1)));
    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(2, indexedImageUrl(2)));
    QCOMPARE(dataLoader.loadCount(), std::size_t(2));
    QVERIFY(dataLoader.frontLoad().canceled);

    dataLoader.deliverFrontLoadDataIgnoringCancellation(QByteArrayLiteral("ok"));
    dataLoader.finishBackLoad(QByteArrayLiteral("ok"));

    QTRY_COMPARE(decodedRequests.size(), std::size_t(1));
    QCOMPARE(decodedRequests.front().id(), quint64(2));
    QCOMPARE(decodedRequests.front().imageUrl(), indexedImageUrl(2));
}

void TestImageDecodeJob::restartedSameRequestIgnoresStaleLoadResult()
{
    ManualImageDataLoader dataLoader;
    QByteArray decodedData;
    int decodedCount = 0;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader,
            [&decodedData](const QByteArray &data, const KiriView::ImageDecodeRequest &) {
                decodedData = data;
                return KiriView::successfulDecodedImageResult(
                    KiriView::TestSupport::staticDecodedTestImage());
            }),
        decodeJobCallbacks([&decodedCount](KiriView::ImageDecodeRequest,
                               KiriView::DecodedImageResult) { ++decodedCount; }));
    const KiriView::ImageDecodeRequest request
        = KiriView::ImageDecodeRequest::fromUrl(7, indexedImageUrl(7));

    decodeJob.start(request);
    decodeJob.start(request);
    QCOMPARE(dataLoader.loadCount(), std::size_t(2));
    QVERIFY(dataLoader.frontLoad().canceled);

    dataLoader.deliverFrontLoadDataIgnoringCancellation(QByteArrayLiteral("old"));
    QTest::qWait(50);
    QCOMPARE(decodedCount, 0);

    dataLoader.finishBackLoad(QByteArrayLiteral("new"));

    QTRY_COMPARE(decodedCount, 1);
    QCOMPARE(decodedData, QByteArrayLiteral("new"));
}

void TestImageDecodeJob::loadErrorsAreDeliveredForCurrentRequest()
{
    ManualImageDataLoader dataLoader;
    std::vector<KiriView::ImageDecodeRequest> errorRequests;
    QString errorString;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        decodeJobCallbacks({},
            [&errorRequests, &errorString](
                const KiriView::ImageDecodeRequest &request, const QString &error) {
                errorRequests.push_back(request);
                errorString = error;
            }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(3, indexedImageUrl(3)));
    dataLoader.failFrontLoad(QStringLiteral("missing"));

    QCOMPARE(errorRequests.size(), std::size_t(1));
    QCOMPARE(errorRequests.front().id(), quint64(3));
    QCOMPARE(errorString, QStringLiteral("missing"));
    QVERIFY(!decodeJob.hasActiveRequest());
}

void TestImageDecodeJob::decodeErrorsAreDeliveredAsResults()
{
    ManualImageDataLoader dataLoader;
    std::optional<KiriView::DecodedImageResult> decodedResult;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        decodeJobCallbacks(
            [&decodedResult](KiriView::ImageDecodeRequest, KiriView::DecodedImageResult result) {
                decodedResult = std::move(result);
            }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(4, indexedImageUrl(4)));
    dataLoader.finishFrontLoad(QByteArrayLiteral("bad"));

    QTRY_VERIFY(decodedResult.has_value());
    const auto *failure = KiriView::decodedImageResultFailure(*decodedResult);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, testImageDecodeFailureString());
    QVERIFY(!decodeJob.hasActiveRequest());
}

void TestImageDecodeJob::decodeRequestIsPassedToDecoder()
{
    ManualImageDataLoader dataLoader;
    std::optional<KiriView::ImageDecodeRequest> decoderRequest;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader,
            [&decoderRequest](const QByteArray &, const KiriView::ImageDecodeRequest &request) {
                decoderRequest = request;
                return KiriView::successfulDecodedImageResult(
                    KiriView::TestSupport::staticDecodedTestImage());
            }),
        decodeJobCallbacks([](KiriView::ImageDecodeRequest, KiriView::DecodedImageResult) {}));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(
        5, indexedImageUrl(5), KiriView::ImageFirstDisplayDecodeContext { QSize(320, 200) }));
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decoderRequest.has_value());
    QCOMPARE(decoderRequest->id(), quint64(5));
    QCOMPARE(decoderRequest->firstDisplay().physicalViewportSize, QSize(320, 200));
}

void TestImageDecodeJob::xdgThumbnailPreviewIsDeliveredBeforeDecodeCompletes()
{
    const QByteArray data = encodedPngData(QSize(800, 600));
    QVERIFY(!data.isEmpty());

    ManualImageDataLoader dataLoader;
    ManualThumbnailLookupProvider thumbnailLookup;
    QSemaphore decoderMayFinish;
    std::optional<KiriView::StaticDisplayImagePayload> previewPayload;
    int decodedCount = 0;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [&decoderMayFinish](const QByteArray &, const KiriView::ImageDecodeRequest &) {
                decoderMayFinish.acquire();
                return KiriView::successfulDecodedImageResult(
                    KiriView::TestSupport::staticDecodedTestImage(
                        KiriView::TestSupport::testImage(800, 600)));
            },
            thumbnailLookup),
        decodeJobCallbacks([&decodedCount](KiriView::ImageDecodeRequest,
                               KiriView::DecodedImageResult) { ++decodedCount; },
            {},
            [&previewPayload](
                const KiriView::ImageDecodeRequest &, KiriView::StaticDisplayImagePayload payload) {
                previewPayload = std::move(payload);
            }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(8, indexedImageUrl(8)));
    dataLoader.finishFrontLoad(data);

    QTRY_COMPARE(thumbnailLookup.requests.size(), std::size_t(1));
    QCOMPARE(thumbnailLookup.requests.front().requestedBucket,
        KiriView::ActiveNavigationThumbnailDemandBucket::XXLarge);
    thumbnailLookup.finish(0, readyThumbnailLookup());

    QTRY_VERIFY(previewPayload.has_value());
    QCOMPARE(previewPayload->quality, KiriView::DisplayImageQuality::ThumbnailPreview);
    QCOMPARE(previewPayload->previewOrigin, KiriView::DisplayImagePreviewOrigin::XdgThumbnail);
    QCOMPARE(previewPayload->originalSize, QSize(800, 600));
    QCOMPARE(previewPayload->image.size(), QSize(400, 300));
    QCOMPARE(decodedCount, 0);

    decoderMayFinish.release();
    QTRY_COMPARE(decodedCount, 1);
}

void TestImageDecodeJob::staleXdgThumbnailPreviewIsIgnored()
{
    const QByteArray data = encodedPngData(QSize(800, 600));
    QVERIFY(!data.isEmpty());

    ManualImageDataLoader dataLoader;
    ManualThumbnailLookupProvider thumbnailLookup;
    QSemaphore decoderMayFinish;
    std::atomic<int> decoderCalls = 0;
    int previewCount = 0;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [&decoderMayFinish, &decoderCalls](
                const QByteArray &, const KiriView::ImageDecodeRequest &) {
                ++decoderCalls;
                decoderMayFinish.acquire();
                return KiriView::successfulDecodedImageResult(
                    KiriView::TestSupport::staticDecodedTestImage(
                        KiriView::TestSupport::testImage(800, 600)));
            },
            thumbnailLookup),
        decodeJobCallbacks([](KiriView::ImageDecodeRequest, KiriView::DecodedImageResult) {}, {},
            [&previewCount](const KiriView::ImageDecodeRequest &,
                KiriView::StaticDisplayImagePayload) { ++previewCount; }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(9, indexedImageUrl(9)));
    dataLoader.finishFrontLoad(data);
    QTRY_COMPARE(thumbnailLookup.requests.size(), std::size_t(1));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(10, indexedImageUrl(10)));
    thumbnailLookup.finish(0, readyThumbnailLookup());

    QTest::qWait(50);
    QCOMPARE(previewCount, 0);
    decoderMayFinish.release(decoderCalls.load() + 1);
}

void TestImageDecodeJob::nonRawXdgMissDoesNotRunRawEmbeddedPreview()
{
    const QByteArray data = encodedPngData(QSize(800, 600));
    QVERIFY(!data.isEmpty());

    ManualImageDataLoader dataLoader;
    ManualThumbnailLookupProvider thumbnailLookup;
    ManualRawEmbeddedThumbnailPreviewExtractor rawExtractor;
    QSemaphore decoderMayFinish;
    int decodedCount = 0;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [&decoderMayFinish](const QByteArray &, const KiriView::ImageDecodeRequest &) {
                decoderMayFinish.acquire();
                return KiriView::successfulDecodedImageResult(
                    KiriView::TestSupport::staticDecodedTestImage(
                        KiriView::TestSupport::testImage(800, 600)));
            },
            thumbnailLookup, &rawExtractor),
        decodeJobCallbacks([&decodedCount](KiriView::ImageDecodeRequest,
                               KiriView::DecodedImageResult) { ++decodedCount; }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(13, indexedImageUrl(13)));
    dataLoader.finishFrontLoad(data);

    QTRY_COMPARE(thumbnailLookup.requests.size(), std::size_t(1));
    thumbnailLookup.finish(0, missingThumbnailLookup());

    QTest::qWait(50);
    QCOMPARE(rawExtractor.callCount(), 0);
    QCOMPARE(decodedCount, 0);

    decoderMayFinish.release();
    QTRY_COMPARE(decodedCount, 1);
}

void TestImageDecodeJob::rawEmbeddedPreviewIsDeliveredAfterXdgMiss()
{
    MinimumThreadPoolConcurrency threadPoolConcurrency(2);
    const QByteArray data = rawFixtureData();
    QVERIFY(!data.isEmpty());

    ManualImageDataLoader dataLoader;
    ManualThumbnailLookupProvider thumbnailLookup;
    ManualRawEmbeddedThumbnailPreviewExtractor rawExtractor;
    QSemaphore decoderMayFinish;
    SemaphoreReleaseOnExit releaseDecoderOnExit(decoderMayFinish);
    std::optional<KiriView::StaticDisplayImagePayload> previewPayload;
    int decodedCount = 0;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [&decoderMayFinish](const QByteArray &, const KiriView::ImageDecodeRequest &) {
                decoderMayFinish.acquire();
                return KiriView::successfulDecodedImageResult(
                    KiriView::TestSupport::staticDecodedTestImage(
                        KiriView::TestSupport::testImage(32, 32)));
            },
            thumbnailLookup, &rawExtractor),
        decodeJobCallbacks([&decodedCount](KiriView::ImageDecodeRequest,
                               KiriView::DecodedImageResult) { ++decodedCount; },
            {},
            [&previewPayload](
                const KiriView::ImageDecodeRequest &, KiriView::StaticDisplayImagePayload payload) {
                previewPayload = std::move(payload);
            }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(
        11, QUrl::fromLocalFile(QStringLiteral("/tmp/raw-cfa-smoke.dng"))));
    dataLoader.finishFrontLoad(data);

    QTRY_COMPARE(thumbnailLookup.requests.size(), std::size_t(1));
    QCOMPARE(rawExtractor.callCount(), 0);

    thumbnailLookup.finish(0, missingThumbnailLookup());

    QTRY_COMPARE(rawExtractor.callCount(), 1);
    const std::vector<KiriView::ImageDecodeRequest> rawRequests = rawExtractor.requests();
    const std::vector<qsizetype> rawDataSizes = rawExtractor.dataSizes();
    QCOMPARE(rawRequests.front().id(), quint64(11));
    QCOMPARE(rawDataSizes.front(), data.size());
    QTRY_VERIFY(previewPayload.has_value());
    QCOMPARE(previewPayload->quality, KiriView::DisplayImageQuality::ThumbnailPreview);
    QCOMPARE(
        previewPayload->previewOrigin, KiriView::DisplayImagePreviewOrigin::RawEmbeddedThumbnail);
    QCOMPARE(previewPayload->originalSize, QSize(32, 32));
    QCOMPARE(previewPayload->image.size(), QSize(16, 16));
    QVERIFY(previewPayload->refinementSource == nullptr);
    QCOMPARE(decodedCount, 0);

    decoderMayFinish.release();
    releaseDecoderOnExit.dismiss();
    QTRY_COMPARE(decodedCount, 1);
}

void TestImageDecodeJob::rawEmbeddedPreviewMissDoesNotPublish()
{
    MinimumThreadPoolConcurrency threadPoolConcurrency(2);
    const QByteArray data = rawFixtureData();
    QVERIFY(!data.isEmpty());

    ManualImageDataLoader dataLoader;
    ManualThumbnailLookupProvider thumbnailLookup;
    ManualRawEmbeddedThumbnailPreviewExtractor rawExtractor;
    rawExtractor.result.status = KiriView::RawEmbeddedThumbnailPreviewStatus::Missing;
    rawExtractor.result.image = {};
    QSemaphore decoderMayFinish;
    int previewCount = 0;
    int decodedCount = 0;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [&decoderMayFinish](const QByteArray &, const KiriView::ImageDecodeRequest &) {
                decoderMayFinish.acquire();
                return KiriView::successfulDecodedImageResult(
                    KiriView::TestSupport::staticDecodedTestImage(
                        KiriView::TestSupport::testImage(32, 32)));
            },
            thumbnailLookup, &rawExtractor),
        decodeJobCallbacks([&decodedCount](KiriView::ImageDecodeRequest,
                               KiriView::DecodedImageResult) { ++decodedCount; },
            {},
            [&previewCount](const KiriView::ImageDecodeRequest &,
                KiriView::StaticDisplayImagePayload) { ++previewCount; }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(
        14, QUrl::fromLocalFile(QStringLiteral("/tmp/raw-cfa-smoke.dng"))));
    dataLoader.finishFrontLoad(data);

    QTRY_COMPARE(thumbnailLookup.requests.size(), std::size_t(1));
    thumbnailLookup.finish(0, missingThumbnailLookup());

    QTRY_COMPARE(rawExtractor.callCount(), 1);
    QTest::qWait(50);
    QCOMPARE(previewCount, 0);
    QCOMPARE(decodedCount, 0);

    decoderMayFinish.release();
    QTRY_COMPARE(decodedCount, 1);
}

void TestImageDecodeJob::lateRawEmbeddedPreviewAfterDecodeIsIgnored()
{
    MinimumThreadPoolConcurrency threadPoolConcurrency(2);
    const QByteArray data = rawFixtureData();
    QVERIFY(!data.isEmpty());

    ManualImageDataLoader dataLoader;
    ManualThumbnailLookupProvider thumbnailLookup;
    ManualRawEmbeddedThumbnailPreviewExtractor rawExtractor;
    QSemaphore rawExtractorMayReturn;
    SemaphoreReleaseOnExit releaseRawExtractorOnExit(rawExtractorMayReturn);
    rawExtractor.mayReturn = &rawExtractorMayReturn;
    int previewCount = 0;
    int decodedCount = 0;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
                return KiriView::successfulDecodedImageResult(
                    KiriView::TestSupport::staticDecodedTestImage(
                        KiriView::TestSupport::testImage(32, 32)));
            },
            thumbnailLookup, &rawExtractor),
        decodeJobCallbacks([&decodedCount](KiriView::ImageDecodeRequest,
                               KiriView::DecodedImageResult) { ++decodedCount; },
            {},
            [&previewCount](const KiriView::ImageDecodeRequest &,
                KiriView::StaticDisplayImagePayload) { ++previewCount; }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(
        15, QUrl::fromLocalFile(QStringLiteral("/tmp/raw-cfa-smoke.dng"))));
    dataLoader.finishFrontLoad(data);

    QTRY_COMPARE(thumbnailLookup.requests.size(), std::size_t(1));
    thumbnailLookup.finish(0, missingThumbnailLookup());
    QTRY_COMPARE(decodedCount, 1);

    rawExtractorMayReturn.release();
    releaseRawExtractorOnExit.dismiss();
    QTRY_COMPARE(rawExtractor.callCount(), 1);
    QTest::qWait(50);
    QCOMPARE(previewCount, 0);
}

void TestImageDecodeJob::readyXdgThumbnailPreviewSuppressesRawEmbeddedPreview()
{
    const QByteArray data = rawFixtureData();
    QVERIFY(!data.isEmpty());

    ManualImageDataLoader dataLoader;
    ManualThumbnailLookupProvider thumbnailLookup;
    ManualRawEmbeddedThumbnailPreviewExtractor rawExtractor;
    QSemaphore decoderMayFinish;
    std::optional<KiriView::StaticDisplayImagePayload> previewPayload;
    int decodedCount = 0;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [&decoderMayFinish](const QByteArray &, const KiriView::ImageDecodeRequest &) {
                decoderMayFinish.acquire();
                return KiriView::successfulDecodedImageResult(
                    KiriView::TestSupport::staticDecodedTestImage(
                        KiriView::TestSupport::testImage(32, 32)));
            },
            thumbnailLookup, &rawExtractor),
        decodeJobCallbacks([&decodedCount](KiriView::ImageDecodeRequest,
                               KiriView::DecodedImageResult) { ++decodedCount; },
            {},
            [&previewPayload](
                const KiriView::ImageDecodeRequest &, KiriView::StaticDisplayImagePayload payload) {
                previewPayload = std::move(payload);
            }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(
        12, QUrl::fromLocalFile(QStringLiteral("/tmp/raw-cfa-smoke.dng"))));
    dataLoader.finishFrontLoad(data);

    QTRY_COMPARE(thumbnailLookup.requests.size(), std::size_t(1));
    thumbnailLookup.finish(0, readyThumbnailLookup(thumbnailImage(QSize(16, 16))));

    QTRY_VERIFY(previewPayload.has_value());
    QCOMPARE(previewPayload->previewOrigin, KiriView::DisplayImagePreviewOrigin::XdgThumbnail);
    QCOMPARE(rawExtractor.callCount(), 0);
    QCOMPARE(decodedCount, 0);

    decoderMayFinish.release();
    QTRY_COMPARE(decodedCount, 1);
}

QTEST_GUILESS_MAIN(TestImageDecodeJob)

#include "test_imagedecodejob.moc"
