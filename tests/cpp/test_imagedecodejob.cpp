// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/imagedecodejob.h"
#include "decoding/rawthumbnailpreview.h"
#include "image_test_support.h"
#include "thumbnail/thumbnailcachelookup.h"

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
using kiriview::TestSupport::imageDecodeDependenciesFor;
using kiriview::TestSupport::indexedImageUrl;
using kiriview::TestSupport::ManualImageDataLoader;
using kiriview::TestSupport::staticImageDataDecoderRejectingBadData;
using kiriview::TestSupport::testImageDecodeFailureString;

kiriview::ImageDecodeJob::Callbacks decodeJobCallbacks(
    kiriview::ImageDecodeJob::DecodedCallback decoded = {},
    kiriview::ImageDecodeJob::LoadErrorCallback loadError = {},
    kiriview::ImageDecodeJob::ThumbnailPreviewCallback thumbnailPreview = {})
{
    return kiriview::ImageDecodeJob::Callbacks {
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

kiriview::ThumbnailCacheLookupResult readyThumbnailLookup(QImage image = thumbnailImage())
{
    return kiriview::ThumbnailCacheLookupResult {
        kiriview::ThumbnailCacheLookupStatus::Ready,
        std::move(image),
        kiriview::ActiveNavigationThumbnailDemandBucket::XXLarge,
        kiriview::ActiveNavigationThumbnailDemandBucket::XXLarge,
        QStringLiteral("/cache/photo.png"),
        {},
    };
}

kiriview::ThumbnailCacheLookupResult missingThumbnailLookup()
{
    return kiriview::ThumbnailCacheLookupResult {
        kiriview::ThumbnailCacheLookupStatus::Missing,
        {},
        kiriview::ActiveNavigationThumbnailDemandBucket::XXLarge,
        kiriview::ActiveNavigationThumbnailDemandBucket::None,
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
    kiriview::ThumbnailCacheLookupProvider provider()
    {
        return [this](QObject *, kiriview::ThumbnailCacheLookupRequest request,
                   kiriview::ThumbnailCacheLookupCallback callback) {
            requests.push_back(std::move(request));
            callbacks.push_back(std::move(callback));
            return kiriview::ImageIoJob();
        };
    }

    void finish(std::size_t index, kiriview::ThumbnailCacheLookupResult result)
    {
        if (index >= callbacks.size() || !callbacks.at(index)) {
            return;
        }

        callbacks.at(index)(std::move(result));
    }

    std::vector<kiriview::ThumbnailCacheLookupRequest> requests;
    std::vector<kiriview::ThumbnailCacheLookupCallback> callbacks;
};

struct ManualImageWorkerSchedule {
    kiriview::ImageWorkerOperation work;
    kiriview::ImageWorkerCompletion completion;
};

class ManualImageWorkerScheduler
{
public:
    kiriview::ImageWorkerScheduler scheduler()
    {
        return kiriview::ImageWorkerScheduler([this](QObject *, kiriview::ImageWorkerOperation work,
                                                  kiriview::ImageWorkerCompletion completion) {
            m_schedules.push_back(
                ManualImageWorkerSchedule { std::move(work), std::move(completion) });
        });
    }

    std::size_t scheduleCount() const { return m_schedules.size(); }

    void runWork(std::size_t index)
    {
        if (m_schedules.at(index).work) {
            m_schedules.at(index).work();
        }
    }

    void finish(std::size_t index)
    {
        if (m_schedules.at(index).completion) {
            m_schedules.at(index).completion();
        }
    }

private:
    std::vector<ManualImageWorkerSchedule> m_schedules;
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
    kiriview::RawEmbeddedThumbnailPreviewExtractor extractor()
    {
        return [this](const QByteArray &data, const kiriview::ImageDecodeRequest &request) {
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

    std::vector<kiriview::ImageDecodeRequest> requests() const
    {
        std::lock_guard lock(m_mutex);
        return m_requests;
    }

    std::vector<qsizetype> dataSizes() const
    {
        std::lock_guard lock(m_mutex);
        return m_dataSizes;
    }

    kiriview::RawEmbeddedThumbnailPreviewResult result {
        kiriview::RawEmbeddedThumbnailPreviewStatus::Ready,
        thumbnailImage(QSize(16, 16)),
        QSize(32, 32),
        {},
    };
    QSemaphore *mayReturn = nullptr;

private:
    mutable std::mutex m_mutex;
    int m_callCount = 0;
    std::vector<kiriview::ImageDecodeRequest> m_requests;
    std::vector<qsizetype> m_dataSizes;
};

kiriview::ImageDecodeDependencies imageDecodeDependenciesForThumbnailPreview(
    ManualImageDataLoader &dataLoader, kiriview::ImageDataDecoder dataDecoder,
    ManualThumbnailLookupProvider &thumbnailLookup,
    ManualRawEmbeddedThumbnailPreviewExtractor *rawExtractor = nullptr)
{
    kiriview::ImageDecodeDependencies dependencies
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
    void decodeWorkerSchedulerCanBeDrivenManually();
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
    kiriview::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        decodeJobCallbacks([&decodedCount](kiriview::ImageDecodeRequest,
                               kiriview::DecodedImageResult) { ++decodedCount; }));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(1, indexedImageUrl(1)));
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
    std::vector<kiriview::ImageDecodeRequest> decodedRequests;
    kiriview::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        decodeJobCallbacks(
            [&decodedRequests](kiriview::ImageDecodeRequest request, kiriview::DecodedImageResult) {
                decodedRequests.push_back(request);
            }));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(1, indexedImageUrl(1)));
    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(2, indexedImageUrl(2)));
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
    kiriview::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader,
            [&decodedData](const QByteArray &data, const kiriview::ImageDecodeRequest &) {
                decodedData = data;
                return kiriview::successfulDecodedImageResult(
                    kiriview::TestSupport::staticDecodedTestImage());
            }),
        decodeJobCallbacks([&decodedCount](kiriview::ImageDecodeRequest,
                               kiriview::DecodedImageResult) { ++decodedCount; }));
    const kiriview::ImageDecodeRequest request
        = kiriview::ImageDecodeRequest::fromUrl(7, indexedImageUrl(7));

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
    std::vector<kiriview::ImageDecodeRequest> errorRequests;
    QString errorString;
    kiriview::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        decodeJobCallbacks({},
            [&errorRequests, &errorString](
                const kiriview::ImageDecodeRequest &request, const QString &error) {
                errorRequests.push_back(request);
                errorString = error;
            }));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(3, indexedImageUrl(3)));
    dataLoader.failFrontLoad(QStringLiteral("missing"));

    QCOMPARE(errorRequests.size(), std::size_t(1));
    QCOMPARE(errorRequests.front().id(), quint64(3));
    QCOMPARE(errorString, QStringLiteral("missing"));
    QVERIFY(!decodeJob.hasActiveRequest());
}

void TestImageDecodeJob::decodeErrorsAreDeliveredAsResults()
{
    ManualImageDataLoader dataLoader;
    std::optional<kiriview::DecodedImageResult> decodedResult;
    kiriview::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        decodeJobCallbacks(
            [&decodedResult](kiriview::ImageDecodeRequest, kiriview::DecodedImageResult result) {
                decodedResult = std::move(result);
            }));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(4, indexedImageUrl(4)));
    dataLoader.finishFrontLoad(QByteArrayLiteral("bad"));

    QTRY_VERIFY(decodedResult.has_value());
    const auto *failure = kiriview::decodedImageResultFailure(*decodedResult);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, testImageDecodeFailureString());
    QVERIFY(!decodeJob.hasActiveRequest());
}

void TestImageDecodeJob::decodeRequestIsPassedToDecoder()
{
    ManualImageDataLoader dataLoader;
    std::optional<kiriview::ImageDecodeRequest> decoderRequest;
    kiriview::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader,
            [&decoderRequest](const QByteArray &, const kiriview::ImageDecodeRequest &request) {
                decoderRequest = request;
                return kiriview::successfulDecodedImageResult(
                    kiriview::TestSupport::staticDecodedTestImage());
            }),
        decodeJobCallbacks([](kiriview::ImageDecodeRequest, kiriview::DecodedImageResult) {}));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(
        5, indexedImageUrl(5), kiriview::ImageFirstDisplayDecodeContext { QSize(320, 200) }));
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decoderRequest.has_value());
    QCOMPARE(decoderRequest->id(), quint64(5));
    QCOMPARE(decoderRequest->firstDisplay().physicalViewportSize, QSize(320, 200));
}

void TestImageDecodeJob::decodeWorkerSchedulerCanBeDrivenManually()
{
    ManualImageDataLoader dataLoader;
    ManualImageWorkerScheduler workerScheduler;
    bool decoderCalled = false;
    std::vector<kiriview::ImageDecodeRequest> decodedRequests;
    kiriview::ImageDecodeDependencies dependencies = imageDecodeDependenciesFor(
        dataLoader, [&decoderCalled](const QByteArray &, const kiriview::ImageDecodeRequest &) {
            decoderCalled = true;
            return kiriview::successfulDecodedImageResult(
                kiriview::TestSupport::staticDecodedTestImage());
        });
    dependencies.workerScheduler = workerScheduler.scheduler();
    kiriview::ImageDecodeJob decodeJob(this, std::move(dependencies),
        decodeJobCallbacks(
            [&decodedRequests](kiriview::ImageDecodeRequest request, kiriview::DecodedImageResult) {
                decodedRequests.push_back(std::move(request));
            }));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(16, indexedImageUrl(16)));
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QCOMPARE(workerScheduler.scheduleCount(), std::size_t(1));
    QVERIFY(!decoderCalled);
    QCOMPARE(decodedRequests.size(), std::size_t(0));

    workerScheduler.runWork(0);

    QVERIFY(decoderCalled);
    QCOMPARE(decodedRequests.size(), std::size_t(0));

    workerScheduler.finish(0);

    QCOMPARE(decodedRequests.size(), std::size_t(1));
    QCOMPARE(decodedRequests.front().id(), quint64(16));
    QVERIFY(!decodeJob.hasActiveRequest());
}

void TestImageDecodeJob::xdgThumbnailPreviewIsDeliveredBeforeDecodeCompletes()
{
    const QByteArray data = encodedPngData(QSize(800, 600));
    QVERIFY(!data.isEmpty());

    ManualImageDataLoader dataLoader;
    ManualThumbnailLookupProvider thumbnailLookup;
    QSemaphore decoderMayFinish;
    std::optional<kiriview::StaticDisplayImagePayload> previewPayload;
    int decodedCount = 0;
    kiriview::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [&decoderMayFinish](const QByteArray &, const kiriview::ImageDecodeRequest &) {
                decoderMayFinish.acquire();
                return kiriview::successfulDecodedImageResult(
                    kiriview::TestSupport::staticDecodedTestImage(
                        kiriview::TestSupport::testImage(800, 600)));
            },
            thumbnailLookup),
        decodeJobCallbacks([&decodedCount](kiriview::ImageDecodeRequest,
                               kiriview::DecodedImageResult) { ++decodedCount; },
            {},
            [&previewPayload](
                const kiriview::ImageDecodeRequest &, kiriview::StaticDisplayImagePayload payload) {
                previewPayload = std::move(payload);
            }));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(8, indexedImageUrl(8)));
    dataLoader.finishFrontLoad(data);

    QTRY_COMPARE(thumbnailLookup.requests.size(), std::size_t(1));
    QCOMPARE(thumbnailLookup.requests.front().requestedBucket,
        kiriview::ActiveNavigationThumbnailDemandBucket::XXLarge);
    thumbnailLookup.finish(0, readyThumbnailLookup());

    QTRY_VERIFY(previewPayload.has_value());
    QCOMPARE(previewPayload->quality, kiriview::DisplayImageQuality::ThumbnailPreview);
    QCOMPARE(previewPayload->previewOrigin, kiriview::DisplayImagePreviewOrigin::XdgThumbnail);
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
    kiriview::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [&decoderMayFinish, &decoderCalls](
                const QByteArray &, const kiriview::ImageDecodeRequest &) {
                ++decoderCalls;
                decoderMayFinish.acquire();
                return kiriview::successfulDecodedImageResult(
                    kiriview::TestSupport::staticDecodedTestImage(
                        kiriview::TestSupport::testImage(800, 600)));
            },
            thumbnailLookup),
        decodeJobCallbacks([](kiriview::ImageDecodeRequest, kiriview::DecodedImageResult) {}, {},
            [&previewCount](const kiriview::ImageDecodeRequest &,
                kiriview::StaticDisplayImagePayload) { ++previewCount; }));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(9, indexedImageUrl(9)));
    dataLoader.finishFrontLoad(data);
    QTRY_COMPARE(thumbnailLookup.requests.size(), std::size_t(1));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(10, indexedImageUrl(10)));
    thumbnailLookup.finish(0, readyThumbnailLookup());

    QTest::qWait(50);
    QCOMPARE(previewCount, 0);
    decoderMayFinish.release(2);
    QVERIFY(QThreadPool::globalInstance()->waitForDone(5000));
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
    kiriview::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [&decoderMayFinish](const QByteArray &, const kiriview::ImageDecodeRequest &) {
                decoderMayFinish.acquire();
                return kiriview::successfulDecodedImageResult(
                    kiriview::TestSupport::staticDecodedTestImage(
                        kiriview::TestSupport::testImage(800, 600)));
            },
            thumbnailLookup, &rawExtractor),
        decodeJobCallbacks([&decodedCount](kiriview::ImageDecodeRequest,
                               kiriview::DecodedImageResult) { ++decodedCount; }));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(13, indexedImageUrl(13)));
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
    std::optional<kiriview::StaticDisplayImagePayload> previewPayload;
    int decodedCount = 0;
    kiriview::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [&decoderMayFinish](const QByteArray &, const kiriview::ImageDecodeRequest &) {
                decoderMayFinish.acquire();
                return kiriview::successfulDecodedImageResult(
                    kiriview::TestSupport::staticDecodedTestImage(
                        kiriview::TestSupport::testImage(32, 32)));
            },
            thumbnailLookup, &rawExtractor),
        decodeJobCallbacks([&decodedCount](kiriview::ImageDecodeRequest,
                               kiriview::DecodedImageResult) { ++decodedCount; },
            {},
            [&previewPayload](
                const kiriview::ImageDecodeRequest &, kiriview::StaticDisplayImagePayload payload) {
                previewPayload = std::move(payload);
            }));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(
        11, QUrl::fromLocalFile(QStringLiteral("/tmp/raw-cfa-smoke.dng"))));
    dataLoader.finishFrontLoad(data);

    QTRY_COMPARE(thumbnailLookup.requests.size(), std::size_t(1));
    QCOMPARE(rawExtractor.callCount(), 0);

    thumbnailLookup.finish(0, missingThumbnailLookup());

    QTRY_COMPARE(rawExtractor.callCount(), 1);
    const std::vector<kiriview::ImageDecodeRequest> rawRequests = rawExtractor.requests();
    const std::vector<qsizetype> rawDataSizes = rawExtractor.dataSizes();
    QCOMPARE(rawRequests.front().id(), quint64(11));
    QCOMPARE(rawDataSizes.front(), data.size());
    QTRY_VERIFY(previewPayload.has_value());
    QCOMPARE(previewPayload->quality, kiriview::DisplayImageQuality::ThumbnailPreview);
    QCOMPARE(
        previewPayload->previewOrigin, kiriview::DisplayImagePreviewOrigin::RawEmbeddedThumbnail);
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
    rawExtractor.result.status = kiriview::RawEmbeddedThumbnailPreviewStatus::Missing;
    rawExtractor.result.image = {};
    QSemaphore decoderMayFinish;
    int previewCount = 0;
    int decodedCount = 0;
    kiriview::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [&decoderMayFinish](const QByteArray &, const kiriview::ImageDecodeRequest &) {
                decoderMayFinish.acquire();
                return kiriview::successfulDecodedImageResult(
                    kiriview::TestSupport::staticDecodedTestImage(
                        kiriview::TestSupport::testImage(32, 32)));
            },
            thumbnailLookup, &rawExtractor),
        decodeJobCallbacks([&decodedCount](kiriview::ImageDecodeRequest,
                               kiriview::DecodedImageResult) { ++decodedCount; },
            {},
            [&previewCount](const kiriview::ImageDecodeRequest &,
                kiriview::StaticDisplayImagePayload) { ++previewCount; }));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(
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
    kiriview::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [](const QByteArray &, const kiriview::ImageDecodeRequest &) {
                return kiriview::successfulDecodedImageResult(
                    kiriview::TestSupport::staticDecodedTestImage(
                        kiriview::TestSupport::testImage(32, 32)));
            },
            thumbnailLookup, &rawExtractor),
        decodeJobCallbacks([&decodedCount](kiriview::ImageDecodeRequest,
                               kiriview::DecodedImageResult) { ++decodedCount; },
            {},
            [&previewCount](const kiriview::ImageDecodeRequest &,
                kiriview::StaticDisplayImagePayload) { ++previewCount; }));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(
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
    QVERIFY(QThreadPool::globalInstance()->waitForDone(5000));
}

void TestImageDecodeJob::readyXdgThumbnailPreviewSuppressesRawEmbeddedPreview()
{
    const QByteArray data = rawFixtureData();
    QVERIFY(!data.isEmpty());

    ManualImageDataLoader dataLoader;
    ManualThumbnailLookupProvider thumbnailLookup;
    ManualRawEmbeddedThumbnailPreviewExtractor rawExtractor;
    QSemaphore decoderMayFinish;
    std::optional<kiriview::StaticDisplayImagePayload> previewPayload;
    int decodedCount = 0;
    kiriview::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesForThumbnailPreview(
            dataLoader,
            [&decoderMayFinish](const QByteArray &, const kiriview::ImageDecodeRequest &) {
                decoderMayFinish.acquire();
                return kiriview::successfulDecodedImageResult(
                    kiriview::TestSupport::staticDecodedTestImage(
                        kiriview::TestSupport::testImage(32, 32)));
            },
            thumbnailLookup, &rawExtractor),
        decodeJobCallbacks([&decodedCount](kiriview::ImageDecodeRequest,
                               kiriview::DecodedImageResult) { ++decodedCount; },
            {},
            [&previewPayload](
                const kiriview::ImageDecodeRequest &, kiriview::StaticDisplayImagePayload payload) {
                previewPayload = std::move(payload);
            }));

    decodeJob.start(kiriview::ImageDecodeRequest::fromUrl(
        12, QUrl::fromLocalFile(QStringLiteral("/tmp/raw-cfa-smoke.dng"))));
    dataLoader.finishFrontLoad(data);

    QTRY_COMPARE(thumbnailLookup.requests.size(), std::size_t(1));
    thumbnailLookup.finish(0, readyThumbnailLookup(thumbnailImage(QSize(16, 16))));

    QTRY_VERIFY(previewPayload.has_value());
    QCOMPARE(previewPayload->previewOrigin, kiriview::DisplayImagePreviewOrigin::XdgThumbnail);
    QCOMPARE(rawExtractor.callCount(), 0);
    QCOMPARE(decodedCount, 0);

    decoderMayFinish.release();
    QTRY_COMPARE(decodedCount, 1);
}

QTEST_GUILESS_MAIN(TestImageDecodeJob)

#include "test_imagedecodejob.moc"
