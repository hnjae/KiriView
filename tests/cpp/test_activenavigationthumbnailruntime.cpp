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
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::localUrl;

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

kiriview::ActiveNavigationThumbnailRow thumbnailRow(int number, const QUrl &url,
    const QString &label, kiriview::ActiveNavigationThumbnailSourceKind sourceKind,
    bool current = false)
{
    return kiriview::ActiveNavigationThumbnailRow {
        number,
        url,
        label,
        sourceKind == kiriview::ActiveNavigationThumbnailSourceKind::DirectVideo
                || sourceKind
                    == kiriview::ActiveNavigationThumbnailSourceKind::ImageDocumentPageVideo
            ? kiriview::ActiveNavigationThumbnailKind::Video
            : kiriview::ActiveNavigationThumbnailKind::Image,
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
    kiriview::ThumbnailCacheLookupRequest request;
    kiriview::ThumbnailCacheLookupCallback callback;
    kiriview::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualThumbnailLookupProvider
{
public:
    kiriview::ThumbnailCacheLookupProvider provider()
    {
        return [this](QObject *receiver, kiriview::ThumbnailCacheLookupRequest request,
                   kiriview::ThumbnailCacheLookupCallback callback) {
            auto lookup = std::make_shared<ManualThumbnailLookup>();
            lookup->request = std::move(request);
            lookup->callback = std::move(callback);

            kiriview::ImageIoJob job
                = kiriview::TestSupport::Detail::startManualIoJob(receiver, lookup);
            m_lookups.push_back(lookup);
            return job;
        };
    }

    std::size_t lookupCount() const { return m_lookups.size(); }

    ManualThumbnailLookup &lookupAt(std::size_t index) { return *m_lookups.at(index); }

    void finish(std::size_t index, kiriview::ThumbnailCacheLookupResult result)
    {
        kiriview::TestSupport::Detail::finishManualIoJob(m_lookups.at(index),
            [result = std::move(result)](ManualThumbnailLookup &lookup) mutable {
                if (lookup.callback) {
                    lookup.callback(std::move(result));
                }
            });
    }

    void deliverIgnoringCancellation(std::size_t index, kiriview::ThumbnailCacheLookupResult result)
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
    kiriview::ThumbnailGenerationRequest request;
    kiriview::ThumbnailGenerationCallback callback;
    kiriview::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualThumbnailGenerationProvider
{
public:
    kiriview::ThumbnailGenerationProvider provider()
    {
        return [this](QObject *receiver, kiriview::ThumbnailGenerationRequest request,
                   kiriview::ThumbnailGenerationCallback callback) {
            auto generation = std::make_shared<ManualThumbnailGeneration>();
            generation->request = std::move(request);
            generation->callback = std::move(callback);

            kiriview::ImageIoJob job
                = kiriview::TestSupport::Detail::startManualIoJob(receiver, generation);
            m_generations.push_back(generation);
            return job;
        };
    }

    std::size_t generationCount() const { return m_generations.size(); }

    ManualThumbnailGeneration &generationAt(std::size_t index) { return *m_generations.at(index); }

    void finish(std::size_t index, kiriview::ThumbnailGenerationResult result)
    {
        kiriview::TestSupport::Detail::finishManualIoJob(m_generations.at(index),
            [result = std::move(result)](ManualThumbnailGeneration &generation) mutable {
                if (generation.callback) {
                    generation.callback(std::move(result));
                }
            });
    }

    void deliverIgnoringCancellation(std::size_t index, kiriview::ThumbnailGenerationResult result)
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

kiriview::ThumbnailCacheLookupResult lookupResult(kiriview::ThumbnailCacheLookupStatus status,
    QImage image = {},
    kiriview::ActiveNavigationThumbnailDemandBucket requestedBucket
    = kiriview::ActiveNavigationThumbnailDemandBucket::Normal,
    QString errorString = {})
{
    return kiriview::ThumbnailCacheLookupResult {
        status,
        std::move(image),
        requestedBucket,
        requestedBucket,
        QStringLiteral("/cache/thumbnail.png"),
        std::move(errorString),
    };
}

kiriview::ThumbnailGenerationResult generationResult(kiriview::ThumbnailGenerationStatus status,
    QImage image = {},
    kiriview::ActiveNavigationThumbnailDemandBucket requestedBucket
    = kiriview::ActiveNavigationThumbnailDemandBucket::Normal,
    QString errorString = {})
{
    return kiriview::ThumbnailGenerationResult {
        status,
        std::move(image),
        requestedBucket,
        QStringLiteral("/cache/generated.png"),
        std::move(errorString),
    };
}

kiriview::ThumbnailSourceAdapterPlan sourceAdapterPlan(
    kiriview::ThumbnailSourceAdapterPlanKind kind, QByteArray localPathBytes = {})
{
    return kiriview::ThumbnailSourceAdapterPlan {
        kind,
        std::move(localPathBytes),
    };
}

class RecordingThumbnailSourceAdapter
{
public:
    kiriview::ThumbnailSourceAdapter adapter()
    {
        return [this](kiriview::ThumbnailSourceAdapterRequest request) {
            requests.push_back(request);
            return plan;
        };
    }

    kiriview::ThumbnailSourceAdapterPlan plan
        = sourceAdapterPlan(kiriview::ThumbnailSourceAdapterPlanKind::Unsupported);
    std::vector<kiriview::ThumbnailSourceAdapterRequest> requests;
};
}

class TestActiveNavigationThumbnailRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void buildsSourceKeysAndBumpsGenerationOnlyForScopeChanges();
    void sameRowDemandCoalescesButBucketAndPriorityChangesAreAccepted();
    void repeatedDemandAfterOtherRowDoesNotPlanAgain();
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
    void failureDiagnosticsPreserveSourceAndErrorDetail();
    void generationFailurePublishesFallbackResult();
    void nonLocalDirectImageIsUnsupportedWithoutLookup();
    void nonLocalDirectVideoIsUnsupportedWithoutLookup();
    void defaultDirectLocalVideoUsesCacheLookupFirst();
    void defaultDirectLocalVideoCacheMissStartsVideoGeneration();
    void directVideoGenerationFailurePublishesFallbackAndDiagnostics();
    void staleLookupCompletionIsRejectedAndReleasesInsertedImage();
    void staleGenerationCompletionIsRejected();
    void independentRowsDoNotOverwriteGeneratedResults();
    void backgroundFillStartsOnlyAfterForegroundJobsDrain();
    void backgroundFillRunsNormalBeforeLargerBuckets();
    void foregroundDemandCancelsActiveBackgroundWork();
    void backgroundFillIncludesDirectLocalVideoAndSkipsNonLocalRows();
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
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.mp4"));

    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.mp4"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectVideo),
    });

    QCOMPARE(runtime.navigationGeneration(), quint64(1));
    QCOMPARE(runtime.sourceKeyAt(0).rowNumber, 1);
    QCOMPARE(runtime.sourceKeyAt(0).url, firstUrl);
    QCOMPARE(runtime.sourceKeyAt(0).label, QStringLiteral("01.png"));
    QCOMPARE(runtime.sourceKeyAt(0).pageKind, QStringLiteral("image"));
    QCOMPARE(runtime.sourceKeyAt(0).sourceKind, QStringLiteral("direct-image"));
    QCOMPARE(runtime.sourceKeyAt(0).navigationGeneration, quint64(1));

    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage),
        thumbnailRow(2, secondUrl, QStringLiteral("02.mp4"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
    });
    QCOMPARE(runtime.navigationGeneration(), quint64(1));

    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("renamed.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage),
        thumbnailRow(2, secondUrl, QStringLiteral("02.mp4"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
    });
    QCOMPARE(runtime.navigationGeneration(), quint64(2));
    QCOMPARE(runtime.sourceKeyAt(0).label, QStringLiteral("renamed.png"));
    QCOMPARE(runtime.sourceKeyAt(0).navigationGeneration, quint64(2));
}

void TestActiveNavigationThumbnailRuntime::
    sameRowDemandCoalescesButBucketAndPriorityChangesAreAccepted()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(!runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    QCOMPARE(runtime.resultAt(0).status, kiriview::ActiveNavigationThumbnailResultStatus::Pending);

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));
    QVERIFY(!runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Nearby, generation));
    QVERIFY(!runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Nearby, generation));
}

void TestActiveNavigationThumbnailRuntime::repeatedDemandAfterOtherRowDoesNotPlanAgain()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    RecordingThumbnailSourceAdapter sourceAdapter;
    sourceAdapter.plan = sourceAdapterPlan(kiriview::ThumbnailSourceAdapterPlanKind::InMemoryOnly);
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, lookupProvider.provider(), {},
        generationProvider.provider(), sourceAdapter.adapter());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(runtime.reportDemand(2, secondUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(!runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    QCOMPARE(sourceAdapter.requests.size(), std::size_t(2));
}

void TestActiveNavigationThumbnailRuntime::multipleRowsKeepIndependentDemandState()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(!runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(runtime.reportDemand(2, secondUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(!runtime.reportDemand(2, secondUrl, Bucket::Normal, Priority::Visible, generation));
}

void TestActiveNavigationThumbnailRuntime::unsupportedRowsProjectThroughModelResultPath()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl videoUrl(QStringLiteral("https://example.invalid/clip.mp4"));
    const QUrl pageUrl = localUrl(QStringLiteral("/archive/01.png"));
    runtime.setRows({
        thumbnailRow(1, videoUrl, QStringLiteral("clip.mp4"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
        thumbnailRow(2, pageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::ImageDocumentPageImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, videoUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(runtime.reportDemand(2, pageUrl, Bucket::Normal, Priority::Visible, generation));

    QAbstractItemModel *model = runtime.model();
    QVERIFY(model != nullptr);
    QCOMPARE(modelData(*model, 0, QByteArrayLiteral("thumbnailStatus")).toInt(),
        static_cast<int>(kiriview::ActiveNavigationThumbnailResultStatus::Unsupported));
    QCOMPARE(modelData(*model, 1, QByteArrayLiteral("thumbnailStatus")).toInt(),
        static_cast<int>(kiriview::ActiveNavigationThumbnailResultStatus::Unsupported));
    QCOMPARE(modelData(*model, 0, QByteArrayLiteral("thumbnailImageSource")).toUrl(), QUrl());
}

void TestActiveNavigationThumbnailRuntime::staleCompletionIsRejected()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));

    const kiriview::ThumbnailSourceKey key = runtime.sourceKeyAt(0);
    kiriview::ActiveNavigationThumbnailCompletion completion {
        key,
        Bucket::Large,
        kiriview::ActiveNavigationThumbnailResult {
            Status::Ready,
            QUrl(QStringLiteral("image://kiriview-thumbnails/current")),
        },
    };

    kiriview::ActiveNavigationThumbnailCompletion stale = completion;
    stale.sourceKey = kiriview::thumbnailSourceKey(1, localUrl(QStringLiteral("/media/other.png")),
        QStringLiteral("01.png"), QStringLiteral("image"), QStringLiteral("direct-image"),
        generation);
    QVERIFY(!runtime.applyCompletion(stale));

    stale = completion;
    stale.sourceKey = kiriview::thumbnailSourceKey(2, imageUrl, QStringLiteral("01.png"),
        QStringLiteral("image"), QStringLiteral("direct-image"), generation);
    QVERIFY(!runtime.applyCompletion(stale));

    stale = completion;
    stale.sourceKey = kiriview::thumbnailSourceKey(1, imageUrl, QStringLiteral("01.png"),
        QStringLiteral("image"), QStringLiteral("direct-video"), generation);
    QVERIFY(!runtime.applyCompletion(stale));

    stale = completion;
    stale.bucket = Bucket::Normal;
    QVERIFY(!runtime.applyCompletion(stale));

    stale = completion;
    stale.sourceKey.navigationGeneration = generation + 1;
    QVERIFY(kiriview::sameThumbnailSourceKey(completion.sourceKey, stale.sourceKey));
    QVERIFY(!runtime.applyCompletion(stale));

    QCOMPARE(runtime.resultAt(0).status, Status::Pending);
    QVERIFY(runtime.applyCompletion(completion));
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(runtime.resultAt(0).imageSource, completion.result.imageSource);
}

void TestActiveNavigationThumbnailRuntime::rowResetRejectsOlderCompletion()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    const kiriview::ThumbnailSourceKey staleKey = runtime.sourceKeyAt(0);

    runtime.setRows({
        thumbnailRow(1, secondUrl, QStringLiteral("02.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    QCOMPARE(runtime.navigationGeneration(), quint64(2));
    QCOMPARE(runtime.resultAt(0).status, Status::NoResult);

    QVERIFY(!runtime.applyCompletion(kiriview::ActiveNavigationThumbnailCompletion {
        staleKey,
        Bucket::Normal,
        kiriview::ActiveNavigationThumbnailResult {
            Status::Ready,
            QUrl(QStringLiteral("image://kiriview-thumbnails/stale")),
        },
    }));
    QCOMPARE(runtime.resultAt(0).status, Status::NoResult);
}

void TestActiveNavigationThumbnailRuntime::activeJobsCancelOnIdentityChangeAndSupersedingDemand()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
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
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    QCOMPARE(runtime.activeJobCount(), qsizetype(0));
    QCOMPARE(runtime.canceledJobCount(), qsizetype(2));

    ManualThumbnailGenerationProvider generationProvider;
    kiriview::ActiveNavigationThumbnailRuntime generationRuntime(
        &owner, provider.provider(), {}, generationProvider.provider());
    generationRuntime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    QVERIFY(generationRuntime.reportDemand(
        1, firstUrl, Bucket::Normal, Priority::Visible, generationRuntime.navigationGeneration()));
    provider.finish(2, lookupResult(kiriview::ThumbnailCacheLookupStatus::Missing));
    QCOMPARE(generationProvider.generationCount(), std::size_t(1));
    QCOMPARE(generationRuntime.activeJobCount(), qsizetype(1));

    QVERIFY(generationRuntime.reportDemand(
        1, firstUrl, Bucket::Large, Priority::Visible, generationRuntime.navigationGeneration()));
    QCOMPARE(generationProvider.generationAt(0).canceled, true);
    QCOMPARE(generationRuntime.activeJobCount(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::readyLookupPublishesImageProviderSourceAndStoresImage()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider(), store);
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, imageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    QCOMPARE(provider.lookupCount(), std::size_t(1));
    QCOMPARE(provider.lookupAt(0).request.localPathBytes, QByteArrayLiteral("/media/01.png"));
    QCOMPARE(provider.lookupAt(0).request.requestedBucket, Bucket::Normal);

    provider.finish(0,
        lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::green)));

    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QVERIFY(runtime.resultAt(0).imageSource.toString().startsWith(
        QStringLiteral("image://kiriview-thumbnails/")));
    QCOMPARE(store->size(), qsizetype(1));

    kiriview::ThumbnailImageProvider imageProvider(store);
    QSize imageSize;
    const QString id = runtime.resultAt(0).imageSource.path().mid(1);
    const QImage image = imageProvider.requestImage(id, &imageSize, {});
    QCOMPARE(imageSize, QSize(2, 1));
    QCOMPARE(image.size(), QSize(2, 1));
    QCOMPARE(image.pixelColor(0, 0), QColor(Qt::green));
}

void TestActiveNavigationThumbnailRuntime::missingLookupStartsGenerationAndPublishesGeneratedImage()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), store, generationProvider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    const quint64 generation = runtime.navigationGeneration();

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Missing));
    QCOMPARE(runtime.resultAt(0).status, Status::Pending);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
    QCOMPARE(generationProvider.generationCount(), std::size_t(1));
    QCOMPARE(generationProvider.generationAt(0).request.localPathBytes,
        QByteArrayLiteral("/media/01.png"));
    QCOMPARE(generationProvider.generationAt(0).request.requestedBucket, Bucket::Normal);

    generationProvider.finish(0,
        generationResult(kiriview::ThumbnailGenerationStatus::Ready, testThumbnailImage(Qt::blue)));
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QVERIFY(runtime.resultAt(0).imageSource.toString().startsWith(
        QStringLiteral("image://kiriview-thumbnails/")));
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::cacheHitSkipsGeneration()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), {}, generationProvider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, imageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    provider.finish(0,
        lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::green)));

    QCOMPARE(generationProvider.generationCount(), std::size_t(0));
}

void TestActiveNavigationThumbnailRuntime::
    largerBucketDemandKeepsReadyThumbnailWhileLookupIsPending()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider(), store);
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    const quint64 generation = runtime.navigationGeneration();

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(0,
        lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::green)));
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
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider(), store);
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    const quint64 generation = runtime.navigationGeneration();

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(0,
        lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::green)));
    const QUrl previousSource = runtime.resultAt(0).imageSource;
    const QString previousId = thumbnailImageStoreId(previousSource);

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));
    QCOMPARE(provider.lookupAt(1).canceled, true);
    QCOMPARE(provider.lookupCount(), std::size_t(3));
    provider.finish(
        2, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::blue)));

    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QVERIFY(runtime.resultAt(0).imageSource != previousSource);
    QCOMPARE(store->size(), qsizetype(1));
    QVERIFY(store->image(previousId).isNull());
    const QImage replacement = store->image(thumbnailImageStoreId(runtime.resultAt(0).imageSource));
    QCOMPARE(replacement.pixelColor(0, 0), QColor(Qt::blue));
}

void TestActiveNavigationThumbnailRuntime::lookupOrGenerationFailureKeepsExistingReadyThumbnail()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), store, generationProvider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    const quint64 generation = runtime.navigationGeneration();

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(0,
        lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::green)));
    const QUrl previousSource = runtime.resultAt(0).imageSource;

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));
    QCOMPARE(provider.lookupAt(1).canceled, true);
    QCOMPARE(provider.lookupCount(), std::size_t(3));
    provider.finish(2, lookupResult(kiriview::ThumbnailCacheLookupStatus::Failed));
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(runtime.resultAt(0).imageSource, previousSource);

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::XLarge, Priority::Visible, generation));
    QCOMPARE(provider.lookupAt(3).canceled, true);
    QCOMPARE(provider.lookupCount(), std::size_t(5));
    provider.finish(4, lookupResult(kiriview::ThumbnailCacheLookupStatus::Missing));
    generationProvider.finish(0, generationResult(kiriview::ThumbnailGenerationStatus::Failed));
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(runtime.resultAt(0).imageSource, previousSource);
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::
    lookupInvalidAndFailedSkipGenerationAndPublishFallbackResults()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), store, generationProvider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    const quint64 generation = runtime.navigationGeneration();

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));
    provider.finish(0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Invalid));
    QCOMPARE(runtime.resultAt(0).status, Status::Failed);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::XLarge, Priority::Visible, generation));
    QCOMPARE(provider.lookupAt(1).canceled, true);
    QCOMPARE(provider.lookupCount(), std::size_t(3));
    provider.finish(2, lookupResult(kiriview::ThumbnailCacheLookupStatus::Failed));
    QCOMPARE(runtime.resultAt(0).status, Status::Failed);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
    QCOMPARE(store->size(), qsizetype(0));
    QCOMPARE(generationProvider.generationCount(), std::size_t(0));
}

void TestActiveNavigationThumbnailRuntime::failureDiagnosticsPreserveSourceAndErrorDetail()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using FailureKind = kiriview::ActiveNavigationThumbnailFailureKind;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using WorkKind = kiriview::ActiveNavigationThumbnailWorkKind;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), {}, generationProvider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    const quint64 generation = runtime.navigationGeneration();

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(0,
        lookupResult(kiriview::ThumbnailCacheLookupStatus::Failed, {}, Bucket::Normal,
            QStringLiteral("cache database unavailable")));

    const std::vector<kiriview::ActiveNavigationThumbnailFailureDiagnostic> &lookupDiagnostics
        = runtime.failureDiagnostics();
    QCOMPARE(lookupDiagnostics.size(), std::size_t(1));
    if (lookupDiagnostics.size() != std::size_t(1)) {
        return;
    }
    QCOMPARE(lookupDiagnostics.at(0).jobId, quint64(1));
    QCOMPARE(lookupDiagnostics.at(0).sourceKey.rowNumber, 1);
    QCOMPARE(lookupDiagnostics.at(0).sourceKey.url, imageUrl);
    QCOMPARE(lookupDiagnostics.at(0).sourceKey.label, QStringLiteral("01.png"));
    QCOMPARE(lookupDiagnostics.at(0).bucket, Bucket::Normal);
    QCOMPARE(lookupDiagnostics.at(0).workKind, WorkKind::Foreground);
    QCOMPARE(lookupDiagnostics.at(0).failureKind, FailureKind::CacheLookupFailed);
    QCOMPARE(lookupDiagnostics.at(0).errorString, QStringLiteral("cache database unavailable"));

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));
    provider.finish(
        2, lookupResult(kiriview::ThumbnailCacheLookupStatus::Missing, {}, Bucket::Large));
    generationProvider.finish(0,
        generationResult(kiriview::ThumbnailGenerationStatus::Failed, {}, Bucket::Large,
            QStringLiteral("decoder rejected thumbnail bytes")));

    const std::vector<kiriview::ActiveNavigationThumbnailFailureDiagnostic> &diagnostics
        = runtime.failureDiagnostics();
    QCOMPARE(diagnostics.size(), std::size_t(2));
    if (diagnostics.size() != std::size_t(2)) {
        return;
    }
    QCOMPARE(diagnostics.at(1).jobId, quint64(4));
    QCOMPARE(diagnostics.at(1).sourceKey.rowNumber, 1);
    QCOMPARE(diagnostics.at(1).sourceKey.url, imageUrl);
    QCOMPARE(diagnostics.at(1).bucket, Bucket::Large);
    QCOMPARE(diagnostics.at(1).workKind, WorkKind::Foreground);
    QCOMPARE(diagnostics.at(1).failureKind, FailureKind::GenerationFailed);
    QCOMPARE(diagnostics.at(1).errorString, QStringLiteral("decoder rejected thumbnail bytes"));
}

void TestActiveNavigationThumbnailRuntime::generationFailurePublishesFallbackResult()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), {}, generationProvider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, imageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    provider.finish(0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Missing));
    generationProvider.finish(0, generationResult(kiriview::ThumbnailGenerationStatus::Failed));

    QCOMPARE(runtime.resultAt(0).status, Status::Failed);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
}

void TestActiveNavigationThumbnailRuntime::nonLocalDirectImageIsUnsupportedWithoutLookup()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl imageUrl(QStringLiteral("https://example.invalid/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, imageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(provider.lookupCount(), std::size_t(0));
    QCOMPARE(runtime.resultAt(0).status, Status::Unsupported);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
}

void TestActiveNavigationThumbnailRuntime::nonLocalDirectVideoIsUnsupportedWithoutLookup()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, lookupProvider.provider(), {}, generationProvider.provider());
    const QUrl videoUrl(QStringLiteral("https://example.invalid/clip.mp4"));
    runtime.setRows({
        thumbnailRow(1, videoUrl, QStringLiteral("clip.mp4"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
    });

    QVERIFY(runtime.reportDemand(
        1, videoUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(lookupProvider.lookupCount(), std::size_t(0));
    QCOMPARE(generationProvider.generationCount(), std::size_t(0));
    QCOMPARE(runtime.resultAt(0).status, Status::Unsupported);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
}

void TestActiveNavigationThumbnailRuntime::defaultDirectLocalVideoUsesCacheLookupFirst()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, lookupProvider.provider(), {}, generationProvider.provider());
    const QUrl videoUrl = localUrl(QStringLiteral("/media/clip.mp4"));
    runtime.setRows({
        thumbnailRow(1, videoUrl, QStringLiteral("clip.mp4"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
    });

    QVERIFY(runtime.reportDemand(
        1, videoUrl, Bucket::Large, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(lookupProvider.lookupCount(), std::size_t(1));
    QCOMPARE(
        lookupProvider.lookupAt(0).request.localPathBytes, QByteArrayLiteral("/media/clip.mp4"));
    QCOMPARE(lookupProvider.lookupAt(0).request.requestedBucket, Bucket::Large);
    QVERIFY(lookupProvider.lookupAt(0).request.originalIdentity.isLocalPath());
    QCOMPARE(lookupProvider.lookupAt(0).request.originalIdentity.localPathBytes,
        QByteArrayLiteral("/media/clip.mp4"));
    QCOMPARE(generationProvider.generationCount(), std::size_t(0));
}

void TestActiveNavigationThumbnailRuntime::defaultDirectLocalVideoCacheMissStartsVideoGeneration()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, lookupProvider.provider(), {}, generationProvider.provider());
    const QUrl videoUrl = localUrl(QStringLiteral("/media/clip.mp4"));
    runtime.setRows({
        thumbnailRow(1, videoUrl, QStringLiteral("clip.mp4"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
    });

    QVERIFY(runtime.reportDemand(
        1, videoUrl, Bucket::Large, Priority::Visible, runtime.navigationGeneration()));
    lookupProvider.finish(0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Missing));

    QCOMPARE(generationProvider.generationCount(), std::size_t(1));
    QCOMPARE(generationProvider.generationAt(0).request.localPathBytes,
        QByteArrayLiteral("/media/clip.mp4"));
    QCOMPARE(generationProvider.generationAt(0).request.sourceUrl, videoUrl);
    QCOMPARE(generationProvider.generationAt(0).request.sourceKind,
        kiriview::ThumbnailSourceKind::DirectVideo);
    QCOMPARE(generationProvider.generationAt(0).request.requestedBucket, Bucket::Large);
    QCOMPARE(generationProvider.generationAt(0).request.cacheInstallEnabled, true);
}

void TestActiveNavigationThumbnailRuntime::
    directVideoGenerationFailurePublishesFallbackAndDiagnostics()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using FailureKind = kiriview::ActiveNavigationThumbnailFailureKind;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, lookupProvider.provider(), {}, generationProvider.provider());
    const QUrl videoUrl = localUrl(QStringLiteral("/media/clip.mp4"));
    runtime.setRows({
        thumbnailRow(1, videoUrl, QStringLiteral("clip.mp4"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
    });

    QVERIFY(runtime.reportDemand(
        1, videoUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    lookupProvider.finish(0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Missing));
    generationProvider.finish(0,
        generationResult(kiriview::ThumbnailGenerationStatus::Failed, {}, Bucket::Normal,
            QStringLiteral("synthetic video decode failure")));

    QCOMPARE(runtime.resultAt(0).status, Status::Failed);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
    const std::vector<kiriview::ActiveNavigationThumbnailFailureDiagnostic> &diagnostics
        = runtime.failureDiagnostics();
    QCOMPARE(diagnostics.size(), std::size_t(1));
    if (diagnostics.empty()) {
        return;
    }
    QCOMPARE(diagnostics.at(0).sourceKey.url, videoUrl);
    QCOMPARE(diagnostics.at(0).sourceKey.sourceKind, QStringLiteral("direct-video"));
    QCOMPARE(diagnostics.at(0).failureKind, FailureKind::GenerationFailed);
    QCOMPARE(diagnostics.at(0).errorString, QStringLiteral("synthetic video decode failure"));
}

void TestActiveNavigationThumbnailRuntime::staleLookupCompletionIsRejectedAndReleasesInsertedImage()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider(), store);
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    QCOMPARE(provider.lookupCount(), std::size_t(1));

    runtime.setRows({
        thumbnailRow(1, secondUrl, QStringLiteral("02.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    provider.deliverIgnoringCancellation(
        0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));

    QCOMPARE(runtime.resultAt(0).status, Status::NoResult);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
    QCOMPARE(store->size(), qsizetype(0));
}

void TestActiveNavigationThumbnailRuntime::staleGenerationCompletionIsRejected()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), store, generationProvider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    provider.finish(0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Missing));

    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Large, Priority::Visible, runtime.navigationGeneration()));
    generationProvider.deliverIgnoringCancellation(
        0, generationResult(kiriview::ThumbnailGenerationStatus::Ready, testThumbnailImage()));
    QCOMPARE(runtime.resultAt(0).status, Status::Pending);
    QCOMPARE(store->size(), qsizetype(0));

    provider.finish(1, lookupResult(kiriview::ThumbnailCacheLookupStatus::Missing));
    runtime.setRows({
        thumbnailRow(1, secondUrl, QStringLiteral("02.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    generationProvider.deliverIgnoringCancellation(
        1, generationResult(kiriview::ThumbnailGenerationStatus::Ready, testThumbnailImage()));
    QCOMPARE(runtime.resultAt(0).status, Status::NoResult);
    QCOMPARE(store->size(), qsizetype(0));
}

void TestActiveNavigationThumbnailRuntime::independentRowsDoNotOverwriteGeneratedResults()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, provider.provider(), store, generationProvider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(runtime.reportDemand(2, secondUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Missing));
    provider.finish(1, lookupResult(kiriview::ThumbnailCacheLookupStatus::Missing));

    generationProvider.finish(1,
        generationResult(kiriview::ThumbnailGenerationStatus::Ready, testThumbnailImage(Qt::cyan)));
    QCOMPARE(runtime.resultAt(0).status, Status::Pending);
    QCOMPARE(runtime.resultAt(1).status, Status::Ready);

    generationProvider.finish(0,
        generationResult(
            kiriview::ThumbnailGenerationStatus::Ready, testThumbnailImage(Qt::magenta)));
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(runtime.resultAt(1).status, Status::Ready);
    QVERIFY(runtime.resultAt(0).imageSource != runtime.resultAt(1).imageSource);
    QCOMPARE(store->size(), qsizetype(2));
}

void TestActiveNavigationThumbnailRuntime::backgroundFillStartsOnlyAfterForegroundJobsDrain()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/media/03.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage),
        thumbnailRow(3, thirdUrl, QStringLiteral("03.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(runtime.reportDemand(2, secondUrl, Bucket::Normal, Priority::Nearby, generation));
    QCOMPARE(provider.lookupCount(), std::size_t(2));

    provider.finish(
        0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));
    QCOMPARE(provider.lookupCount(), std::size_t(2));

    provider.finish(
        1, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));
    QCOMPARE(provider.lookupCount(), std::size_t(3));
    QCOMPARE(provider.lookupAt(2).request.localPathBytes, QByteArrayLiteral("/media/03.png"));
    QCOMPARE(provider.lookupAt(2).request.requestedBucket, Bucket::Normal);
}

void TestActiveNavigationThumbnailRuntime::backgroundFillRunsNormalBeforeLargerBuckets()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(
        0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));

    QCOMPARE(provider.lookupCount(), std::size_t(2));
    QCOMPARE(provider.lookupAt(1).request.localPathBytes, QByteArrayLiteral("/media/02.png"));
    QCOMPARE(provider.lookupAt(1).request.requestedBucket, Bucket::Normal);

    provider.finish(
        1, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));
    QCOMPARE(provider.lookupCount(), std::size_t(3));
    QCOMPARE(provider.lookupAt(2).request.localPathBytes, QByteArrayLiteral("/media/01.png"));
    QCOMPARE(provider.lookupAt(2).request.requestedBucket, Bucket::Large);

    provider.finish(
        2, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));
    QCOMPARE(provider.lookupCount(), std::size_t(4));
    QCOMPARE(provider.lookupAt(3).request.localPathBytes, QByteArrayLiteral("/media/02.png"));
    QCOMPARE(provider.lookupAt(3).request.requestedBucket, Bucket::Large);
}

void TestActiveNavigationThumbnailRuntime::foregroundDemandCancelsActiveBackgroundWork()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(
        0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));
    QCOMPARE(provider.lookupCount(), std::size_t(2));

    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Large, Priority::Visible, generation));
    QCOMPARE(provider.lookupAt(1).canceled, true);
    QCOMPARE(provider.lookupCount(), std::size_t(3));
    QCOMPARE(provider.lookupAt(2).request.localPathBytes, QByteArrayLiteral("/media/01.png"));
    QCOMPARE(provider.lookupAt(2).request.requestedBucket, Bucket::Large);
}

void TestActiveNavigationThumbnailRuntime::
    backgroundFillIncludesDirectLocalVideoAndSkipsNonLocalRows()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider());
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl remoteUrl(QStringLiteral("https://example.invalid/03.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, videoUrl, QStringLiteral("02.mp4"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectVideo),
        thumbnailRow(3, remoteUrl, QStringLiteral("03.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    provider.finish(
        0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));

    QCOMPARE(provider.lookupCount(), std::size_t(2));
    QCOMPARE(provider.lookupAt(1).request.localPathBytes, QByteArrayLiteral("/media/02.mp4"));
    QCOMPARE(provider.lookupAt(1).request.requestedBucket, Bucket::Normal);

    provider.finish(
        1, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));

    QCOMPARE(provider.lookupCount(), std::size_t(3));
    QCOMPARE(provider.lookupAt(2).request.localPathBytes, QByteArrayLiteral("/media/01.png"));
    QCOMPARE(provider.lookupAt(2).request.requestedBucket, Bucket::Large);
}

void TestActiveNavigationThumbnailRuntime::staleCanceledBackgroundCompletionsAreRejected()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider(), store);
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(
        0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage()));
    QCOMPARE(provider.lookupCount(), std::size_t(2));

    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Large, Priority::Visible, generation));
    provider.deliverIgnoringCancellation(
        1, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::blue)));

    QCOMPARE(runtime.resultAt(1).status, Status::NoResult);
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::
    backgroundReadyResultDoesNotReplaceExistingReadyThumbnail()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider provider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, provider.provider(), store);
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    provider.finish(
        0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::red)));
    QCOMPARE(provider.lookupCount(), std::size_t(2));
    provider.finish(
        1, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::blue)));
    QCOMPARE(runtime.resultAt(1).status, Status::Ready);
    const QUrl backgroundSource = runtime.resultAt(1).imageSource;

    QVERIFY(runtime.reportDemand(2, secondUrl, Bucket::Normal, Priority::Visible, generation));
    const QUrl foregroundSource = runtime.resultAt(1).imageSource;
    QCOMPARE(foregroundSource, backgroundSource);
    QCOMPARE(provider.lookupAt(2).canceled, true);
    QCOMPARE(provider.lookupCount(), std::size_t(4));
    provider.finish(3,
        lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::green)));
    QCOMPARE(runtime.resultAt(1).status, Status::Ready);
    QVERIFY(runtime.resultAt(1).imageSource != foregroundSource);
    const QUrl foregroundReadySource = runtime.resultAt(1).imageSource;

    while (provider.lookupCount() < 6) {
        provider.finish(provider.lookupCount() - 1,
            lookupResult(
                kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::yellow)));
    }

    QCOMPARE(provider.lookupAt(5).request.localPathBytes, QByteArrayLiteral("/media/02.png"));
    QCOMPARE(provider.lookupAt(5).request.requestedBucket, Bucket::Large);
    provider.finish(
        5, lookupResult(kiriview::ThumbnailCacheLookupStatus::Ready, testThumbnailImage(Qt::cyan)));

    QCOMPARE(runtime.resultAt(1).status, Status::Ready);
    QCOMPARE(runtime.resultAt(1).imageSource, foregroundReadySource);
}

void TestActiveNavigationThumbnailRuntime::injectedUnsupportedAdapterSkipsLookupAndGeneration()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    RecordingThumbnailSourceAdapter sourceAdapter;
    sourceAdapter.plan = sourceAdapterPlan(kiriview::ThumbnailSourceAdapterPlanKind::Unsupported);
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, lookupProvider.provider(), {},
        generationProvider.provider(), sourceAdapter.adapter());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/unsupported.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("unsupported.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, imageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(sourceAdapter.requests.size(), std::size_t(1));
    QCOMPARE(sourceAdapter.requests.at(0).sourceKey.sourceKind, QStringLiteral("direct-image"));
    QCOMPARE(sourceAdapter.requests.at(0).requestedBucket, Bucket::Normal);
    QCOMPARE(lookupProvider.lookupCount(), std::size_t(0));
    QCOMPARE(generationProvider.generationCount(), std::size_t(0));
    QCOMPARE(runtime.resultAt(0).status, Status::Unsupported);
    QCOMPARE(runtime.resultAt(0).imageSource, QUrl());
}

void TestActiveNavigationThumbnailRuntime::injectedDirectVideoAdapterUsesGeneratedResultPath()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    RecordingThumbnailSourceAdapter sourceAdapter;
    sourceAdapter.plan
        = sourceAdapterPlan(kiriview::ThumbnailSourceAdapterPlanKind::CacheableLocalFile,
            QByteArrayLiteral("/media/clip.mp4"));
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, lookupProvider.provider(), store,
        generationProvider.provider(), sourceAdapter.adapter());
    const QUrl videoUrl = localUrl(QStringLiteral("/media/clip.mp4"));
    runtime.setRows({
        thumbnailRow(1, videoUrl, QStringLiteral("clip.mp4"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
    });

    QVERIFY(runtime.reportDemand(
        1, videoUrl, Bucket::Large, Priority::Visible, runtime.navigationGeneration()));
    QCOMPARE(lookupProvider.lookupCount(), std::size_t(1));
    QCOMPARE(
        lookupProvider.lookupAt(0).request.localPathBytes, QByteArrayLiteral("/media/clip.mp4"));
    lookupProvider.finish(0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Missing));

    QCOMPARE(generationProvider.generationCount(), std::size_t(1));
    QCOMPARE(generationProvider.generationAt(0).request.localPathBytes,
        QByteArrayLiteral("/media/clip.mp4"));
    QCOMPARE(generationProvider.generationAt(0).request.sourceKind,
        kiriview::ThumbnailSourceKind::DirectVideo);
    QCOMPARE(generationProvider.generationAt(0).request.cacheInstallEnabled, true);
    generationProvider.finish(0,
        generationResult(kiriview::ThumbnailGenerationStatus::Ready, testThumbnailImage(Qt::blue),
            Bucket::Large));

    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QVERIFY(runtime.resultAt(0).imageSource.toString().startsWith(
        QStringLiteral("image://kiriview-thumbnails/")));
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::injectedCollectionAdapterUsesGeneratedResultPath()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    RecordingThumbnailSourceAdapter sourceAdapter;
    sourceAdapter.plan
        = sourceAdapterPlan(kiriview::ThumbnailSourceAdapterPlanKind::CacheableLocalFile,
            QByteArrayLiteral("/archive/pages/001.png"));
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, lookupProvider.provider(), store,
        generationProvider.provider(), sourceAdapter.adapter());
    const QUrl pageUrl = localUrl(QStringLiteral("/archive/book.cbz#001"));
    runtime.setRows({
        thumbnailRow(1, pageUrl, QStringLiteral("001.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::ImageDocumentPageImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, pageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    lookupProvider.finish(0, lookupResult(kiriview::ThumbnailCacheLookupStatus::Missing));
    generationProvider.finish(0,
        generationResult(kiriview::ThumbnailGenerationStatus::Ready, testThumbnailImage(Qt::cyan)));

    QCOMPARE(lookupProvider.lookupAt(0).request.localPathBytes,
        QByteArrayLiteral("/archive/pages/001.png"));
    QCOMPARE(generationProvider.generationAt(0).request.sourceKind,
        kiriview::ThumbnailSourceKind::ImageDocumentPageImage);
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::
    openedCollectionEntryAdapterStartsGenerationWithoutPrelookup()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    const kiriview::OpenedCollectionScopeLocation cbzScope
        = kiriview::OpenedCollectionScopeLocation::fromUrls(
            localUrl(QStringLiteral("/books/book.cbz")),
            QUrl(QStringLiteral("zip:///books/book.cbz/")),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive);
    RecordingThumbnailSourceAdapter sourceAdapter;
    sourceAdapter.plan.kind
        = kiriview::ThumbnailSourceAdapterPlanKind::CacheableOpenedCollectionEntry;
    sourceAdapter.plan.openedCollectionScope = cbzScope;
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, lookupProvider.provider(), store,
        generationProvider.provider(), sourceAdapter.adapter());
    const QUrl pageUrl = archivePageUrl(cbzScope.rootUrl(), QStringLiteral("pages/001.png"));
    runtime.setRows({
        thumbnailRow(1, pageUrl, QStringLiteral("pages/001.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::ImageDocumentPageImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, pageUrl, Bucket::Large, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(lookupProvider.lookupCount(), std::size_t(0));
    QCOMPARE(generationProvider.generationCount(), std::size_t(1));
    QCOMPARE(generationProvider.generationAt(0).request.openedCollectionScope, cbzScope);
    QCOMPARE(generationProvider.generationAt(0).request.cacheInstallEnabled, true);
    generationProvider.finish(0,
        generationResult(kiriview::ThumbnailGenerationStatus::Ready, testThumbnailImage(Qt::cyan),
            Bucket::Large));

    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::inMemoryOnlyAdapterSkipsLookupAndDisablesCacheInstall()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ActiveNavigationThumbnailResultStatus;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    RecordingThumbnailSourceAdapter sourceAdapter;
    sourceAdapter.plan = sourceAdapterPlan(kiriview::ThumbnailSourceAdapterPlanKind::InMemoryOnly,
        QByteArrayLiteral("archive-entry:001"));
    kiriview::ActiveNavigationThumbnailRuntime runtime(&owner, lookupProvider.provider(), store,
        generationProvider.provider(), sourceAdapter.adapter());
    const QUrl pageUrl = localUrl(QStringLiteral("/archive/book.cbz#001"));
    runtime.setRows({
        thumbnailRow(1, pageUrl, QStringLiteral("001.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::ImageDocumentPageImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, pageUrl, Bucket::XLarge, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(lookupProvider.lookupCount(), std::size_t(0));
    QCOMPARE(generationProvider.generationCount(), std::size_t(1));
    QCOMPARE(generationProvider.generationAt(0).request.localPathBytes,
        QByteArrayLiteral("archive-entry:001"));
    QCOMPARE(generationProvider.generationAt(0).request.cacheInstallEnabled, false);
    generationProvider.finish(0,
        generationResult(kiriview::ThumbnailGenerationStatus::Ready,
            testThumbnailImage(Qt::magenta), Bucket::XLarge));

    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(store->size(), qsizetype(1));
}

void TestActiveNavigationThumbnailRuntime::defaultDirectLocalImageBehaviorStillUsesLookup()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualThumbnailLookupProvider lookupProvider;
    ManualThumbnailGenerationProvider generationProvider;
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, lookupProvider.provider(), {}, generationProvider.provider());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, imageUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(lookupProvider.lookupCount(), std::size_t(1));
    QCOMPARE(lookupProvider.lookupAt(0).request.localPathBytes, QByteArrayLiteral("/media/01.png"));
    QCOMPARE(generationProvider.generationCount(), std::size_t(0));
}

void TestActiveNavigationThumbnailRuntime::defaultGenerationProviderUsesWorkerScheduler()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    QObject owner;
    ManualImageWorkerScheduler workerScheduler;
    RecordingThumbnailSourceAdapter sourceAdapter;
    sourceAdapter.plan = sourceAdapterPlan(kiriview::ThumbnailSourceAdapterPlanKind::InMemoryOnly);
    kiriview::ActiveNavigationThumbnailRuntime runtime(
        &owner, {}, {}, {}, sourceAdapter.adapter(), workerScheduler.scheduler());
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            kiriview::ActiveNavigationThumbnailSourceKind::DirectImage, true),
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
