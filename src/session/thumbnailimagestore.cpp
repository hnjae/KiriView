// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "thumbnailimagestore.h"

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <memory>
#include <utility>

namespace KiriView {
class ThumbnailImageStore::Private
{
public:
    mutable QMutex mutex;
    QHash<QString, QImage> images;
    quint64 nextId = 1;
};

ThumbnailImageStore::ThumbnailImageStore()
    : d(std::make_unique<Private>())
{
}

ThumbnailImageStore::~ThumbnailImageStore() = default;

QString ThumbnailImageStore::insert(QImage image)
{
    if (image.isNull()) {
        return {};
    }

    QMutexLocker locker(&d->mutex);
    QString id;
    do {
        id = QStringLiteral("thumbnail-%1").arg(d->nextId++);
        if (d->nextId == 0) {
            ++d->nextId;
        }
    } while (d->images.contains(id));

    d->images.insert(id, std::move(image));
    return id;
}

void ThumbnailImageStore::release(const QString &id)
{
    if (id.isEmpty()) {
        return;
    }

    QMutexLocker locker(&d->mutex);
    d->images.remove(id);
}

void ThumbnailImageStore::clear()
{
    QMutexLocker locker(&d->mutex);
    d->images.clear();
}

QImage ThumbnailImageStore::image(const QString &id) const
{
    QMutexLocker locker(&d->mutex);
    return d->images.value(id);
}

qsizetype ThumbnailImageStore::size() const
{
    QMutexLocker locker(&d->mutex);
    return d->images.size();
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
        = std::make_shared<ThumbnailImageStore>();
    return store;
}

QUrl thumbnailImageSourceForId(const QString &id)
{
    if (id.isEmpty()) {
        return {};
    }

    return QUrl(QStringLiteral("image://kiriview-thumbnails/%1").arg(id));
}
}
