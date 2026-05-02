// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPREDECODECOORDINATOR_H
#define KIRIVIEW_IMAGEPREDECODECOORDINATOR_H

#include "imageiojob.h"
#include "imagelocation.h"
#include "predecodecache.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
class ImagePredecodeCoordinator final : public QObject
{
public:
    struct Context {
        DisplayedImageLocation displayedImageLocation;
        bool displayedImageIsCacheable = false;
        QImage displayedImage;
    };

    explicit ImagePredecodeCoordinator(QObject *parent = nullptr);

    void schedule(Context context);
    void cancel();
    void clear();
    std::optional<PredecodedImage> tryTake(const QUrl &url) const;

private:
    void scheduleFileAdjacentImagePredecode(const Context &context, quint64 generation);
    void scheduleComicBookAdjacentImagePredecode(const Context &context, quint64 generation);
    void startPredecodeImageLoads(const std::vector<QUrl> &urls, const QUrl &comicBookRootUrl,
        const Context &context, quint64 generation);
    void startNextPredecodeImageLoad(quint64 generation);
    void startPredecodeImageLoad(const QUrl &url, const QUrl &comicBookRootUrl, quint64 generation);
    void startPredecodeImageDecode(
        QByteArray data, const QUrl &url, const QUrl &comicBookRootUrl, quint64 generation);

    ImageIoJob m_listerJob;
    ImageIoJob m_listJob;
    ImageIoJob m_imageLoadJob;
    PredecodeCache m_cache;
    QUrl m_activePredecodeUrl;
    quint64 m_generation = 0;
};
}

#endif
