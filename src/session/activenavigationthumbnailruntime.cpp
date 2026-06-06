// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "activenavigationthumbnailruntime.h"

#include "session/thumbnaillogging.h"

#include <QDebug>
#include <QFile>
#include <algorithm>
#include <array>
#include <utility>

namespace {
using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;

KiriView::ActiveNavigationThumbnailSourceKey sourceKeyForRow(
    const KiriView::ActiveNavigationThumbnailRow &row, quint64 navigationGeneration)
{
    return KiriView::ActiveNavigationThumbnailSourceKey {
        row.number,
        row.url,
        row.label,
        row.kind,
        row.sourceKind,
        navigationGeneration,
    };
}

constexpr std::array<Bucket, 4> backgroundFillBuckets()
{
    return { Bucket::Normal, Bucket::Large, Bucket::XLarge, Bucket::XXLarge };
}
}

namespace KiriView {
ThumbnailSourceAdapter defaultThumbnailSourceAdapter()
{
    return [](ThumbnailSourceAdapterRequest request) {
        if (request.sourceKey.sourceKind != ActiveNavigationThumbnailSourceKind::DirectImage
            || !request.sourceKey.url.isLocalFile()) {
            return ThumbnailSourceAdapterPlan {};
        }

        return ThumbnailSourceAdapterPlan {
            ThumbnailSourceAdapterPlanKind::CacheableLocalFile,
            QFile::encodeName(request.sourceKey.url.toLocalFile()),
            ThumbnailOriginalIdentity::fromLocalPathBytes(
                QFile::encodeName(request.sourceKey.url.toLocalFile())),
            {},
        };
    };
}

ActiveNavigationThumbnailRuntime::ActiveNavigationThumbnailRuntime(QObject *owner,
    ThumbnailCacheLookupProvider lookupProvider, std::shared_ptr<ThumbnailImageStore> imageStore,
    ThumbnailGenerationProvider generationProvider, ThumbnailSourceAdapter sourceAdapter,
    ImageWorkerScheduler workerScheduler)
    : m_owner(owner)
    , m_model(std::make_unique<ActiveNavigationThumbnailModel>(owner))
    , m_lookupProvider(lookupProvider ? std::move(lookupProvider)
                                      : defaultThumbnailCacheLookupProvider(workerScheduler))
    , m_generationProvider(generationProvider ? std::move(generationProvider)
                                              : defaultThumbnailGenerationProvider(workerScheduler))
    , m_sourceAdapter(sourceAdapter ? std::move(sourceAdapter) : defaultThumbnailSourceAdapter())
    , m_imageStore(std::move(imageStore))
{
    if (m_imageStore == nullptr) {
        m_imageStore = sharedThumbnailImageStore();
    }
}

ActiveNavigationThumbnailRuntime::~ActiveNavigationThumbnailRuntime()
{
    cancelAllActiveJobs();
    releaseAllImages();
}

QAbstractListModel *ActiveNavigationThumbnailRuntime::model() const { return m_model.get(); }

quint64 ActiveNavigationThumbnailRuntime::navigationGeneration() const
{
    return m_navigationGeneration;
}

void ActiveNavigationThumbnailRuntime::setRows(std::vector<ActiveNavigationThumbnailRow> rows)
{
    bool sameIdentities = m_rows.size() == rows.size();
    if (sameIdentities) {
        for (std::size_t row = 0; row < rows.size(); ++row) {
            if (!sameRowIdentity(m_rows.at(row).row, rows.at(row))) {
                sameIdentities = false;
                break;
            }
        }
    }

    if (sameIdentities) {
        for (std::size_t row = 0; row < rows.size(); ++row) {
            m_rows.at(row).row.current = rows.at(row).current;
        }
        publishRows();
        return;
    }

    cancelAllActiveJobs();
    releaseAllImages();
    ++m_navigationGeneration;
    m_backgroundArmed = false;
    qCDebug(kiriviewThumbnailLog) << "Reset active navigation thumbnail rows generation"
                                  << m_navigationGeneration << "rowCount" << rows.size();
    m_rows.clear();
    m_rows.reserve(rows.size());
    for (ActiveNavigationThumbnailRow &row : rows) {
        RowState state;
        state.row = std::move(row);
        state.sourceKey = sourceKeyForRow(state.row, m_navigationGeneration);
        m_rows.push_back(std::move(state));
    }
    publishRows();
}

bool ActiveNavigationThumbnailRuntime::reportDemand(int number, const QUrl &url,
    ActiveNavigationThumbnailDemandBucket bucket, ActiveNavigationThumbnailDemandPriority priority,
    quint64 navigationGeneration)
{
    qCDebug(kiriviewThumbnailLog) << "Thumbnail demand" << number << url << "bucket"
                                  << static_cast<int>(bucket) << "priority"
                                  << static_cast<int>(priority) << "generation"
                                  << navigationGeneration;
    if (bucket == ActiveNavigationThumbnailDemandBucket::None || navigationGeneration == 0
        || navigationGeneration != m_navigationGeneration) {
        qCDebug(kiriviewThumbnailLog)
            << "Rejecting thumbnail demand for stale or empty generation" << navigationGeneration
            << "current" << m_navigationGeneration;
        return false;
    }

    const std::optional<std::size_t> rowIndex
        = rowIndexForIdentity(number, url, navigationGeneration);
    if (!rowIndex.has_value()) {
        qCDebug(kiriviewThumbnailLog)
            << "Rejecting thumbnail demand for unknown row" << number << url;
        return false;
    }

    RowState &state = m_rows.at(*rowIndex);
    const ThumbnailSourceAdapterPlan sourcePlan
        = sourcePlanForDemand(state.sourceKey, bucket, priority);
    const AcceptedDemand demand {
        state.sourceKey,
        bucket,
        priority,
        sourcePlan,
    };
    if (state.acceptedDemand.has_value() && sameAcceptedDemand(*state.acceptedDemand, demand)) {
        return false;
    }

    m_backgroundArmed = true;
    cancelActiveBackgroundJob();

    if (state.acceptedDemand.has_value()
        && sameSourceKey(state.acceptedDemand->sourceKey, demand.sourceKey)
        && state.acceptedDemand->bucket == demand.bucket
        && state.acceptedDemand->priority != demand.priority) {
        state.acceptedDemand = demand;
        if (m_imageStore != nullptr && !state.imageStoreId.isEmpty()) {
            m_imageStore->updatePriority(
                state.imageStoreId, imageRetentionPriority(demand.priority));
        }
        if (state.activeJob.has_value()) {
            state.activeJob->demand = demand;
        }
        maybeScheduleBackgroundWork();
        return true;
    }

    cancelActiveJob(state);
    state.acceptedDemand = demand;
    if (supportsGeneratedThumbnail(sourcePlan)) {
        if (!hasUsableReadyImage(state)) {
            releaseImage(state);
            state.result.status = ActiveNavigationThumbnailResultStatus::Pending;
            state.result.imageSource = QUrl();
        }
        state.activeJob = ActiveJobSlot {
            m_nextJobId++,
            ThumbnailWorkKind::Foreground,
            demand,
            {},
        };
        if (usesCacheLookup(sourcePlan)) {
            startLookupJob(state, demand, ThumbnailWorkKind::Foreground);
        } else {
            startGenerationJob(state, demand, ThumbnailWorkKind::Foreground);
        }
    } else {
        releaseImage(state);
        state.result.status = ActiveNavigationThumbnailResultStatus::Unsupported;
        state.result.imageSource = QUrl();
        state.activeJob.reset();
        qCDebug(kiriviewThumbnailLog) << "Thumbnail demand unsupported" << number << url;
    }
    publishResultAt(*rowIndex);
    maybeScheduleBackgroundWork();
    return true;
}

bool ActiveNavigationThumbnailRuntime::applyCompletion(
    const ActiveNavigationThumbnailCompletion &completion)
{
    const std::optional<std::size_t> rowIndex = rowIndexForSourceKey(completion.sourceKey);
    if (!rowIndex.has_value()) {
        return false;
    }

    RowState &state = m_rows.at(*rowIndex);
    if (!state.acceptedDemand.has_value()
        || !sameSourceKey(state.acceptedDemand->sourceKey, completion.sourceKey)
        || state.acceptedDemand->bucket != completion.bucket) {
        return false;
    }

    state.activeJob.reset();
    if (completion.result.status == ActiveNavigationThumbnailResultStatus::Ready) {
        releaseImage(state);
        state.result = completion.result;
    } else if (hasUsableReadyImage(state)) {
        state.result.status = ActiveNavigationThumbnailResultStatus::Ready;
        state.result.imageSource = thumbnailImageSourceForId(state.imageStoreId);
    } else {
        releaseImage(state);
        state.result = completion.result;
    }
    publishResultAt(*rowIndex);
    return true;
}

ActiveNavigationThumbnailSourceKey ActiveNavigationThumbnailRuntime::sourceKeyAt(
    std::size_t row) const
{
    return m_rows.at(row).sourceKey;
}

ActiveNavigationThumbnailResult ActiveNavigationThumbnailRuntime::resultAt(std::size_t row) const
{
    return m_rows.at(row).result;
}

qsizetype ActiveNavigationThumbnailRuntime::activeJobCount() const
{
    qsizetype count = 0;
    for (const RowState &state : m_rows) {
        if (state.activeJob.has_value()) {
            ++count;
        }
    }
    return count;
}

qsizetype ActiveNavigationThumbnailRuntime::canceledJobCount() const
{
    return static_cast<qsizetype>(m_canceledJobIds.size());
}

bool ActiveNavigationThumbnailRuntime::sameRowIdentity(
    const ActiveNavigationThumbnailRow &left, const ActiveNavigationThumbnailRow &right)
{
    return left.number == right.number && left.url == right.url && left.label == right.label
        && left.kind == right.kind && left.sourceKind == right.sourceKind;
}

bool ActiveNavigationThumbnailRuntime::sameSourceKey(
    const ActiveNavigationThumbnailSourceKey &left, const ActiveNavigationThumbnailSourceKey &right)
{
    return left.number == right.number && left.url == right.url && left.label == right.label
        && left.kind == right.kind && left.sourceKind == right.sourceKind
        && left.navigationGeneration == right.navigationGeneration;
}

bool ActiveNavigationThumbnailRuntime::sameSourceAdapterPlan(
    const ThumbnailSourceAdapterPlan &left, const ThumbnailSourceAdapterPlan &right)
{
    return left.kind == right.kind && left.localPathBytes == right.localPathBytes
        && left.originalIdentity.mode == right.originalIdentity.mode
        && left.originalIdentity.localPathBytes == right.originalIdentity.localPathBytes
        && left.originalIdentity.uri == right.originalIdentity.uri
        && left.originalIdentity.mtimeSeconds == right.originalIdentity.mtimeSeconds
        && left.originalIdentity.originalByteSize == right.originalIdentity.originalByteSize
        && left.originalIdentity.mimeType == right.originalIdentity.mimeType
        && sameOpenedCollectionScopeLocation(
            left.openedCollectionScope, right.openedCollectionScope);
}

bool ActiveNavigationThumbnailRuntime::sameAcceptedDemand(
    const AcceptedDemand &left, const AcceptedDemand &right)
{
    return sameSourceKey(left.sourceKey, right.sourceKey) && left.bucket == right.bucket
        && left.priority == right.priority
        && sameSourceAdapterPlan(left.sourcePlan, right.sourcePlan);
}

bool ActiveNavigationThumbnailRuntime::supportsGeneratedThumbnail(
    const ThumbnailSourceAdapterPlan &plan)
{
    return plan.kind == ThumbnailSourceAdapterPlanKind::InMemoryOnly
        || (plan.kind == ThumbnailSourceAdapterPlanKind::CacheableLocalFile
            && !plan.localPathBytes.isEmpty())
        || (plan.kind == ThumbnailSourceAdapterPlanKind::CacheableOpenedCollectionEntry
            && !plan.openedCollectionScope.isEmpty());
}

bool ActiveNavigationThumbnailRuntime::usesCacheLookup(const ThumbnailSourceAdapterPlan &plan)
{
    return plan.kind == ThumbnailSourceAdapterPlanKind::CacheableLocalFile
        && !plan.localPathBytes.isEmpty();
}

bool ActiveNavigationThumbnailRuntime::enablesCacheInstall(const ThumbnailSourceAdapterPlan &plan)
{
    return (plan.kind == ThumbnailSourceAdapterPlanKind::CacheableLocalFile
               && !plan.localPathBytes.isEmpty())
        || (plan.kind == ThumbnailSourceAdapterPlanKind::CacheableOpenedCollectionEntry
            && !plan.openedCollectionScope.isEmpty());
}

ThumbnailImageRetentionPriority ActiveNavigationThumbnailRuntime::imageRetentionPriority(
    ActiveNavigationThumbnailDemandPriority priority)
{
    switch (priority) {
    case ActiveNavigationThumbnailDemandPriority::Visible:
        return ThumbnailImageRetentionPriority::Visible;
    case ActiveNavigationThumbnailDemandPriority::Nearby:
        return ThumbnailImageRetentionPriority::Nearby;
    }

    return ThumbnailImageRetentionPriority::Nearby;
}

ThumbnailImageRetentionPriority ActiveNavigationThumbnailRuntime::imageRetentionPriority(
    ThumbnailWorkKind kind, ActiveNavigationThumbnailDemandPriority priority)
{
    if (kind == ThumbnailWorkKind::Background) {
        return ThumbnailImageRetentionPriority::Background;
    }

    return imageRetentionPriority(priority);
}

std::optional<std::size_t> ActiveNavigationThumbnailRuntime::rowIndexForIdentity(
    int number, const QUrl &url, quint64 navigationGeneration) const
{
    for (std::size_t row = 0; row < m_rows.size(); ++row) {
        const ActiveNavigationThumbnailSourceKey &sourceKey = m_rows.at(row).sourceKey;
        if (sourceKey.number == number && sourceKey.url == url
            && sourceKey.navigationGeneration == navigationGeneration) {
            return row;
        }
    }

    return {};
}

std::optional<std::size_t> ActiveNavigationThumbnailRuntime::rowIndexForSourceKey(
    const ActiveNavigationThumbnailSourceKey &sourceKey) const
{
    for (std::size_t row = 0; row < m_rows.size(); ++row) {
        if (sameSourceKey(m_rows.at(row).sourceKey, sourceKey)) {
            return row;
        }
    }

    return {};
}

void ActiveNavigationThumbnailRuntime::cancelActiveJob(RowState &state)
{
    if (!state.activeJob.has_value()) {
        return;
    }

    qCDebug(kiriviewThumbnailLog) << "Canceling thumbnail job" << state.activeJob->id << "kind"
                                  << static_cast<int>(state.activeJob->kind) << "number"
                                  << state.sourceKey.number << "bucket"
                                  << static_cast<int>(state.activeJob->demand.bucket);
    m_canceledJobIds.push_back(state.activeJob->id);
    state.activeJob->job.cancel();
    state.activeJob.reset();
}

void ActiveNavigationThumbnailRuntime::cancelActiveBackgroundJob()
{
    for (RowState &state : m_rows) {
        if (state.activeJob.has_value() && state.activeJob->kind == ThumbnailWorkKind::Background) {
            cancelActiveJob(state);
            return;
        }
    }
}

void ActiveNavigationThumbnailRuntime::cancelAllActiveJobs()
{
    for (RowState &state : m_rows) {
        cancelActiveJob(state);
    }
}

bool ActiveNavigationThumbnailRuntime::hasActiveForegroundJob() const
{
    return std::any_of(m_rows.cbegin(), m_rows.cend(), [](const RowState &state) {
        return state.activeJob.has_value()
            && state.activeJob->kind == ThumbnailWorkKind::Foreground;
    });
}

bool ActiveNavigationThumbnailRuntime::hasUsableReadyImage(const RowState &state) const
{
    return state.result.status == ActiveNavigationThumbnailResultStatus::Ready
        && !state.imageStoreId.isEmpty() && m_imageStore != nullptr
        && !m_imageStore->image(state.imageStoreId).isNull();
}

void ActiveNavigationThumbnailRuntime::releaseImage(RowState &state)
{
    if (m_imageStore != nullptr && !state.imageStoreId.isEmpty()) {
        m_imageStore->release(state.imageStoreId);
    }
    state.imageStoreId.clear();
}

void ActiveNavigationThumbnailRuntime::releaseAllImages()
{
    for (RowState &state : m_rows) {
        releaseImage(state);
    }
}

void ActiveNavigationThumbnailRuntime::startLookupJob(
    RowState &state, const AcceptedDemand &demand, ThumbnailWorkKind kind)
{
    if (state.activeJob == std::nullopt || !m_lookupProvider) {
        return;
    }

    ThumbnailCacheLookupRequest request;
    request.localPathBytes = demand.sourcePlan.localPathBytes;
    request.originalIdentity = demand.sourcePlan.originalIdentity;
    request.requestedBucket = demand.bucket;

    const quint64 jobId = state.activeJob->id;
    qCDebug(kiriviewThumbnailLog) << "Starting thumbnail lookup job" << jobId << "kind"
                                  << static_cast<int>(kind) << "number" << demand.sourceKey.number
                                  << "bucket" << static_cast<int>(demand.bucket);
    ImageIoJob job = m_lookupProvider(m_owner, std::move(request),
        [this, jobId, sourceKey = demand.sourceKey, bucket = demand.bucket](
            ThumbnailCacheLookupResult result) mutable {
            finishLookup(jobId, sourceKey, bucket, std::move(result));
        });
    const std::optional<std::size_t> rowIndex = rowIndexForSourceKey(demand.sourceKey);
    if (!rowIndex.has_value() || !activeJobMatches(m_rows.at(*rowIndex), jobId, demand, kind)) {
        job.cancel();
        return;
    }
    m_rows.at(*rowIndex).activeJob->job = std::move(job);
}

void ActiveNavigationThumbnailRuntime::startGenerationJob(
    RowState &state, const AcceptedDemand &demand, ThumbnailWorkKind kind)
{
    if (state.activeJob == std::nullopt || !m_generationProvider) {
        state.activeJob.reset();
        if (kind == ThumbnailWorkKind::Background) {
            markBackgroundBucketCompleted(state, demand.bucket);
            maybeScheduleBackgroundWork();
        } else if (hasUsableReadyImage(state)) {
            state.result.status = ActiveNavigationThumbnailResultStatus::Ready;
            state.result.imageSource = thumbnailImageSourceForId(state.imageStoreId);
        } else {
            releaseImage(state);
            state.result.status = ActiveNavigationThumbnailResultStatus::Failed;
            state.result.imageSource = QUrl();
        }
        return;
    }

    ThumbnailGenerationRequest request;
    request.localPathBytes = demand.sourcePlan.localPathBytes;
    request.originalIdentity = demand.sourcePlan.originalIdentity;
    request.openedCollectionScope = demand.sourcePlan.openedCollectionScope;
    request.sourceUrl = demand.sourceKey.url;
    request.sourceLabel = demand.sourceKey.label;
    request.sourceKind = demand.sourceKey.sourceKind;
    request.requestedBucket = demand.bucket;
    request.cacheInstallEnabled = enablesCacheInstall(demand.sourcePlan);

    const quint64 jobId = state.activeJob->id;
    qCDebug(kiriviewThumbnailLog) << "Starting thumbnail generation job" << jobId << "kind"
                                  << static_cast<int>(kind) << "number" << demand.sourceKey.number
                                  << "bucket" << static_cast<int>(demand.bucket);
    ImageIoJob job = m_generationProvider(m_owner, std::move(request),
        [this, jobId, sourceKey = demand.sourceKey, bucket = demand.bucket](
            ThumbnailGenerationResult result) mutable {
            finishGeneration(jobId, sourceKey, bucket, std::move(result));
        });
    const std::optional<std::size_t> rowIndex = rowIndexForSourceKey(demand.sourceKey);
    if (!rowIndex.has_value() || !activeJobMatches(m_rows.at(*rowIndex), jobId, demand, kind)) {
        job.cancel();
        return;
    }
    m_rows.at(*rowIndex).activeJob->job = std::move(job);
}

bool ActiveNavigationThumbnailRuntime::activeJobMatches(const RowState &state, quint64 jobId,
    const AcceptedDemand &demand, ThumbnailWorkKind kind) const
{
    return state.activeJob.has_value() && state.activeJob->id == jobId
        && state.activeJob->kind == kind && sameAcceptedDemand(state.activeJob->demand, demand);
}

bool ActiveNavigationThumbnailRuntime::backgroundBucketCompleted(
    const RowState &state, ActiveNavigationThumbnailDemandBucket bucket) const
{
    if (state.acceptedDemand.has_value() && state.acceptedDemand->bucket == bucket) {
        return true;
    }

    return std::find(state.completedBackgroundBuckets.cbegin(),
               state.completedBackgroundBuckets.cend(), bucket)
        != state.completedBackgroundBuckets.cend();
}

void ActiveNavigationThumbnailRuntime::markBackgroundBucketCompleted(
    RowState &state, ActiveNavigationThumbnailDemandBucket bucket)
{
    if (std::find(state.completedBackgroundBuckets.cbegin(),
            state.completedBackgroundBuckets.cend(), bucket)
        == state.completedBackgroundBuckets.cend()) {
        state.completedBackgroundBuckets.push_back(bucket);
    }
}

void ActiveNavigationThumbnailRuntime::maybeScheduleBackgroundWork()
{
    if (!m_backgroundArmed || hasActiveForegroundJob()) {
        return;
    }

    for (const RowState &state : m_rows) {
        if (state.activeJob.has_value()) {
            return;
        }
    }

    for (ActiveNavigationThumbnailDemandBucket bucket : backgroundFillBuckets()) {
        for (RowState &state : m_rows) {
            if (backgroundBucketCompleted(state, bucket)
                || (state.acceptedDemand.has_value()
                    && !supportsGeneratedThumbnail(state.acceptedDemand->sourcePlan))) {
                continue;
            }

            ThumbnailSourceAdapterPlan sourcePlan = sourcePlanForDemand(
                state.sourceKey, bucket, ActiveNavigationThumbnailDemandPriority::Nearby);
            if (!supportsGeneratedThumbnail(sourcePlan)) {
                continue;
            }

            qCDebug(kiriviewThumbnailLog)
                << "Scheduling background thumbnail fill" << state.sourceKey.number
                << state.sourceKey.url << "bucket" << static_cast<int>(bucket) << "generation"
                << m_navigationGeneration;
            startBackgroundWork(state, bucket, std::move(sourcePlan));
            return;
        }
    }
}

ThumbnailSourceAdapterPlan ActiveNavigationThumbnailRuntime::sourcePlanForDemand(
    const ActiveNavigationThumbnailSourceKey &sourceKey,
    ActiveNavigationThumbnailDemandBucket bucket,
    ActiveNavigationThumbnailDemandPriority priority) const
{
    if (!m_sourceAdapter) {
        return {};
    }

    return m_sourceAdapter(ThumbnailSourceAdapterRequest {
        sourceKey,
        bucket,
        priority,
    });
}

void ActiveNavigationThumbnailRuntime::startBackgroundWork(RowState &state,
    ActiveNavigationThumbnailDemandBucket bucket, ThumbnailSourceAdapterPlan sourcePlan)
{
    const AcceptedDemand demand {
        state.sourceKey,
        bucket,
        ActiveNavigationThumbnailDemandPriority::Nearby,
        std::move(sourcePlan),
    };
    state.activeJob = ActiveJobSlot {
        m_nextJobId++,
        ThumbnailWorkKind::Background,
        demand,
        {},
    };
    if (usesCacheLookup(demand.sourcePlan)) {
        startLookupJob(state, demand, ThumbnailWorkKind::Background);
    } else {
        startGenerationJob(state, demand, ThumbnailWorkKind::Background);
    }
}

void ActiveNavigationThumbnailRuntime::finishLookup(quint64 jobId,
    const ActiveNavigationThumbnailSourceKey &sourceKey,
    ActiveNavigationThumbnailDemandBucket bucket, ThumbnailCacheLookupResult lookupResult)
{
    const std::optional<std::size_t> rowIndex = rowIndexForSourceKey(sourceKey);
    if (!rowIndex.has_value()) {
        qCDebug(kiriviewThumbnailLog) << "Rejecting stale thumbnail lookup for missing source"
                                      << sourceKey.number << sourceKey.url;
        return;
    }

    RowState &state = m_rows.at(*rowIndex);
    if (!state.activeJob.has_value() || state.activeJob->id != jobId
        || !sameSourceKey(state.activeJob->demand.sourceKey, sourceKey)
        || state.activeJob->demand.bucket != bucket) {
        qCDebug(kiriviewThumbnailLog)
            << "Rejecting stale thumbnail lookup completion" << jobId << sourceKey.number
            << sourceKey.url << "bucket" << static_cast<int>(bucket);
        return;
    }

    const ThumbnailWorkKind kind = state.activeJob->kind;
    const AcceptedDemand demand = state.activeJob->demand;
    qCDebug(kiriviewThumbnailLog) << "Thumbnail lookup finished" << jobId << "kind"
                                  << static_cast<int>(kind) << "number" << sourceKey.number
                                  << "bucket" << static_cast<int>(bucket) << "status"
                                  << static_cast<int>(lookupResult.status);

    switch (lookupResult.status) {
    case ThumbnailCacheLookupStatus::Ready: {
        state.activeJob.reset();
        if (kind == ThumbnailWorkKind::Background && hasUsableReadyImage(state)) {
            markBackgroundBucketCompleted(state, bucket);
            maybeScheduleBackgroundWork();
            break;
        }

        const QString imageId = m_imageStore == nullptr
            ? QString()
            : m_imageStore->insert(
                  lookupResult.image, imageRetentionPriority(kind, demand.priority));
        if (imageId.isEmpty()) {
            if (kind == ThumbnailWorkKind::Background) {
                markBackgroundBucketCompleted(state, bucket);
                maybeScheduleBackgroundWork();
            } else if (!hasUsableReadyImage(state)) {
                markBackgroundBucketCompleted(state, bucket);
                releaseImage(state);
                state.result.status = ActiveNavigationThumbnailResultStatus::Failed;
                state.result.imageSource = QUrl();
            } else {
                markBackgroundBucketCompleted(state, bucket);
            }
            break;
        }
        releaseImage(state);
        state.imageStoreId = imageId;
        state.result.status = ActiveNavigationThumbnailResultStatus::Ready;
        state.result.imageSource = thumbnailImageSourceForId(imageId);
        qCDebug(kiriviewThumbnailLog) << "Installed thumbnail lookup result in image store"
                                      << imageId << "kind" << static_cast<int>(kind);
        markBackgroundBucketCompleted(state, bucket);
        break;
    }
    case ThumbnailCacheLookupStatus::Missing:
        if (!hasUsableReadyImage(state)) {
            if (kind == ThumbnailWorkKind::Foreground) {
                releaseImage(state);
                state.result.status = ActiveNavigationThumbnailResultStatus::Pending;
                state.result.imageSource = QUrl();
            }
        }
        state.activeJob = ActiveJobSlot {
            m_nextJobId++,
            kind,
            demand,
            {},
        };
        startGenerationJob(state, demand, kind);
        break;
    case ThumbnailCacheLookupStatus::Invalid:
    case ThumbnailCacheLookupStatus::Failed:
        state.activeJob.reset();
        if (kind == ThumbnailWorkKind::Background) {
            markBackgroundBucketCompleted(state, bucket);
            maybeScheduleBackgroundWork();
        } else if (hasUsableReadyImage(state)) {
            markBackgroundBucketCompleted(state, bucket);
            state.result.status = ActiveNavigationThumbnailResultStatus::Ready;
            state.result.imageSource = thumbnailImageSourceForId(state.imageStoreId);
        } else {
            markBackgroundBucketCompleted(state, bucket);
            releaseImage(state);
            state.result.status = ActiveNavigationThumbnailResultStatus::Failed;
            state.result.imageSource = QUrl();
        }
        break;
    }
    publishResultAt(*rowIndex);
    maybeScheduleBackgroundWork();
}

void ActiveNavigationThumbnailRuntime::finishGeneration(quint64 jobId,
    const ActiveNavigationThumbnailSourceKey &sourceKey,
    ActiveNavigationThumbnailDemandBucket bucket, ThumbnailGenerationResult generationResult)
{
    const std::optional<std::size_t> rowIndex = rowIndexForSourceKey(sourceKey);
    if (!rowIndex.has_value()) {
        qCDebug(kiriviewThumbnailLog) << "Rejecting stale thumbnail generation for missing source"
                                      << sourceKey.number << sourceKey.url;
        return;
    }

    RowState &state = m_rows.at(*rowIndex);
    if (!state.activeJob.has_value() || state.activeJob->id != jobId
        || !sameSourceKey(state.activeJob->demand.sourceKey, sourceKey)
        || state.activeJob->demand.bucket != bucket) {
        qCDebug(kiriviewThumbnailLog)
            << "Rejecting stale thumbnail generation completion" << jobId << sourceKey.number
            << sourceKey.url << "bucket" << static_cast<int>(bucket);
        return;
    }

    const ThumbnailWorkKind kind = state.activeJob->kind;
    const AcceptedDemand demand = state.activeJob->demand;
    state.activeJob.reset();
    qCDebug(kiriviewThumbnailLog) << "Thumbnail generation finished" << jobId << "kind"
                                  << static_cast<int>(kind) << "number" << sourceKey.number
                                  << "bucket" << static_cast<int>(bucket) << "status"
                                  << static_cast<int>(generationResult.status);
    switch (generationResult.status) {
    case ThumbnailGenerationStatus::Ready: {
        if (kind == ThumbnailWorkKind::Background && hasUsableReadyImage(state)) {
            markBackgroundBucketCompleted(state, bucket);
            break;
        }

        const QString imageId = m_imageStore == nullptr
            ? QString()
            : m_imageStore->insert(
                  generationResult.image, imageRetentionPriority(kind, demand.priority));
        if (imageId.isEmpty()) {
            if (kind == ThumbnailWorkKind::Background) {
                markBackgroundBucketCompleted(state, bucket);
            } else if (!hasUsableReadyImage(state)) {
                markBackgroundBucketCompleted(state, bucket);
                releaseImage(state);
                state.result.status = ActiveNavigationThumbnailResultStatus::Failed;
                state.result.imageSource = QUrl();
            } else {
                markBackgroundBucketCompleted(state, bucket);
            }
            break;
        }
        releaseImage(state);
        state.imageStoreId = imageId;
        state.result.status = ActiveNavigationThumbnailResultStatus::Ready;
        state.result.imageSource = thumbnailImageSourceForId(imageId);
        qCDebug(kiriviewThumbnailLog) << "Installed generated thumbnail in image store" << imageId
                                      << "kind" << static_cast<int>(kind);
        markBackgroundBucketCompleted(state, bucket);
        break;
    }
    case ThumbnailGenerationStatus::Failed:
        if (kind == ThumbnailWorkKind::Background) {
            markBackgroundBucketCompleted(state, bucket);
        } else if (hasUsableReadyImage(state)) {
            markBackgroundBucketCompleted(state, bucket);
            state.result.status = ActiveNavigationThumbnailResultStatus::Ready;
            state.result.imageSource = thumbnailImageSourceForId(state.imageStoreId);
        } else {
            markBackgroundBucketCompleted(state, bucket);
            releaseImage(state);
            state.result.status = ActiveNavigationThumbnailResultStatus::Failed;
            state.result.imageSource = QUrl();
        }
        break;
    }
    publishResultAt(*rowIndex);
    maybeScheduleBackgroundWork();
}

void ActiveNavigationThumbnailRuntime::publishRows()
{
    std::vector<ActiveNavigationThumbnailRow> rows;
    rows.reserve(m_rows.size());
    for (const RowState &state : m_rows) {
        rows.push_back(state.row);
    }
    m_model->setRows(std::move(rows), m_navigationGeneration);
    for (std::size_t row = 0; row < m_rows.size(); ++row) {
        publishResultAt(row);
    }
}

void ActiveNavigationThumbnailRuntime::publishResultAt(std::size_t row)
{
    const RowState &state = m_rows.at(row);
    m_model->setThumbnailResultAt(
        static_cast<int>(row), state.result.status, state.result.imageSource);
}
}
