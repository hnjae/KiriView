// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODECACHE_H
#define KIRIVIEW_PREDECODECACHE_H

#include "imagelocation.h"

#include <QImage>
#include <QUrl>
#include <QtGlobal>
#include <cstddef>
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
    static constexpr qsizetype preferredByteBudget() { return 1024 * 1024 * 1024; }
    static qsizetype defaultByteBudget();
    static qsizetype byteBudget() { return defaultByteBudget(); }
    static qsizetype byteBudgetForSystemMemory(qsizetype systemMemoryByteSize);

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
    std::optional<PredecodedImage> findImage(const QUrl &url) const;
    void cacheImage(const QUrl &url, const QUrl &comicBookRootUrl, const QImage &image);
    void cacheDisplayedImage(
        bool cacheable, const QUrl &url, const QUrl &comicBookRootUrl, const QImage &image);

private:
    struct CachedImage {
        QUrl url;
        QUrl comicBookRootUrl;
        QImage image;
        qsizetype byteCost = 0;
    };
    using CachedImageIterator = std::vector<CachedImage>::iterator;
    using ConstCachedImageIterator = std::vector<CachedImage>::const_iterator;

    static bool containsUrl(const std::vector<QUrl> &urls, const QUrl &url);
    CachedImageIterator findCachedImage(const QUrl &normalizedUrl);
    ConstCachedImageIterator findCachedImage(const QUrl &normalizedUrl) const;
    void removeCachedImage(const QUrl &normalizedUrl);
    std::size_t windowPriority(const QUrl &normalizedUrl) const;
    void trimImagesToWindow();

    std::vector<QUrl> m_windowUrls;
    std::deque<PredecodeRequest> m_queue;
    std::vector<CachedImage> m_images;
    qsizetype m_byteBudget = defaultByteBudget();
};
}

#endif
