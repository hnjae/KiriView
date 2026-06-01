// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_async_test_support.h"
#include "session/activenavigationthumbnailruntime.h"

#include <QAbstractItemModel>
#include <QColor>
#include <QImage>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <QtGlobal>
#include <vector>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

KiriView::ActiveNavigationThumbnailRow thumbnailRow(int number, const QUrl &url,
    const QString &label, KiriView::ActiveNavigationThumbnailSourceKind sourceKind,
    bool current = false)
{
    return KiriView::ActiveNavigationThumbnailRow {
        number,
        url,
        label,
        sourceKind == KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo
                || sourceKind
                    == KiriView::ActiveNavigationThumbnailSourceKind::ImageDocumentPageVideo
            ? KiriView::ActiveNavigationThumbnailKind::Video
            : KiriView::ActiveNavigationThumbnailKind::Image,
        sourceKind,
        current,
    };
}

int roleForName(const QAbstractItemModel &model, const QByteArray &name)
{
    const QHash<int, QByteArray> roles = model.roleNames();
    for (auto iterator = roles.cbegin(); iterator != roles.cend(); ++iterator) {
        if (iterator.value() == name) {
            return iterator.key();
        }
    }

    return -1;
}

QVariant modelData(const QAbstractItemModel &model, int row, const QByteArray &roleName)
{
    const int role = roleForName(model, roleName);
    if (role < 0) {
        return {};
    }

    return model.data(model.index(row, 0), role);
}

struct ManualThumbnailLookup {
    QObject *object = nullptr;
    KiriView::ThumbnailCacheLookupRequest request;
    KiriView::ThumbnailCacheLookupCallback callback;
    KiriView::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualThumbnailLookupProvider
{
public:
    KiriView::ThumbnailCacheLookupProvider provider()
    {
        return [this](QObject *receiver, KiriView::ThumbnailCacheLookupRequest request,
                   KiriView::ThumbnailCacheLookupCallback callback) {
            auto lookup = std::make_shared<ManualThumbnailLookup>();
            lookup->request = std::move(request);
            lookup->callback = std::move(callback);

            KiriView::ImageIoJob job
                = KiriView::TestSupport::Detail::startManualIoJob(receiver, lookup);
            m_lookups.push_back(lookup);
            return job;
        };
    }

    std::size_t lookupCount() const { return m_lookups.size(); }

    ManualThumbnailLookup &lookupAt(std::size_t index) { return *m_lookups.at(index); }

    void finish(std::size_t index, KiriView::ThumbnailCacheLookupResult result)
    {
        KiriView::TestSupport::Detail::finishManualIoJob(m_lookups.at(index),
            [result = std::move(result)](ManualThumbnailLookup &lookup) mutable {
                if (lookup.callback) {
                    lookup.callback(std::move(result));
                }
            });
    }

    void deliverIgnoringCancellation(std::size_t index, KiriView::ThumbnailCacheLookupResult result)
    {
        ManualThumbnailLookup &lookup = lookupAt(index);
        if (lookup.callback) {
            lookup.callback(std::move(result));
        }
    }

private:
    std::vector<std::shared_ptr<ManualThumbnailLookup>> m_lookups;
};

struct ManualThumbnailGeneration {
    QObject *object = nullptr;
    KiriView::ThumbnailGenerationRequest request;
    KiriView::ThumbnailGenerationCallback callback;
    KiriView::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualThumbnailGenerationProvider
{
public:
    KiriView::ThumbnailGenerationProvider provider()
    {
        return [this](QObject *receiver, KiriView::ThumbnailGenerationRequest request,
                   KiriView::ThumbnailGenerationCallback callback) {
            auto generation = std::make_shared<ManualThumbnailGeneration>();
            generation->request = std::move(request);
            generation->callback = std::move(callback);

            KiriView::ImageIoJob job
                = KiriView::TestSupport::Detail::startManualIoJob(receiver, generation);
            m_generations.push_back(generation);
            return job;
        };
    }

    std::size_t generationCount() const { return m_generations.size(); }

    ManualThumbnailGeneration &generationAt(std::size_t index) { return *m_generations.at(index); }

    void finish(std::size_t index, KiriView::ThumbnailGenerationResult result)
    {
        KiriView::TestSupport::Detail::finishManualIoJob(m_generations.at(index),
            [result = std::move(result)](ManualThumbnailGeneration &generation) mutable {
                if (generation.callback) {
                    generation.callback(std::move(result));
                }
            });
    }

    void deliverIgnoringCancellation(std::size_t index, KiriView::ThumbnailGenerationResult result)
    {
        ManualThumbnailGeneration &generation = generationAt(index);
        if (generation.callback) {
            generation.callback(std::move(result));
        }
    }

private:
    std::vector<std::shared_ptr<ManualThumbnailGeneration>> m_generations;
};

QImage testThumbnailImage(QColor color = Qt::red)
{
    QImage image(QSize(2, 1), QImage::Format_RGBA8888);
    image.fill(color);
    return image;
}

KiriView::ThumbnailCacheLookupResult lookupResult(
    KiriView::ThumbnailCacheLookupStatus status, QImage image = {})
{
    return KiriView::ThumbnailCacheLookupResult {
        status,
        std::move(image),
        KiriView::ActiveNavigationThumbnailDemandBucket::Normal,
        KiriView::ActiveNavigationThumbnailDemandBucket::Normal,
        QStringLiteral("/cache/thumbnail.png"),
        {},
    };
}

KiriView::ThumbnailGenerationResult generationResult(
    KiriView::ThumbnailGenerationStatus status, QImage image = {})
{
    return KiriView::ThumbnailGenerationResult {
        status,
        std::move(image),
        KiriView::ActiveNavigationThumbnailDemandBucket::Normal,
        QStringLiteral("/cache/generated.png"),
        {},
    };
}
}

class TestActiveNavigationThumbnailRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void buildsSourceKeysAndBumpsGenerationOnlyForScopeChanges();
    void sameRowDemandCoalescesButBucketAndPriorityChangesAreAccepted();
    void multipleRowsKeepIndependentDemandState();
    void unsupportedRowsProjectThroughModelResultPath();
    void staleCompletionIsRejected();
    void rowResetRejectsOlderCompletion();
    void activeJobsCancelOnIdentityChangeAndSupersedingDemand();
    void readyLookupPublishesImageProviderSourceAndStoresImage();
    void missingLookupStartsGenerationAndPublishesGeneratedImage();
    void cacheHitSkipsGeneration();
    void lookupInvalidAndFailedSkipGenerationAndPublishFallbackResults();
    void generationFailurePublishesFallbackResult();
    void nonLocalDirectImageIsUnsupportedWithoutLookup();
    void staleLookupCompletionIsRejectedAndReleasesInsertedImage();
    void staleGenerationCompletionIsRejected();
    void independentRowsDoNotOverwriteGeneratedResults();
};

void TestActiveNavigationThumbnailRuntime::buildsSourceKeysAndBumpsGenerationOnlyForScopeChanges()
{
    QObject owner;
    ManualThumbnailLookupProvider provider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.mp4"));

    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.mp4"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo),
    });

    QCOMPARE(runtime.navigationGeneration(), quint64(1));
    QCOMPARE(runtime.sourceKeyAt(0).number, 1);
    QCOMPARE(runtime.sourceKeyAt(0).url, firstUrl);
    QCOMPARE(runtime.sourceKeyAt(0).label, QStringLiteral("01.png"));
    QCOMPARE(runtime.sourceKeyAt(0).sourceKind,
        KiriView::ActiveNavigationThumbnailSourceKind::DirectImage);
    QCOMPARE(runtime.sourceKeyAt(0).navigationGeneration, quint64(1));

    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage),
        thumbnailRow(2, secondUrl, QStringLiteral("02.mp4"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
    });
    QCOMPARE(runtime.navigationGeneration(), quint64(1));

    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("renamed.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage),
        thumbnailRow(2, secondUrl, QStringLiteral("02.mp4"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
    });
    QCOMPARE(runtime.navigationGeneration(), quint64(2));
    QCOMPARE(runtime.sourceKeyAt(0).label, QStringLiteral("renamed.png"));
    QCOMPARE(runtime.sourceKeyAt(0).navigationGeneration, quint64(2));
}

void TestActiveNavigationThumbnailRuntime::
    sameRowDemandCoalescesButBucketAndPriorityChangesAreAccepted()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(!runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    QCOMPARE(runtime.resultAt(0).status, KiriView::ActiveNavigationThumbnailResultStatus::Pending);

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));
    QVERIFY(!runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Nearby, generation));
    QVERIFY(!runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Nearby, generation));
}

void TestActiveNavigationThumbnailRuntime::multipleRowsKeepIndependentDemandState()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(!runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(runtime.reportDemand(2, secondUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(!runtime.reportDemand(2, secondUrl, Bucket::Normal, Priority::Visible, generation));
}

void TestActiveNavigationThumbnailRuntime::unsupportedRowsProjectThroughModelResultPath()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl videoUrl = localUrl(QStringLiteral("/media/clip.mp4"));
    const QUrl pageUrl = localUrl(QStringLiteral("/archive/01.png"));
    runtime.setRows({
        thumbnailRow(1, videoUrl, QStringLiteral("clip.mp4"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
        thumbnailRow(2, pageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::ImageDocumentPageImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, videoUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(runtime.reportDemand(2, pageUrl, Bucket::Normal, Priority::Visible, generation));

    QAbstractItemModel *model = runtime.model();
    QVERIFY(model != nullptr);
    QCOMPARE(modelData(*model, 0, QByteArrayLiteral("thumbnailStatus")).toInt(),
        static_cast<int>(KiriView::ActiveNavigationThumbnailResultStatus::Unsupported));
    QCOMPARE(modelData(*model, 1, QByteArrayLiteral("thumbnailStatus")).toInt(),
        static_cast<int>(KiriView::ActiveNavigationThumbnailResultStatus::Unsupported));
    QCOMPARE(modelData(*model, 0, QByteArrayLiteral("thumbnailImageSource")).toUrl(), QUrl());
}

void TestActiveNavigationThumbnailRuntime::staleCompletionIsRejected()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));

    const KiriView::ActiveNavigationThumbnailSourceKey key = runtime.sourceKeyAt(0);
    KiriView::ActiveNavigationThumbnailCompletion completion {
        key,
        Bucket::Large,
        KiriView::ActiveNavigationThumbnailResult {
            Status::Ready,
            QUrl(QStringLiteral("image://kiriview-thumbnails/current")),
        },
    };

    KiriView::ActiveNavigationThumbnailCompletion stale = completion;
    stale.sourceKey.url = localUrl(QStringLiteral("/media/other.png"));
    QVERIFY(!runtime.applyCompletion(stale));

    stale = completion;
    stale.sourceKey.number = 2;
    QVERIFY(!runtime.applyCompletion(stale));

    stale = completion;
    stale.sourceKey.sourceKind = KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo;
    QVERIFY(!runtime.applyCompletion(stale));

    stale = completion;
    stale.bucket = Bucket::Normal;
    QVERIFY(!runtime.applyCompletion(stale));

    stale = completion;
    stale.sourceKey.navigationGeneration = generation + 1;
    QVERIFY(!runtime.applyCompletion(stale));

    QCOMPARE(runtime.resultAt(0).status, Status::Pending);
    QVERIFY(runtime.applyCompletion(completion));
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(runtime.resultAt(0).imageSource, completion.result.imageSource);
}

void TestActiveNavigationThumbnailRuntime::rowResetRejectsOlderCompletion()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    const KiriView::ActiveNavigationThumbnailSourceKey staleKey = runtime.sourceKeyAt(0);

    runtime.setRows({
        thumbnailRow(1, secondUrl, QStringLiteral("02.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    QCOMPARE(runtime.navigationGeneration(), quint64(2));
    QCOMPARE(runtime.resultAt(0).status, Status::NoResult);

    QVERIFY(!runtime.applyCompletion(KiriView::ActiveNavigationThumbnailCompletion {
        staleKey,
        Bucket::Normal,
        KiriView::ActiveNavigationThumbnailResult {
            Status::Ready,
            QUrl(QStringLiteral("image://kiriview-thumbnails/stale")),
        },
    }));
    QCOMPARE(runtime.resultAt(0).status, Status::NoResult);
}

void TestActiveNavigationThumbnailRuntime::activeJobsCancelOnIdentityChangeAndSupersedingDemand()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    QCOMPARE(runtime.activeJobCount(), qsizetype(1));
    QCOMPARE(runtime.canceledJobCount(), qsizetype(0));

    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Large, Priority::Visible, runtime.navigationGeneration()));
    QCOMPARE(runtime.activeJobCount(), qsizetype(1));
    QCOMPARE(runtime.canceledJobCount(), qsizetype(1));

    runtime.setRows({
        thumbnailRow(1, secondUrl, QStringLiteral("02.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    QCOMPARE(runtime.activeJobCount(), qsizetype(0));
    QCOMPARE(runtime.canceledJobCount(), qsizetype(2));

    ManualThumbnailGenerationProvider generationProvider;
    KiriView::ActiveNavigationThumbnailRuntime generationRuntime(
        &owner, provider.provider(), {}, generationProvider.provider());
    generationRuntime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    QVERIFY(generationRuntime.reportDemand(
        1, firstUrl, Bucket::Normal, Priority::Visible, generationRuntime.navigationGeneration()));
    provider.finish(2, lookupResult(KiriView::ThumbnailCacheLookupStatus::Missing));
    QCOMPARE(generationProvider.generationCount(), std::size_t(1));
    QCOMPARE(generationRuntime.activeJobCount(), qsizetype(1));

    QVERIFY(generationRuntime.reportDemand(
        1, firstUrl, Bucket::Large, Priority::Visible, generationRuntime.navigationGeneration()));
    QCOMPARE(generationProvider.generationAt(0).canceled, true);
    QCOMPARE(generationRuntime.activeJobCount(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::readyLookupPublishesImageProviderSourceAndStoresImage()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    auto store = std::make_shared<KiriView::ThumbnailImageStore>();
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider(), store);
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, imageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    QCOMPARE(provider.lookupCount(), std::size_t(1));
    QCOMPARE(provider.lookupAt(0).request.localPathBytes, QByteArrayLiteral("/media/01.png"));
    QCOMPARE(provider.lookupAt(0).request.requestedBucket, Bucket::Normal);

    provider.finish(0,
        lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::green)));

    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QVERIFY(runtime.resultAt(0).imageSource.toString().startsWith(
        QStringLiteral("image://kiriview-thumbnails/")));
    QCOMPARE(store->size(), qsizetype(1));

    KiriView::ThumbnailImageProvider imageProvider(store);
    QSize imageSize;
    const QString id = runtime.resultAt(0).imageSource.path().mid(1);
    const QImage image = imageProvider.requestImage(id, &imageSize, {});
    QCOMPARE(imageSize, QSize(2, 1));
    QCOMPARE(image.size(), QSize(2, 1));
    QCOMPARE(image.pixelColor(0, 0), QColor(Qt::green));
}

void TestActiveNavigationThumbnailRuntime::missingLookupStartsGenerationAndPublishesGeneratedImage()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<KiriView::ThumbnailImageStore>();
    KiriView::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), store, generationProvider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    const quint64 generation = runtime.navigationGeneration();

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Missing));
    QCOMPARE(runtime.resultAt(0).status, Status::Pending);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
    QCOMPARE(generationProvider.generationCount(), std::size_t(1));
    QCOMPARE(generationProvider.generationAt(0).request.localPathBytes,
        QByteArrayLiteral("/media/01.png"));
    QCOMPARE(generationProvider.generationAt(0).request.requestedBucket, Bucket::Normal);

    generationProvider.finish(0,
        generationResult(KiriView::ThumbnailGenerationStatus::Ready, testThumbnailImage(Qt::blue)));
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QVERIFY(runtime.resultAt(0).imageSource.toString().startsWith(
        QStringLiteral("image://kiriview-thumbnails/")));
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::cacheHitSkipsGeneration()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), {}, generationProvider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, imageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    provider.finish(0,
        lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::green)));

    QCOMPARE(generationProvider.generationCount(), std::size_t(0));
}

void TestActiveNavigationThumbnailRuntime::
    lookupInvalidAndFailedSkipGenerationAndPublishFallbackResults()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<KiriView::ThumbnailImageStore>();
    KiriView::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), store, generationProvider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    const quint64 generation = runtime.navigationGeneration();

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));
    provider.finish(0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Invalid));
    QCOMPARE(runtime.resultAt(0).status, Status::Failed);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::XLarge, Priority::Visible, generation));
    provider.finish(1, lookupResult(KiriView::ThumbnailCacheLookupStatus::Failed));
    QCOMPARE(runtime.resultAt(0).status, Status::Failed);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
    QCOMPARE(store->size(), qsizetype(0));
    QCOMPARE(generationProvider.generationCount(), std::size_t(0));
}

void TestActiveNavigationThumbnailRuntime::generationFailurePublishesFallbackResult()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), {}, generationProvider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, imageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    provider.finish(0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Missing));
    generationProvider.finish(0, generationResult(KiriView::ThumbnailGenerationStatus::Failed));

    QCOMPARE(runtime.resultAt(0).status, Status::Failed);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
}

void TestActiveNavigationThumbnailRuntime::nonLocalDirectImageIsUnsupportedWithoutLookup()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl imageUrl(QStringLiteral("https://example.invalid/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, imageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(provider.lookupCount(), std::size_t(0));
    QCOMPARE(runtime.resultAt(0).status, Status::Unsupported);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
}

void TestActiveNavigationThumbnailRuntime::staleLookupCompletionIsRejectedAndReleasesInsertedImage()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    auto store = std::make_shared<KiriView::ThumbnailImageStore>();
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider(), store);
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    QCOMPARE(provider.lookupCount(), std::size_t(1));

    runtime.setRows({
        thumbnailRow(1, secondUrl, QStringLiteral("02.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    provider.deliverIgnoringCancellation(
        0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));

    QCOMPARE(runtime.resultAt(0).status, Status::NoResult);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
    QCOMPARE(store->size(), qsizetype(0));
}

void TestActiveNavigationThumbnailRuntime::staleGenerationCompletionIsRejected()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<KiriView::ThumbnailImageStore>();
    KiriView::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), store, generationProvider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    provider.finish(0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Missing));

    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Large, Priority::Visible, runtime.navigationGeneration()));
    generationProvider.deliverIgnoringCancellation(
        0, generationResult(KiriView::ThumbnailGenerationStatus::Ready, testThumbnailImage()));
    QCOMPARE(runtime.resultAt(0).status, Status::Pending);
    QCOMPARE(store->size(), qsizetype(0));

    provider.finish(1, lookupResult(KiriView::ThumbnailCacheLookupStatus::Missing));
    runtime.setRows({
        thumbnailRow(1, secondUrl, QStringLiteral("02.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    generationProvider.deliverIgnoringCancellation(
        1, generationResult(KiriView::ThumbnailGenerationStatus::Ready, testThumbnailImage()));
    QCOMPARE(runtime.resultAt(0).status, Status::NoResult);
    QCOMPARE(store->size(), qsizetype(0));
}

void TestActiveNavigationThumbnailRuntime::independentRowsDoNotOverwriteGeneratedResults()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<KiriView::ThumbnailImageStore>();
    KiriView::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), store, generationProvider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(runtime.reportDemand(2, secondUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Missing));
    provider.finish(1, lookupResult(KiriView::ThumbnailCacheLookupStatus::Missing));

    generationProvider.finish(1,
        generationResult(KiriView::ThumbnailGenerationStatus::Ready, testThumbnailImage(Qt::cyan)));
    QCOMPARE(runtime.resultAt(0).status, Status::Pending);
    QCOMPARE(runtime.resultAt(1).status, Status::Ready);

    generationProvider.finish(0,
        generationResult(
            KiriView::ThumbnailGenerationStatus::Ready, testThumbnailImage(Qt::magenta)));
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(runtime.resultAt(1).status, Status::Ready);
    QVERIFY(runtime.resultAt(0).imageSource != runtime.resultAt(1).imageSource);
    QCOMPARE(store->size(), qsizetype(2));
}

QTEST_GUILESS_MAIN(TestActiveNavigationThumbnailRuntime)

#include "test_activenavigationthumbnailruntime.moc"
