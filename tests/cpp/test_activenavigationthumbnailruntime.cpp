// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/imageworkerscheduler.h"
#include "image_async_test_support.h"
#include "image_test_support.h"
#include "session/activenavigationthumbnailruntime.h"

#include <QAbstractItemModel>
#include <QColor>
#include <QImage>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <QtGlobal>
#include <utility>
#include <vector>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::localUrl;

struct ManualImageWorkerSchedule {
    KiriView::ImageWorkerOperation work;
    KiriView::ImageWorkerCompletion completion;
};

class ManualImageWorkerScheduler
{
public:
    KiriView::ImageWorkerScheduler scheduler()
    {
        return KiriView::ImageWorkerScheduler([this](QObject *, KiriView::ImageWorkerOperation work,
                                                  KiriView::ImageWorkerCompletion completion) {
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

QString thumbnailImageStoreId(const QUrl &source) { return source.path().mid(1); }

KiriView::ThumbnailCacheLookupResult lookupResult(KiriView::ThumbnailCacheLookupStatus status,
    QImage image = {},
    KiriView::ActiveNavigationThumbnailDemandBucket requestedBucket
    = KiriView::ActiveNavigationThumbnailDemandBucket::Normal)
{
    return KiriView::ThumbnailCacheLookupResult {
        status,
        std::move(image),
        requestedBucket,
        requestedBucket,
        QStringLiteral("/cache/thumbnail.png"),
        {},
    };
}

KiriView::ThumbnailGenerationResult generationResult(KiriView::ThumbnailGenerationStatus status,
    QImage image = {},
    KiriView::ActiveNavigationThumbnailDemandBucket requestedBucket
    = KiriView::ActiveNavigationThumbnailDemandBucket::Normal)
{
    return KiriView::ThumbnailGenerationResult {
        status,
        std::move(image),
        requestedBucket,
        QStringLiteral("/cache/generated.png"),
        {},
    };
}

KiriView::ThumbnailSourceAdapterPlan sourceAdapterPlan(
    KiriView::ThumbnailSourceAdapterPlanKind kind, QByteArray localPathBytes = {})
{
    return KiriView::ThumbnailSourceAdapterPlan {
        kind,
        std::move(localPathBytes),
    };
}

class RecordingThumbnailSourceAdapter
{
public:
    KiriView::ThumbnailSourceAdapter adapter()
    {
        return [this](KiriView::ThumbnailSourceAdapterRequest request) {
            requests.push_back(request);
            return plan;
        };
    }

    KiriView::ThumbnailSourceAdapterPlan plan
        = sourceAdapterPlan(KiriView::ThumbnailSourceAdapterPlanKind::Unsupported);
    std::vector<KiriView::ThumbnailSourceAdapterRequest> requests;
};
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
    void largerBucketDemandKeepsReadyThumbnailWhileLookupIsPending();
    void newerReadyResultReplacesOldThumbnailAndReleasesOldStoreEntry();
    void lookupOrGenerationFailureKeepsExistingReadyThumbnail();
    void lookupInvalidAndFailedSkipGenerationAndPublishFallbackResults();
    void generationFailurePublishesFallbackResult();
    void nonLocalDirectImageIsUnsupportedWithoutLookup();
    void defaultDirectVideoIsUnsupportedWithoutAdapter();
    void staleLookupCompletionIsRejectedAndReleasesInsertedImage();
    void staleGenerationCompletionIsRejected();
    void independentRowsDoNotOverwriteGeneratedResults();
    void backgroundFillStartsOnlyAfterForegroundJobsDrain();
    void backgroundFillRunsNormalBeforeLargerBuckets();
    void foregroundDemandCancelsActiveBackgroundWork();
    void backgroundFillSkipsUnsupportedAndNonLocalRows();
    void staleCanceledBackgroundCompletionsAreRejected();
    void backgroundReadyResultDoesNotReplaceExistingReadyThumbnail();
    void injectedUnsupportedAdapterSkipsLookupAndGeneration();
    void injectedDirectVideoAdapterUsesGeneratedResultPath();
    void injectedCollectionAdapterUsesGeneratedResultPath();
    void openedCollectionEntryAdapterStartsGenerationWithoutPrelookup();
    void inMemoryOnlyAdapterSkipsLookupAndDisablesCacheInstall();
    void defaultGenerationProviderUsesWorkerScheduler();
    void defaultDirectLocalImageBehaviorStillUsesLookup();
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
    largerBucketDemandKeepsReadyThumbnailWhileLookupIsPending()
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
    const quint64 generation = runtime.navigationGeneration();

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(0,
        lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::green)));
    const QUrl previousSource = runtime.resultAt(0).imageSource;
    QVERIFY(!previousSource.isEmpty());

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(runtime.resultAt(0).imageSource, previousSource);
    QCOMPARE(runtime.activeJobCount(), qsizetype(1));
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::
    newerReadyResultReplacesOldThumbnailAndReleasesOldStoreEntry()
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
    const quint64 generation = runtime.navigationGeneration();

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(0,
        lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::green)));
    const QUrl previousSource = runtime.resultAt(0).imageSource;
    const QString previousId = thumbnailImageStoreId(previousSource);

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));
    QCOMPARE(provider.lookupAt(1).canceled, true);
    QCOMPARE(provider.lookupCount(), std::size_t(3));
    provider.finish(
        2, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::blue)));

    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QVERIFY(runtime.resultAt(0).imageSource != previousSource);
    QCOMPARE(store->size(), qsizetype(1));
    QVERIFY(store->image(previousId).isNull());
    const QImage replacement = store->image(thumbnailImageStoreId(runtime.resultAt(0).imageSource));
    QCOMPARE(replacement.pixelColor(0, 0), QColor(Qt::blue));
}

void TestActiveNavigationThumbnailRuntime::lookupOrGenerationFailureKeepsExistingReadyThumbnail()
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
    provider.finish(0,
        lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::green)));
    const QUrl previousSource = runtime.resultAt(0).imageSource;

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));
    QCOMPARE(provider.lookupAt(1).canceled, true);
    QCOMPARE(provider.lookupCount(), std::size_t(3));
    provider.finish(2, lookupResult(KiriView::ThumbnailCacheLookupStatus::Failed));
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(runtime.resultAt(0).imageSource, previousSource);

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::XLarge, Priority::Visible, generation));
    QCOMPARE(provider.lookupAt(3).canceled, true);
    QCOMPARE(provider.lookupCount(), std::size_t(5));
    provider.finish(4, lookupResult(KiriView::ThumbnailCacheLookupStatus::Missing));
    generationProvider.finish(0, generationResult(KiriView::ThumbnailGenerationStatus::Failed));
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(runtime.resultAt(0).imageSource, previousSource);
    QCOMPARE(store->size(), qsizetype(1));
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
    QCOMPARE(provider.lookupAt(1).canceled, true);
    QCOMPARE(provider.lookupCount(), std::size_t(3));
    provider.finish(2, lookupResult(KiriView::ThumbnailCacheLookupStatus::Failed));
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

void TestActiveNavigationThumbnailRuntime::defaultDirectVideoIsUnsupportedWithoutAdapter()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(
        &owner, lookupProvider.provider(), {}, generationProvider.provider());
    const QUrl videoUrl = localUrl(QStringLiteral("/media/clip.mp4"));
    runtime.setRows({
        thumbnailRow(1, videoUrl, QStringLiteral("clip.mp4"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
    });

    QVERIFY(runtime.reportDemand(
        1, videoUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(lookupProvider.lookupCount(), std::size_t(0));
    QCOMPARE(generationProvider.generationCount(), std::size_t(0));
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

void TestActiveNavigationThumbnailRuntime::backgroundFillStartsOnlyAfterForegroundJobsDrain()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/media/03.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage),
        thumbnailRow(3, thirdUrl, QStringLiteral("03.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(runtime.reportDemand(2, secondUrl, Bucket::Normal, Priority::Nearby, generation));
    QCOMPARE(provider.lookupCount(), std::size_t(2));

    provider.finish(
        0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));
    QCOMPARE(provider.lookupCount(), std::size_t(2));

    provider.finish(
        1, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));
    QCOMPARE(provider.lookupCount(), std::size_t(3));
    QCOMPARE(provider.lookupAt(2).request.localPathBytes, QByteArrayLiteral("/media/03.png"));
    QCOMPARE(provider.lookupAt(2).request.requestedBucket, Bucket::Normal);
}

void TestActiveNavigationThumbnailRuntime::backgroundFillRunsNormalBeforeLargerBuckets()
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
    provider.finish(
        0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));

    QCOMPARE(provider.lookupCount(), std::size_t(2));
    QCOMPARE(provider.lookupAt(1).request.localPathBytes, QByteArrayLiteral("/media/02.png"));
    QCOMPARE(provider.lookupAt(1).request.requestedBucket, Bucket::Normal);

    provider.finish(
        1, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));
    QCOMPARE(provider.lookupCount(), std::size_t(3));
    QCOMPARE(provider.lookupAt(2).request.localPathBytes, QByteArrayLiteral("/media/01.png"));
    QCOMPARE(provider.lookupAt(2).request.requestedBucket, Bucket::Large);

    provider.finish(
        2, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));
    QCOMPARE(provider.lookupCount(), std::size_t(4));
    QCOMPARE(provider.lookupAt(3).request.localPathBytes, QByteArrayLiteral("/media/02.png"));
    QCOMPARE(provider.lookupAt(3).request.requestedBucket, Bucket::Large);
}

void TestActiveNavigationThumbnailRuntime::foregroundDemandCancelsActiveBackgroundWork()
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
    provider.finish(
        0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));
    QCOMPARE(provider.lookupCount(), std::size_t(2));

    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Large, Priority::Visible, generation));
    QCOMPARE(provider.lookupAt(1).canceled, true);
    QCOMPARE(provider.lookupCount(), std::size_t(3));
    QCOMPARE(provider.lookupAt(2).request.localPathBytes, QByteArrayLiteral("/media/01.png"));
    QCOMPARE(provider.lookupAt(2).request.requestedBucket, Bucket::Large);
}

void TestActiveNavigationThumbnailRuntime::backgroundFillSkipsUnsupportedAndNonLocalRows()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl remoteUrl(QStringLiteral("https://example.invalid/03.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, videoUrl, QStringLiteral("02.mp4"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo),
        thumbnailRow(3, remoteUrl, QStringLiteral("03.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    provider.finish(
        0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));

    QCOMPARE(provider.lookupCount(), std::size_t(2));
    QCOMPARE(provider.lookupAt(1).request.localPathBytes, QByteArrayLiteral("/media/01.png"));
    QCOMPARE(provider.lookupAt(1).request.requestedBucket, Bucket::Large);
}

void TestActiveNavigationThumbnailRuntime::staleCanceledBackgroundCompletionsAreRejected()
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
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(
        0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));
    QCOMPARE(provider.lookupCount(), std::size_t(2));

    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Large, Priority::Visible, generation));
    provider.deliverIgnoringCancellation(
        1, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::blue)));

    QCOMPARE(runtime.resultAt(1).status, Status::NoResult);
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::
    backgroundReadyResultDoesNotReplaceExistingReadyThumbnail()
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
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(
        0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::red)));
    QCOMPARE(provider.lookupCount(), std::size_t(2));
    provider.finish(
        1, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::blue)));
    QCOMPARE(runtime.resultAt(1).status, Status::Ready);
    const QUrl backgroundSource = runtime.resultAt(1).imageSource;

    QVERIFY(runtime.reportDemand(2, secondUrl, Bucket::Normal, Priority::Visible, generation));
    const QUrl foregroundSource = runtime.resultAt(1).imageSource;
    QCOMPARE(foregroundSource, backgroundSource);
    QCOMPARE(provider.lookupAt(2).canceled, true);
    QCOMPARE(provider.lookupCount(), std::size_t(4));
    provider.finish(3,
        lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::green)));
    QCOMPARE(runtime.resultAt(1).status, Status::Ready);
    QVERIFY(runtime.resultAt(1).imageSource != foregroundSource);
    const QUrl foregroundReadySource = runtime.resultAt(1).imageSource;

    while (provider.lookupCount() < 6) {
        provider.finish(provider.lookupCount() - 1,
            lookupResult(
                KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::yellow)));
    }

    QCOMPARE(provider.lookupAt(5).request.localPathBytes, QByteArrayLiteral("/media/02.png"));
    QCOMPARE(provider.lookupAt(5).request.requestedBucket, Bucket::Large);
    provider.finish(
        5, lookupResult(KiriView::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::cyan)));

    QCOMPARE(runtime.resultAt(1).status, Status::Ready);
    QCOMPARE(runtime.resultAt(1).imageSource, foregroundReadySource);
}

void TestActiveNavigationThumbnailRuntime::injectedUnsupportedAdapterSkipsLookupAndGeneration()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    RecordingThumbnailSourceAdapter sourceAdapter;
    sourceAdapter.plan = sourceAdapterPlan(KiriView::ThumbnailSourceAdapterPlanKind::Unsupported);
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, lookupProvider.provider(), {},
        generationProvider.provider(), sourceAdapter.adapter());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/unsupported.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("unsupported.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, imageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(sourceAdapter.requests.size(), std::size_t(1));
    QCOMPARE(sourceAdapter.requests.at(0).sourceKey.sourceKind,
        KiriView::ActiveNavigationThumbnailSourceKind::DirectImage);
    QCOMPARE(sourceAdapter.requests.at(0).requestedBucket, Bucket::Normal);
    QCOMPARE(lookupProvider.lookupCount(), std::size_t(0));
    QCOMPARE(generationProvider.generationCount(), std::size_t(0));
    QCOMPARE(runtime.resultAt(0).status, Status::Unsupported);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
}

void TestActiveNavigationThumbnailRuntime::injectedDirectVideoAdapterUsesGeneratedResultPath()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<KiriView::ThumbnailImageStore>();
    RecordingThumbnailSourceAdapter sourceAdapter;
    sourceAdapter.plan
        = sourceAdapterPlan(KiriView::ThumbnailSourceAdapterPlanKind::CacheableLocalFile,
            QByteArrayLiteral("/media/clip.mp4"));
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, lookupProvider.provider(), store,
        generationProvider.provider(), sourceAdapter.adapter());
    const QUrl videoUrl = localUrl(QStringLiteral("/media/clip.mp4"));
    runtime.setRows({
        thumbnailRow(1, videoUrl, QStringLiteral("clip.mp4"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
    });

    QVERIFY(runtime.reportDemand(
        1, videoUrl, Bucket::Large, Priority::Visible, runtime.navigationGeneration()));
    QCOMPARE(lookupProvider.lookupCount(), std::size_t(1));
    QCOMPARE(
        lookupProvider.lookupAt(0).request.localPathBytes, QByteArrayLiteral("/media/clip.mp4"));
    lookupProvider.finish(0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Missing));

    QCOMPARE(generationProvider.generationCount(), std::size_t(1));
    QCOMPARE(generationProvider.generationAt(0).request.localPathBytes,
        QByteArrayLiteral("/media/clip.mp4"));
    QCOMPARE(generationProvider.generationAt(0).request.sourceKind,
        KiriView::ThumbnailSourceKind::DirectVideo);
    QCOMPARE(generationProvider.generationAt(0).request.cacheInstallEnabled, true);
    generationProvider.finish(0,
        generationResult(KiriView::ThumbnailGenerationStatus::Ready, testThumbnailImage(Qt::blue),
            Bucket::Large));

    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QVERIFY(runtime.resultAt(0).imageSource.toString().startsWith(
        QStringLiteral("image://kiriview-thumbnails/")));
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::injectedCollectionAdapterUsesGeneratedResultPath()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<KiriView::ThumbnailImageStore>();
    RecordingThumbnailSourceAdapter sourceAdapter;
    sourceAdapter.plan
        = sourceAdapterPlan(KiriView::ThumbnailSourceAdapterPlanKind::CacheableLocalFile,
            QByteArrayLiteral("/archive/pages/001.png"));
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, lookupProvider.provider(), store,
        generationProvider.provider(), sourceAdapter.adapter());
    const QUrl pageUrl = localUrl(QStringLiteral("/archive/book.cbz#001"));
    runtime.setRows({
        thumbnailRow(1, pageUrl, QStringLiteral("001.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::ImageDocumentPageImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, pageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    lookupProvider.finish(0, lookupResult(KiriView::ThumbnailCacheLookupStatus::Missing));
    generationProvider.finish(0,
        generationResult(KiriView::ThumbnailGenerationStatus::Ready, testThumbnailImage(Qt::cyan)));

    QCOMPARE(lookupProvider.lookupAt(0).request.localPathBytes,
        QByteArrayLiteral("/archive/pages/001.png"));
    QCOMPARE(generationProvider.generationAt(0).request.sourceKind,
        KiriView::ThumbnailSourceKind::ImageDocumentPageImage);
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::
    openedCollectionEntryAdapterStartsGenerationWithoutPrelookup()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<KiriView::ThumbnailImageStore>();
    const KiriView::OpenedCollectionScopeLocation cbzScope
        = KiriView::OpenedCollectionScopeLocation::fromUrls(
            localUrl(QStringLiteral("/books/book.cbz")),
            QUrl(QStringLiteral("zip:///books/book.cbz/")),
            KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    RecordingThumbnailSourceAdapter sourceAdapter;
    sourceAdapter.plan.kind
        = KiriView::ThumbnailSourceAdapterPlanKind::CacheableOpenedCollectionEntry;
    sourceAdapter.plan.openedCollectionScope = cbzScope;
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, lookupProvider.provider(), store,
        generationProvider.provider(), sourceAdapter.adapter());
    const QUrl pageUrl = archivePageUrl(cbzScope.rootUrl(), QStringLiteral("pages/001.png"));
    runtime.setRows({
        thumbnailRow(1, pageUrl, QStringLiteral("pages/001.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::ImageDocumentPageImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, pageUrl, Bucket::Large, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(lookupProvider.lookupCount(), std::size_t(0));
    QCOMPARE(generationProvider.generationCount(), std::size_t(1));
    QCOMPARE(generationProvider.generationAt(0).request.openedCollectionScope, cbzScope);
    QCOMPARE(generationProvider.generationAt(0).request.cacheInstallEnabled, true);
    generationProvider.finish(0,
        generationResult(KiriView::ThumbnailGenerationStatus::Ready, testThumbnailImage(Qt::cyan),
            Bucket::Large));

    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::inMemoryOnlyAdapterSkipsLookupAndDisablesCacheInstall()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<KiriView::ThumbnailImageStore>();
    RecordingThumbnailSourceAdapter sourceAdapter;
    sourceAdapter.plan = sourceAdapterPlan(KiriView::ThumbnailSourceAdapterPlanKind::InMemoryOnly,
        QByteArrayLiteral("archive-entry:001"));
    KiriView::ActiveNavigationThumbnailRuntime runtime(&owner, lookupProvider.provider(), store,
        generationProvider.provider(), sourceAdapter.adapter());
    const QUrl pageUrl = localUrl(QStringLiteral("/archive/book.cbz#001"));
    runtime.setRows({
        thumbnailRow(1, pageUrl, QStringLiteral("001.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::ImageDocumentPageImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, pageUrl, Bucket::XLarge, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(lookupProvider.lookupCount(), std::size_t(0));
    QCOMPARE(generationProvider.generationCount(), std::size_t(1));
    QCOMPARE(generationProvider.generationAt(0).request.localPathBytes,
        QByteArrayLiteral("archive-entry:001"));
    QCOMPARE(generationProvider.generationAt(0).request.cacheInstallEnabled, false);
    generationProvider.finish(0,
        generationResult(KiriView::ThumbnailGenerationStatus::Ready,
            testThumbnailImage(Qt::magenta), Bucket::XLarge));

    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::defaultDirectLocalImageBehaviorStillUsesLookup()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    KiriView::ActiveNavigationThumbnailRuntime runtime(
        &owner, lookupProvider.provider(), {}, generationProvider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, imageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(lookupProvider.lookupCount(), std::size_t(1));
    QCOMPARE(lookupProvider.lookupAt(0).request.localPathBytes, QByteArrayLiteral("/media/01.png"));
    QCOMPARE(generationProvider.generationCount(), std::size_t(0));
}

void TestActiveNavigationThumbnailRuntime::defaultGenerationProviderUsesWorkerScheduler()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualImageWorkerScheduler workerScheduler;
    RecordingThumbnailSourceAdapter sourceAdapter;
    sourceAdapter.plan = sourceAdapterPlan(KiriView::ThumbnailSourceAdapterPlanKind::InMemoryOnly);
    KiriView::ActiveNavigationThumbnailRuntime runtime(
        &owner, {}, {}, {}, sourceAdapter.adapter(), workerScheduler.scheduler());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, imageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(workerScheduler.scheduleCount(), std::size_t(1));
    QCOMPARE(runtime.activeJobCount(), qsizetype(1));

    runtime.setRows({});
    workerScheduler.runWork(0);
    workerScheduler.finish(0);

    QCOMPARE(runtime.activeJobCount(), qsizetype(0));
}

QTEST_GUILESS_MAIN(TestActiveNavigationThumbnailRuntime)

#include "test_activenavigationthumbnailruntime.moc"
