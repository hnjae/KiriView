// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "thumbnailimagestore.h"

#include "cache/imagebyteaccounting.h"
#include "cache/imagebytecost.h"
#include "cache/imagecachepolicy.h"
#include "session/thumbnaillogging.h"

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace kiriview {
namespace {
    int priorityRank(ThumbnailImageRetentionPriority priority)
    {
        switch (priority) {
        case ThumbnailImageRetentionPriority::Background:
            return 0;
        case ThumbnailImageRetentionPriority::Nearby:
            return 1;
        case ThumbnailImageRetentionPriority::Visible:
            return 2;
        }

        return 0;
    }

    qsizetype defaultThumbnailStoreByteBudget() { return thumbnailCachePreferredByteBudget(); }
}

class ThumbnailImageStore::Private
{
public:
    struct Entry {
        QString id;
        QImage image;
        qsizetype byteCost = 0;
        quint64 lastUse = 0;
        ThumbnailImageRetentionPriority priority = ThumbnailImageRetentionPriority::Nearby;
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

    void trimToBudget()
    {
        while (byteCost > byteBudget && !images.empty()) {
            auto removable = std::min_element(
                images.begin(), images.end(), [](const Entry &left, const Entry &right) {
                    const int leftPriority = priorityRank(left.priority);
                    const int rightPriority = priorityRank(right.priority);
                    if (leftPriority != rightPriority) {
                        return leftPriority < rightPriority;
                    }
                    return left.lastUse < right.lastUse;
                });
            if (removable == images.end()) {
                return;
            }

            qCDebug(kiriviewThumbnailLog)
                << "Evicting thumbnail image from store" << removable->id << "priority"
                << priorityRank(removable->priority) << "bytes" << removable->byteCost;
            byteCost -= removable->byteCost;
            images.erase(removable);
        }
    }
};

ThumbnailImageStore::ThumbnailImageStore(qsizetype byteBudget)
    : d(std::make_unique<Private>())
{
    d->byteBudget = byteBudget > 0 ? byteBudget : defaultThumbnailStoreByteBudget();
}

ThumbnailImageStore::~ThumbnailImageStore() = default;

QString ThumbnailImageStore::insert(QImage image, ThumbnailImageRetentionPriority priority)
{
    if (image.isNull()) {
        return {};
    }

    const qsizetype byteCost = imageByteCost(image);
    QMutexLocker locker(&d->mutex);
    if (byteCost <= 0 || byteCost > d->byteBudget) {
        return {};
    }

    QString id;
    do {
        id = QStringLiteral("thumbnail-%1").arg(d->nextId++);
        if (d->nextId == 0) {
            ++d->nextId;
        }
    } while (d->findEntry(id) != d->images.end());

    d->images.push_back(Private::Entry {
        id,
        std::move(image),
        byteCost,
        ++d->useClock,
        priority,
    });
    d->byteCost = saturatedQtByteSum(d->byteCost, byteCost);
    d->trimToBudget();
    return d->findEntry(id) == d->images.end() ? QString() : id;
}

void ThumbnailImageStore::updatePriority(
    const QString &id, ThumbnailImageRetentionPriority priority)
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

void ThumbnailImageStore::release(const QString &id)
{
    if (id.isEmpty()) {
        return;
    }

    QMutexLocker locker(&d->mutex);
    auto entry = d->findEntry(id);
    if (entry == d->images.end()) {
        return;
    }

    d->byteCost -= entry->byteCost;
    d->images.erase(entry);
}

void ThumbnailImageStore::clear()
{
    QMutexLocker locker(&d->mutex);
    d->images.clear();
    d->byteCost = 0;
}

void ThumbnailImageStore::setByteBudget(qsizetype byteBudget)
{
    QMutexLocker locker(&d->mutex);
    d->byteBudget = byteBudget > 0 ? byteBudget : defaultThumbnailStoreByteBudget();
    d->trimToBudget();
}

QImage ThumbnailImageStore::image(const QString &id) const
{
    QMutexLocker locker(&d->mutex);
    auto entry = d->findEntry(id);
    if (entry == d->images.end()) {
        return {};
    }

    entry->lastUse = ++d->useClock;
    return entry->image;
}

qsizetype ThumbnailImageStore::byteBudget() const
{
    QMutexLocker locker(&d->mutex);
    return d->byteBudget;
}

qsizetype ThumbnailImageStore::byteCost() const
{
    QMutexLocker locker(&d->mutex);
    return d->byteCost;
}

qsizetype ThumbnailImageStore::size() const
{
    QMutexLocker locker(&d->mutex);
    return static_cast<qsizetype>(d->images.size());
}

ThumbnailImageProvider::ThumbnailImageProvider(std::shared_ptr<ThumbnailImageStore> store)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_store(std::move(store))
{
}

QImage ThumbnailImageProvider::requestImage(
    const QString &id, QSize *size, const QSize &requestedSize)
{
    const QImage image = m_store == nullptr ? QImage() : m_store->image(id);
    if (size != nullptr) {
        *size = image.size();
    }
    if (image.isNull() || !requestedSize.isValid() || requestedSize.isEmpty()) {
        return image;
    }

    return image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

std::shared_ptr<ThumbnailImageStore> sharedThumbnailImageStore()
{
    static const std::shared_ptr<ThumbnailImageStore> store
        = std::make_shared<ThumbnailImageStore>(defaultThumbnailStoreByteBudget());
    return store;
}

void configureSharedThumbnailImageStoreByteBudget(qsizetype byteBudget)
{
    sharedThumbnailImageStore()->setByteBudget(byteBudget);
}

QUrl thumbnailImageSourceForId(const QString &id)
{
    if (id.isEmpty()) {
        return {};
    }

    return QUrl(QStringLiteral("image://kiriview-thumbnails/%1").arg(id));
}
}
