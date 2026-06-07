// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ACTIVENAVIGATIONTHUMBNAILRUNTIME_H
#define KIRIVIEW_ACTIVENAVIGATIONTHUMBNAILRUNTIME_H

#include "session/activenavigationthumbnaildemand.h"
#include "session/activenavigationthumbnailmodel.h"
#include "session/activenavigationthumbnailprojection.h"
#include "session/thumbnailimagestore.h"
#include "thumbnail/thumbnailcachelookup.h"
#include "thumbnail/thumbnailgeneration.h"
#include "thumbnail/thumbnailoriginalidentity.h"

#include <QAbstractListModel>
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class QObject;

namespace KiriView {
struct ActiveNavigationThumbnailSourceKey {
    int number = 0;
    QUrl url;
    QString label;
    ActiveNavigationThumbnailKind kind = ActiveNavigationThumbnailKind::Image;
    ActiveNavigationThumbnailSourceKind sourceKind
        = ActiveNavigationThumbnailSourceKind::DirectImage;
    quint64 navigationGeneration = 0;
};

struct ActiveNavigationThumbnailResult {
    ActiveNavigationThumbnailResultStatus status = ActiveNavigationThumbnailResultStatus::NoResult;
    QUrl imageSource;
};

struct ActiveNavigationThumbnailCompletion {
    ActiveNavigationThumbnailSourceKey sourceKey;
    ActiveNavigationThumbnailDemandBucket bucket = ActiveNavigationThumbnailDemandBucket::None;
    ActiveNavigationThumbnailResult result;
};

enum class ThumbnailSourceAdapterPlanKind {
    Unsupported,
    CacheableLocalFile,
    CacheableOpenedCollectionEntry,
    InMemoryOnly,
};

struct ThumbnailSourceAdapterRequest {
    ActiveNavigationThumbnailSourceKey sourceKey;
    ActiveNavigationThumbnailDemandBucket requestedBucket
        = ActiveNavigationThumbnailDemandBucket::None;
    ActiveNavigationThumbnailDemandPriority priority
        = ActiveNavigationThumbnailDemandPriority::Nearby;
};

struct ThumbnailSourceAdapterPlan {
    ThumbnailSourceAdapterPlanKind kind = ThumbnailSourceAdapterPlanKind::Unsupported;
    QByteArray localPathBytes;
    ThumbnailOriginalIdentity originalIdentity;
    OpenedCollectionScopeLocation openedCollectionScope;
};

using ThumbnailSourceAdapter
    = std::function<ThumbnailSourceAdapterPlan(ThumbnailSourceAdapterRequest)>;

ThumbnailSourceAdapter defaultThumbnailSourceAdapter();

struct ActiveNavigationThumbnailRuntimeDependencies {
    ThumbnailCacheLookupProvider lookupProvider;
    std::shared_ptr<ThumbnailImageStore> imageStore;
    ThumbnailGenerationProvider generationProvider;
    ThumbnailSourceAdapter sourceAdapter;
    ImageWorkerScheduler workerScheduler;
};

class ActiveNavigationThumbnailRuntime final
{
public:
    explicit ActiveNavigationThumbnailRuntime(
        QObject *owner = nullptr, ActiveNavigationThumbnailRuntimeDependencies dependencies = {});
    ActiveNavigationThumbnailRuntime(QObject *owner,
        ThumbnailCacheLookupProvider lookupProvider = {},
        std::shared_ptr<ThumbnailImageStore> imageStore = {},
        ThumbnailGenerationProvider generationProvider = {},
        ThumbnailSourceAdapter sourceAdapter = {}, ImageWorkerScheduler workerScheduler = {});
    ~ActiveNavigationThumbnailRuntime();

    QAbstractListModel *model() const;
    quint64 navigationGeneration() const;

    void setRows(std::vector<ActiveNavigationThumbnailRow> rows);
    bool reportDemand(int number, const QUrl &url, ActiveNavigationThumbnailDemandBucket bucket,
        ActiveNavigationThumbnailDemandPriority priority, quint64 navigationGeneration);
    bool applyCompletion(const ActiveNavigationThumbnailCompletion &completion);

    ActiveNavigationThumbnailSourceKey sourceKeyAt(std::size_t row) const;
    ActiveNavigationThumbnailResult resultAt(std::size_t row) const;
    qsizetype activeJobCount() const;
    qsizetype canceledJobCount() const;

private:
    enum class ThumbnailWorkKind {
        Foreground,
        Background,
    };

    struct AcceptedDemand {
        ActiveNavigationThumbnailSourceKey sourceKey;
        ActiveNavigationThumbnailDemandBucket bucket = ActiveNavigationThumbnailDemandBucket::None;
        ActiveNavigationThumbnailDemandPriority priority
            = ActiveNavigationThumbnailDemandPriority::Nearby;
        ThumbnailSourceAdapterPlan sourcePlan;
    };

    struct ActiveJobSlot {
        quint64 id = 0;
        ThumbnailWorkKind kind = ThumbnailWorkKind::Foreground;
        AcceptedDemand demand;
        ImageIoJob job;
    };

    struct RowState {
        ActiveNavigationThumbnailRow row;
        ActiveNavigationThumbnailSourceKey sourceKey;
        ActiveNavigationThumbnailResult result;
        QString imageStoreId;
        std::optional<AcceptedDemand> acceptedDemand;
        std::optional<ActiveJobSlot> activeJob;
        std::vector<ActiveNavigationThumbnailDemandBucket> completedBackgroundBuckets;
    };

    static bool sameRowIdentity(
        const ActiveNavigationThumbnailRow &left, const ActiveNavigationThumbnailRow &right);
    static bool sameSourceKey(const ActiveNavigationThumbnailSourceKey &left,
        const ActiveNavigationThumbnailSourceKey &right);
    static bool sameSourceAdapterPlan(
        const ThumbnailSourceAdapterPlan &left, const ThumbnailSourceAdapterPlan &right);
    static bool sameAcceptedDemand(const AcceptedDemand &left, const AcceptedDemand &right);
    static bool supportsGeneratedThumbnail(const ThumbnailSourceAdapterPlan &plan);
    static bool usesCacheLookup(const ThumbnailSourceAdapterPlan &plan);
    static bool enablesCacheInstall(const ThumbnailSourceAdapterPlan &plan);
    static ThumbnailImageRetentionPriority imageRetentionPriority(
        ActiveNavigationThumbnailDemandPriority priority);
    static ThumbnailImageRetentionPriority imageRetentionPriority(
        ThumbnailWorkKind kind, ActiveNavigationThumbnailDemandPriority priority);

    std::optional<std::size_t> rowIndexForIdentity(
        int number, const QUrl &url, quint64 navigationGeneration) const;
    std::optional<std::size_t> rowIndexForSourceKey(
        const ActiveNavigationThumbnailSourceKey &sourceKey) const;
    void cancelActiveJob(RowState &state);
    void cancelActiveBackgroundJob();
    void cancelAllActiveJobs();
    bool hasActiveForegroundJob() const;
    bool hasUsableReadyImage(const RowState &state) const;
    void releaseImage(RowState &state);
    void releaseAllImages();
    void startLookupJob(RowState &state, const AcceptedDemand &demand, ThumbnailWorkKind kind);
    void startGenerationJob(RowState &state, const AcceptedDemand &demand, ThumbnailWorkKind kind);
    bool activeJobMatches(const RowState &state, quint64 jobId, const AcceptedDemand &demand,
        ThumbnailWorkKind kind) const;
    bool backgroundBucketCompleted(
        const RowState &state, ActiveNavigationThumbnailDemandBucket bucket) const;
    void markBackgroundBucketCompleted(
        RowState &state, ActiveNavigationThumbnailDemandBucket bucket);
    void maybeScheduleBackgroundWork();
    ThumbnailSourceAdapterPlan sourcePlanForDemand(
        const ActiveNavigationThumbnailSourceKey &sourceKey,
        ActiveNavigationThumbnailDemandBucket bucket,
        ActiveNavigationThumbnailDemandPriority priority) const;
    void startBackgroundWork(RowState &state, ActiveNavigationThumbnailDemandBucket bucket,
        ThumbnailSourceAdapterPlan sourcePlan);
    void finishLookup(quint64 jobId, const ActiveNavigationThumbnailSourceKey &sourceKey,
        ActiveNavigationThumbnailDemandBucket bucket, ThumbnailCacheLookupResult lookupResult);
    void finishGeneration(quint64 jobId, const ActiveNavigationThumbnailSourceKey &sourceKey,
        ActiveNavigationThumbnailDemandBucket bucket, ThumbnailGenerationResult generationResult);
    void publishRows();
    void publishResultAt(std::size_t row);

    QObject *m_owner = nullptr;
    std::unique_ptr<ActiveNavigationThumbnailModel> m_model;
    ThumbnailCacheLookupProvider m_lookupProvider;
    ThumbnailGenerationProvider m_generationProvider;
    ThumbnailSourceAdapter m_sourceAdapter;
    std::shared_ptr<ThumbnailImageStore> m_imageStore;
    std::vector<RowState> m_rows;
    quint64 m_navigationGeneration = 0;
    quint64 m_nextJobId = 1;
    std::vector<quint64> m_canceledJobIds;
    bool m_backgroundArmed = false;
};
}

#endif
