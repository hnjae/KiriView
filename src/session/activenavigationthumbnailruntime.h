// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ACTIVENAVIGATIONTHUMBNAILRUNTIME_H
#define KIRIVIEW_ACTIVENAVIGATIONTHUMBNAILRUNTIME_H

#include "session/activenavigationthumbnaildemand.h"
#include "session/activenavigationthumbnailmodel.h"
#include "session/activenavigationthumbnailprojection.h"
#include "session/thumbnailcachelookup.h"
#include "session/thumbnailgeneration.h"
#include "session/thumbnailimagestore.h"

#include <QAbstractListModel>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <cstddef>
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

class ActiveNavigationThumbnailRuntime final
{
public:
    explicit ActiveNavigationThumbnailRuntime(QObject *owner = nullptr,
        ThumbnailCacheLookupProvider lookupProvider = {},
        std::shared_ptr<ThumbnailImageStore> imageStore = {},
        ThumbnailGenerationProvider generationProvider = {});
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
    struct AcceptedDemand {
        ActiveNavigationThumbnailSourceKey sourceKey;
        ActiveNavigationThumbnailDemandBucket bucket = ActiveNavigationThumbnailDemandBucket::None;
        ActiveNavigationThumbnailDemandPriority priority
            = ActiveNavigationThumbnailDemandPriority::Nearby;
    };

    struct ActiveJobSlot {
        quint64 id = 0;
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
    };

    static bool sameRowIdentity(
        const ActiveNavigationThumbnailRow &left, const ActiveNavigationThumbnailRow &right);
    static bool sameSourceKey(const ActiveNavigationThumbnailSourceKey &left,
        const ActiveNavigationThumbnailSourceKey &right);
    static bool sameAcceptedDemand(const AcceptedDemand &left, const AcceptedDemand &right);
    static bool supportsGeneratedThumbnail(ActiveNavigationThumbnailSourceKind sourceKind);

    std::optional<std::size_t> rowIndexForIdentity(
        int number, const QUrl &url, quint64 navigationGeneration) const;
    std::optional<std::size_t> rowIndexForSourceKey(
        const ActiveNavigationThumbnailSourceKey &sourceKey) const;
    void cancelActiveJob(RowState &state);
    void cancelAllActiveJobs();
    void releaseImage(RowState &state);
    void releaseAllImages();
    void startLookupJob(RowState &state, const AcceptedDemand &demand);
    void startGenerationJob(RowState &state, const AcceptedDemand &demand);
    bool activeJobMatches(const RowState &state, quint64 jobId, const AcceptedDemand &demand) const;
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
    std::shared_ptr<ThumbnailImageStore> m_imageStore;
    std::vector<RowState> m_rows;
    quint64 m_navigationGeneration = 0;
    quint64 m_nextJobId = 1;
    std::vector<quint64> m_canceledJobIds;
};
}

#endif
