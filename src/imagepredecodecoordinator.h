// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPREDECODECOORDINATOR_H
#define KIRIVIEW_IMAGEPREDECODECOORDINATOR_H

#include "asyncobjectslot.h"
#include "predecodecache.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QUrl>
#include <vector>

namespace KiriView {
class ImagePredecodeCoordinator final : public QObject
{
public:
    struct Context {
        QUrl displayedUrl;
        QUrl comicBookRootUrl;
        bool displayedImageIsCacheable = false;
        QImage displayedImage;
    };

    explicit ImagePredecodeCoordinator(QObject *parent = nullptr);

    void schedule(Context context);
    void cancel();
    void clear();
    bool tryTake(const QUrl &url, QImage *image, QUrl *comicBookRootUrl) const;

private:
    void scheduleFileAdjacentImagePredecode(const Context &context, quint64 generation);
    void scheduleComicBookAdjacentImagePredecode(const Context &context, quint64 generation);
    void startPredecodeImageLoads(const std::vector<QUrl> &urls, const QUrl &comicBookRootUrl,
        const Context &context, quint64 generation);
    void startNextPredecodeImageLoad(quint64 generation);
    void startPredecodeImageLoad(const QUrl &url, const QUrl &comicBookRootUrl, quint64 generation);
    void startPredecodeImageDecode(
        QByteArray data, const QUrl &url, const QUrl &comicBookRootUrl, quint64 generation);

    AsyncObjectSlot m_listerSlot;
    AsyncObjectSlot m_listJobSlot;
    AsyncObjectSlot m_imageLoadSlot;
    PredecodeCache m_cache;
    QUrl m_activePredecodeUrl;
    quint64 m_generation = 0;
};
}

#endif
