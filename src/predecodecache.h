// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODECACHE_H
#define KIRIVIEW_PREDECODECACHE_H

#include <QImage>
#include <QUrl>
#include <QtGlobal>
#include <deque>
#include <optional>
#include <vector>

namespace KiriView {
struct PredecodeRequest {
    QUrl url;
    QUrl comicBookRootUrl;
};

class PredecodeCache
{
public:
    static constexpr qsizetype defaultByteBudget() { return 512 * 1024 * 1024; }
    static constexpr qsizetype byteBudget() { return defaultByteBudget(); }

    explicit PredecodeCache(qsizetype byteBudget = defaultByteBudget());

    void clear();
    void clearQueuedLoads();
    void setWindowUrls(const std::vector<QUrl> &urls);
    void enqueueMissingWindowLoads(
        const QUrl &displayedUrl, const QUrl &comicBookRootUrl, const QUrl &activePredecodeUrl);
    std::optional<PredecodeRequest> takeNextRequest(const QUrl &activePredecodeUrl);
    bool windowContains(const QUrl &url) const;
    bool hasImage(const QUrl &url) const;
    bool isInFlight(const QUrl &url, const QUrl &activePredecodeUrl) const;
    bool findImage(const QUrl &url, QImage *image, QUrl *comicBookRootUrl) const;
    void cacheImage(const QUrl &url, const QUrl &comicBookRootUrl, const QImage &image);
    void cacheDisplayedImage(
        bool cacheable, const QUrl &url, const QUrl &comicBookRootUrl, const QImage &image);

private:
    struct PredecodedImage {
        QUrl url;
        QUrl comicBookRootUrl;
        QImage image;
        qsizetype byteCost = 0;
    };

    void trimImagesToWindow();

    std::vector<QUrl> m_windowUrls;
    std::deque<PredecodeRequest> m_queue;
    std::vector<PredecodedImage> m_images;
    qsizetype m_byteBudget = defaultByteBudget();
};
}

#endif
