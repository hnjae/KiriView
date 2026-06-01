// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_THUMBNAILIMAGESTORE_H
#define KIRIVIEW_THUMBNAILIMAGESTORE_H

#include <QImage>
#include <QQuickImageProvider>
#include <QUrl>
#include <QtGlobal>
#include <memory>

namespace KiriView {
class ThumbnailImageStore final
{
public:
    ThumbnailImageStore();
    ~ThumbnailImageStore();

    QString insert(QImage image);
    void release(const QString &id);
    void clear();
    QImage image(const QString &id) const;
    qsizetype size() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

class ThumbnailImageProvider final : public QQuickImageProvider
{
public:
    explicit ThumbnailImageProvider(std::shared_ptr<ThumbnailImageStore> store);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    std::shared_ptr<ThumbnailImageStore> m_store;
};

std::shared_ptr<ThumbnailImageStore> sharedThumbnailImageStore();
QUrl thumbnailImageSourceForId(const QString &id);
}

#endif
