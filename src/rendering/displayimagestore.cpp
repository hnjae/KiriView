// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "displayimagestore.h"

#include "cache/imagebyteaccounting.h"
#include "cache/imagebytecost.h"
#include "cache/imagecachepolicy.h"
#include "displayproviderlogging.h"
#include "system/systemmemory.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QtGlobal>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace kiriview {
namespace {
    int priorityRank(DisplayImageRetentionPriority priority)
    {
        switch (priority) {
        case DisplayImageRetentionPriority::Background:
            return 0;
        case DisplayImageRetentionPriority::Nearby:
            return 1;
        case DisplayImageRetentionPriority::Visible:
            return 2;
        }

        return 0;
    }

    qsizetype defaultDisplayStoreByteBudget()
    {
        return resolvedImageCacheBudgets(ImageCacheBudgetRequest {}, systemMemorySnapshot())
            .displayImageCacheByteBudget;
    }

    QSize normalizedOriginalSize(const DisplayImageEntry &entry)
    {
        if (entry.originalSize.isValid() && !entry.originalSize.isEmpty()) {
            return entry.originalSize;
        }
        if (entry.rasterSize.isValid() && !entry.rasterSize.isEmpty()) {
            return entry.rasterSize;
        }
        return entry.image.size();
    }

    QSize normalizedRasterSize(const DisplayImageEntry &entry)
    {
        if (entry.rasterSize.isValid() && !entry.rasterSize.isEmpty()) {
            return entry.rasterSize;
        }
        return entry.image.size();
    }

    QSize oneDimensionalDownscaleSize(
        const QSize &imageSize, int requestedWidth, int requestedHeight)
    {
        if (requestedWidth > 0) {
            if (requestedWidth >= imageSize.width()) {
                return imageSize;
            }
            const int targetHeight = std::max(1,
                qRound(
                    static_cast<qreal>(requestedWidth) * imageSize.height() / imageSize.width()));
            return QSize(requestedWidth, std::min(targetHeight, imageSize.height()));
        }

        if (requestedHeight >= imageSize.height()) {
            return imageSize;
        }
        const int targetWidth = std::max(1,
            qRound(static_cast<qreal>(requestedHeight) * imageSize.width() / imageSize.height()));
        return QSize(std::min(targetWidth, imageSize.width()), requestedHeight);
    }

    QSize downscaleTargetSize(const QSize &imageSize, const QSize &requestedSize)
    {
        if (imageSize.isEmpty()) {
            return imageSize;
        }

        const int requestedWidth = requestedSize.width();
        const int requestedHeight = requestedSize.height();
        if (requestedWidth < 0 || requestedHeight < 0
            || (requestedWidth == 0 && requestedHeight == 0)) {
            return imageSize;
        }

        if (requestedWidth == 0 || requestedHeight == 0) {
            return oneDimensionalDownscaleSize(imageSize, requestedWidth, requestedHeight);
        }

        if (requestedWidth >= imageSize.width() && requestedHeight >= imageSize.height()) {
            return imageSize;
        }

        const QSize target = imageSize.scaled(requestedSize, Qt::KeepAspectRatio);
        if (target.isEmpty()
            || (target.width() >= imageSize.width() && target.height() >= imageSize.height())) {
            return imageSize;
        }
        return target;
    }
}

class DisplayImageStore::Private
{
public:
    struct Entry {
        QString id;
        QImage image;
        QSize originalSize;
        QSize rasterSize;
        QString sourceIdentity;
        DisplayedPageRole pageRole = DisplayedPageRole::Primary;
        DisplayImageQuality quality = DisplayImageQuality::Exact;
        DisplayImageRetentionPriority priority = DisplayImageRetentionPriority::Nearby;
        qsizetype byteCost = 0;
        quint64 generation = 0;
        QString debugLabel;
        DisplayImagePreviewOrigin previewOrigin = DisplayImagePreviewOrigin::None;
        quint64 lastUse = 0;
        bool releaseRequested = false;
        int visiblePins = 0;
        int staleRetainedPins = 0;
        int pendingLoadPins = 0;
        int frameRetentionPins = 0;

        int totalPinCount() const
        {
            return visiblePins + staleRetainedPins + pendingLoadPins + frameRetentionPins;
        }
    };

    mutable QMutex mutex;
    std::vector<Entry> images;
    qsizetype byteBudget = 0;
    qsizetype byteCost = 0;
    mutable quint64 useClock = 0;
    quint64 nextId = 1;

    std::vector<Entry>::iterator findEntry(const QString &id)
    {
        return std::find_if(
            images.begin(), images.end(), [&id](const Entry &entry) { return entry.id == id; });
    }

    std::vector<Entry>::const_iterator findEntry(const QString &id) const
    {
        return std::find_if(
            images.cbegin(), images.cend(), [&id](const Entry &entry) { return entry.id == id; });
    }

    int &pinCount(Entry &entry, DisplayImagePinKind kind)
    {
        switch (kind) {
        case DisplayImagePinKind::Visible:
            return entry.visiblePins;
        case DisplayImagePinKind::StaleRetained:
            return entry.staleRetainedPins;
        case DisplayImagePinKind::PendingLoad:
            return entry.pendingLoadPins;
        case DisplayImagePinKind::FrameRetention:
            return entry.frameRetentionPins;
        }

        return entry.visiblePins;
    }

    DisplayImageStoreEntry publicEntry(const Entry &entry) const
    {
        return DisplayImageStoreEntry {
            entry.id,
            entry.image,
            entry.originalSize,
            entry.rasterSize,
            entry.sourceIdentity,
            entry.pageRole,
            entry.quality,
            entry.priority,
            entry.byteCost,
            entry.generation,
            entry.debugLabel,
            entry.previewOrigin,
        };
    }

    void removeEntry(std::vector<Entry>::iterator entry)
    {
        byteCost -= entry->byteCost;
        images.erase(entry);
    }

    void trimToBudget()
    {
        while (byteCost > byteBudget) {
            auto removable = images.end();
            for (auto candidate = images.begin(); candidate != images.end(); ++candidate) {
                if (candidate->totalPinCount() > 0) {
                    continue;
                }
                if (removable == images.end()) {
                    removable = candidate;
                    continue;
                }

                const int candidatePriority = priorityRank(candidate->priority);
                const int removablePriority = priorityRank(removable->priority);
                if (candidatePriority < removablePriority
                    || (candidatePriority == removablePriority
                        && candidate->lastUse < removable->lastUse)) {
                    removable = candidate;
                }
            }

            if (removable == images.end()) {
                return;
            }
            removeEntry(removable);
        }
    }
};

DisplayImageStore::DisplayImageStore(qsizetype byteBudget)
    : d(std::make_unique<Private>())
{
    d->byteBudget = byteBudget > 0 ? byteBudget : defaultDisplayStoreByteBudget();
}

DisplayImageStore::~DisplayImageStore() = default;

QString DisplayImageStore::insert(DisplayImageEntry entry)
{
    if (entry.image.isNull()) {
        return {};
    }

    const qsizetype byteCost = imageByteCost(entry.image);
    QMutexLocker locker(&d->mutex);
    if (byteCost <= 0 || byteCost > d->byteBudget) {
        return {};
    }

    QString id;
    do {
        id = QStringLiteral("display-%1").arg(d->nextId++);
        if (d->nextId == 0) {
            ++d->nextId;
        }
    } while (d->findEntry(id) != d->images.end());

    const QSize originalSize = normalizedOriginalSize(entry);
    const QSize rasterSize = normalizedRasterSize(entry);

    d->images.push_back(Private::Entry {
        id,
        std::move(entry.image),
        originalSize,
        rasterSize,
        std::move(entry.sourceIdentity),
        entry.pageRole,
        entry.quality,
        entry.priority,
        byteCost,
        entry.generation,
        std::move(entry.debugLabel),
        entry.previewOrigin,
        ++d->useClock,
    });
    d->byteCost = saturatedQtByteSum(d->byteCost, byteCost);
    d->trimToBudget();
    return d->findEntry(id) == d->images.end() ? QString() : id;
}

std::optional<DisplayImageStoreEntry> DisplayImageStore::entry(const QString &id) const
{
    if (id.isEmpty()) {
        return std::nullopt;
    }

    QMutexLocker locker(&d->mutex);
    auto entry = d->findEntry(id);
    if (entry == d->images.end()) {
        return std::nullopt;
    }

    entry->lastUse = ++d->useClock;
    return d->publicEntry(*entry);
}

void DisplayImageStore::updatePriority(const QString &id, DisplayImageRetentionPriority priority)
{
    if (id.isEmpty()) {
        return;
    }

    QMutexLocker locker(&d->mutex);
    auto entry = d->findEntry(id);
    if (entry == d->images.end()) {
        return;
    }

    entry->priority = priority;
    entry->lastUse = ++d->useClock;
    d->trimToBudget();
}

bool DisplayImageStore::acquirePinLease(const QString &id, DisplayImagePinKind kind)
{
    if (id.isEmpty()) {
        return false;
    }

    QMutexLocker locker(&d->mutex);
    auto entry = d->findEntry(id);
    if (entry == d->images.end()) {
        return false;
    }

    ++d->pinCount(*entry, kind);
    entry->lastUse = ++d->useClock;
    return true;
}

void DisplayImageStore::releasePinLease(const QString &id, DisplayImagePinKind kind)
{
    if (id.isEmpty()) {
        return;
    }

    QMutexLocker locker(&d->mutex);
    auto entry = d->findEntry(id);
    if (entry == d->images.end()) {
        return;
    }

    int &pinCount = d->pinCount(*entry, kind);
    if (pinCount > 0) {
        --pinCount;
    }
    entry->lastUse = ++d->useClock;
    if (entry->releaseRequested && entry->totalPinCount() == 0) {
        d->removeEntry(entry);
        return;
    }
    d->trimToBudget();
}

void DisplayImageStore::release(const QString &id)
{
    if (id.isEmpty()) {
        return;
    }

    QMutexLocker locker(&d->mutex);
    auto entry = d->findEntry(id);
    if (entry == d->images.end()) {
        return;
    }

    if (entry->totalPinCount() > 0) {
        entry->releaseRequested = true;
        entry->lastUse = ++d->useClock;
        return;
    }

    d->removeEntry(entry);
}

void DisplayImageStore::clear()
{
    QMutexLocker locker(&d->mutex);
    d->images.clear();
    d->byteCost = 0;
}

qsizetype DisplayImageStore::byteBudget() const
{
    QMutexLocker locker(&d->mutex);
    return d->byteBudget;
}

qsizetype DisplayImageStore::byteCost() const
{
    QMutexLocker locker(&d->mutex);
    return d->byteCost;
}

qsizetype DisplayImageStore::size() const
{
    QMutexLocker locker(&d->mutex);
    return static_cast<qsizetype>(d->images.size());
}

DisplayImageProvider::DisplayImageProvider(std::shared_ptr<DisplayImageStore> store)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_store(std::move(store))
{
}

QImage DisplayImageProvider::requestImage(
    const QString &id, QSize *size, const QSize &requestedSize)
{
    const bool logRequest = kiriviewDisplayProviderLog().isDebugEnabled();
    QElapsedTimer elapsedTimer;
    if (logRequest) {
        elapsedTimer.start();
    }

    const std::optional<DisplayImageStoreEntry> entry
        = m_store == nullptr ? std::nullopt : m_store->entry(id);
    if (!entry.has_value()) {
        if (size != nullptr) {
            *size = {};
        }
        if (logRequest) {
            const qint64 elapsedUs = elapsedTimer.nsecsElapsed() / 1000;
            qCDebug(kiriviewDisplayProviderLog)
                << "display provider request" << "id" << id << "originalSize" << QSize()
                << "requestedSize" << requestedSize << "returnedSize" << QSize() << "null" << true
                << "elapsedUs" << elapsedUs << "elapsedMs" << elapsedUs / 1000.0;
        }
        return {};
    }

    if (size != nullptr) {
        *size = entry->originalSize;
    }

    const QSize targetSize = downscaleTargetSize(entry->image.size(), requestedSize);
    QImage result;
    if (targetSize == entry->image.size()) {
        result = entry->image;
    } else {
        result = entry->image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    if (logRequest) {
        const qint64 elapsedUs = elapsedTimer.nsecsElapsed() / 1000;
        qCDebug(kiriviewDisplayProviderLog)
            << "display provider request" << "id" << id << "originalSize" << entry->originalSize
            << "requestedSize" << requestedSize << "returnedSize" << result.size() << "null"
            << result.isNull() << "elapsedUs" << elapsedUs << "elapsedMs" << elapsedUs / 1000.0;
    }

    return result;
}

std::shared_ptr<DisplayImageStore> sharedDisplayImageStore()
{
    static const std::shared_ptr<DisplayImageStore> store
        = std::make_shared<DisplayImageStore>(defaultDisplayStoreByteBudget());
    return store;
}

QUrl displayImageSourceForId(const QString &id)
{
    if (id.isEmpty()) {
        return {};
    }

    return QUrl(QStringLiteral("image://kiriview-images/%1").arg(id));
}
}
