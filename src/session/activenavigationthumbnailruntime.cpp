// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "activenavigationthumbnailruntime.h"

#include <QFile>
#include <utility>

namespace {
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
}

namespace KiriView {
ActiveNavigationThumbnailRuntime::ActiveNavigationThumbnailRuntime(QObject *owner,
    ThumbnailCacheLookupProvider lookupProvider, std::shared_ptr<ThumbnailImageStore> imageStore,
    ThumbnailGenerationProvider generationProvider)
    : m_owner(owner)
    , m_model(std::make_unique<ActiveNavigationThumbnailModel>(owner))
    , m_lookupProvider(
          lookupProvider ? std::move(lookupProvider) : defaultThumbnailCacheLookupProvider())
    , m_generationProvider(
          generationProvider ? std::move(generationProvider) : defaultThumbnailGenerationProvider())
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
    if (bucket == ActiveNavigationThumbnailDemandBucket::None || navigationGeneration == 0
        || navigationGeneration != m_navigationGeneration) {
        return false;
    }

    const std::optional<std::size_t> rowIndex
        = rowIndexForIdentity(number, url, navigationGeneration);
    if (!rowIndex.has_value()) {
        return false;
    }

    RowState &state = m_rows.at(*rowIndex);
    const AcceptedDemand demand {
        state.sourceKey,
        bucket,
        priority,
    };
    if (state.acceptedDemand.has_value() && sameAcceptedDemand(*state.acceptedDemand, demand)) {
        return false;
    }

    cancelActiveJob(state);
    state.acceptedDemand = demand;
    releaseImage(state);
    state.result.imageSource = QUrl();
    if (supportsGeneratedThumbnail(state.sourceKey.sourceKind)
        && state.sourceKey.url.isLocalFile()) {
        state.result.status = ActiveNavigationThumbnailResultStatus::Pending;
        state.activeJob = ActiveJobSlot {
            m_nextJobId++,
            demand,
            {},
        };
        startLookupJob(state, demand);
    } else {
        state.result.status = ActiveNavigationThumbnailResultStatus::Unsupported;
        state.activeJob.reset();
    }
    publishResultAt(*rowIndex);
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
    releaseImage(state);
    state.result = completion.result;
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

bool ActiveNavigationThumbnailRuntime::sameAcceptedDemand(
    const AcceptedDemand &left, const AcceptedDemand &right)
{
    return sameSourceKey(left.sourceKey, right.sourceKey) && left.bucket == right.bucket
        && left.priority == right.priority;
}

bool ActiveNavigationThumbnailRuntime::supportsGeneratedThumbnail(
    ActiveNavigationThumbnailSourceKind sourceKind)
{
    return sourceKind == ActiveNavigationThumbnailSourceKind::DirectImage;
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

    m_canceledJobIds.push_back(state.activeJob->id);
    state.activeJob->job.cancel();
    state.activeJob.reset();
}

void ActiveNavigationThumbnailRuntime::cancelAllActiveJobs()
{
    for (RowState &state : m_rows) {
        cancelActiveJob(state);
    }
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

void ActiveNavigationThumbnailRuntime::startLookupJob(RowState &state, const AcceptedDemand &demand)
{
    if (state.activeJob == std::nullopt || !m_lookupProvider) {
        return;
    }

    ThumbnailCacheLookupRequest request;
    request.localPathBytes = QFile::encodeName(demand.sourceKey.url.toLocalFile());
    request.requestedBucket = demand.bucket;

    const quint64 jobId = state.activeJob->id;
    ImageIoJob job = m_lookupProvider(m_owner, std::move(request),
        [this, jobId, sourceKey = demand.sourceKey, bucket = demand.bucket](
            ThumbnailCacheLookupResult result) mutable {
            finishLookup(jobId, sourceKey, bucket, std::move(result));
        });
    const std::optional<std::size_t> rowIndex = rowIndexForSourceKey(demand.sourceKey);
    if (!rowIndex.has_value() || !activeJobMatches(m_rows.at(*rowIndex), jobId, demand)) {
        job.cancel();
        return;
    }
    m_rows.at(*rowIndex).activeJob->job = std::move(job);
}

void ActiveNavigationThumbnailRuntime::startGenerationJob(
    RowState &state, const AcceptedDemand &demand)
{
    if (state.activeJob == std::nullopt || !m_generationProvider) {
        state.activeJob.reset();
        state.result.status = ActiveNavigationThumbnailResultStatus::Failed;
        state.result.imageSource = QUrl();
        return;
    }

    ThumbnailGenerationRequest request;
    request.localPathBytes = QFile::encodeName(demand.sourceKey.url.toLocalFile());
    request.requestedBucket = demand.bucket;

    const quint64 jobId = state.activeJob->id;
    ImageIoJob job = m_generationProvider(m_owner, std::move(request),
        [this, jobId, sourceKey = demand.sourceKey, bucket = demand.bucket](
            ThumbnailGenerationResult result) mutable {
            finishGeneration(jobId, sourceKey, bucket, std::move(result));
        });
    const std::optional<std::size_t> rowIndex = rowIndexForSourceKey(demand.sourceKey);
    if (!rowIndex.has_value() || !activeJobMatches(m_rows.at(*rowIndex), jobId, demand)) {
        job.cancel();
        return;
    }
    m_rows.at(*rowIndex).activeJob->job = std::move(job);
}

bool ActiveNavigationThumbnailRuntime::activeJobMatches(
    const RowState &state, quint64 jobId, const AcceptedDemand &demand) const
{
    return state.activeJob.has_value() && state.activeJob->id == jobId
        && sameAcceptedDemand(state.activeJob->demand, demand);
}

void ActiveNavigationThumbnailRuntime::finishLookup(quint64 jobId,
    const ActiveNavigationThumbnailSourceKey &sourceKey,
    ActiveNavigationThumbnailDemandBucket bucket, ThumbnailCacheLookupResult lookupResult)
{
    const std::optional<std::size_t> rowIndex = rowIndexForSourceKey(sourceKey);
    if (!rowIndex.has_value()) {
        return;
    }

    RowState &state = m_rows.at(*rowIndex);
    if (!state.acceptedDemand.has_value()
        || !sameSourceKey(state.acceptedDemand->sourceKey, sourceKey)
        || state.acceptedDemand->bucket != bucket
        || !activeJobMatches(state, jobId, *state.acceptedDemand)) {
        return;
    }

    releaseImage(state);
    state.result.imageSource = QUrl();
    switch (lookupResult.status) {
    case ThumbnailCacheLookupStatus::Ready: {
        state.activeJob.reset();
        const QString imageId
            = m_imageStore == nullptr ? QString() : m_imageStore->insert(lookupResult.image);
        if (imageId.isEmpty()) {
            state.result.status = ActiveNavigationThumbnailResultStatus::Failed;
            break;
        }
        state.imageStoreId = imageId;
        state.result.status = ActiveNavigationThumbnailResultStatus::Ready;
        state.result.imageSource = thumbnailImageSourceForId(imageId);
        break;
    }
    case ThumbnailCacheLookupStatus::Missing:
        state.result.status = ActiveNavigationThumbnailResultStatus::Pending;
        state.activeJob = ActiveJobSlot {
            m_nextJobId++,
            *state.acceptedDemand,
            {},
        };
        startGenerationJob(state, *state.acceptedDemand);
        break;
    case ThumbnailCacheLookupStatus::Invalid:
    case ThumbnailCacheLookupStatus::Failed:
        state.activeJob.reset();
        state.result.status = ActiveNavigationThumbnailResultStatus::Failed;
        break;
    }
    publishResultAt(*rowIndex);
}

void ActiveNavigationThumbnailRuntime::finishGeneration(quint64 jobId,
    const ActiveNavigationThumbnailSourceKey &sourceKey,
    ActiveNavigationThumbnailDemandBucket bucket, ThumbnailGenerationResult generationResult)
{
    const std::optional<std::size_t> rowIndex = rowIndexForSourceKey(sourceKey);
    if (!rowIndex.has_value()) {
        return;
    }

    RowState &state = m_rows.at(*rowIndex);
    if (!state.acceptedDemand.has_value()
        || !sameSourceKey(state.acceptedDemand->sourceKey, sourceKey)
        || state.acceptedDemand->bucket != bucket
        || !activeJobMatches(state, jobId, *state.acceptedDemand)) {
        return;
    }

    releaseImage(state);
    state.activeJob.reset();
    state.result.imageSource = QUrl();
    switch (generationResult.status) {
    case ThumbnailGenerationStatus::Ready: {
        const QString imageId
            = m_imageStore == nullptr ? QString() : m_imageStore->insert(generationResult.image);
        if (imageId.isEmpty()) {
            state.result.status = ActiveNavigationThumbnailResultStatus::Failed;
            break;
        }
        state.imageStoreId = imageId;
        state.result.status = ActiveNavigationThumbnailResultStatus::Ready;
        state.result.imageSource = thumbnailImageSourceForId(imageId);
        break;
    }
    case ThumbnailGenerationStatus::Failed:
        state.result.status = ActiveNavigationThumbnailResultStatus::Failed;
        break;
    }
    publishResultAt(*rowIndex);
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
