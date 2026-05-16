// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODECACHE_H
#define KIRIVIEW_PREDECODECACHE_H

#include "imagelocation.h"
#include "predecodedimage.h"
#include "staticimage.h"

#include <QUrl>
#include <QtGlobal>
#include <cstddef>
#include <deque>
#include <optional>
#include <vector>

namespace KiriView {
struct PredecodeRequest {
    QUrl url;
    ArchiveDocumentLocation archiveDocument;
};

class PredecodeCache
{
public:
    static qsizetype preferredByteBudget();
    static qsizetype defaultByteBudget();
    static qsizetype byteBudget() { return defaultByteBudget(); }
    static qsizetype byteBudgetForSystemMemory(qsizetype systemMemoryByteSize);
    static bool canCacheImage(const StaticImagePayload &staticImage);
    static bool canCacheImage(const StaticImagePayload &staticImage, qsizetype byteBudget);

    explicit PredecodeCache(qsizetype byteBudget = defaultByteBudget());

    void clear();
    void clearQueuedLoads();
    void setWindowUrls(const std::vector<QUrl> &urls);
    void setDisplayedUrls(const std::vector<QUrl> &urls);
    void enqueueMissingWindowLoads(const QUrl &displayedUrl,
        const ArchiveDocumentLocation &archiveDocument,
        const std::vector<QUrl> &activePredecodeUrls);
    std::optional<PredecodeRequest> takeNextRequest(const std::vector<QUrl> &activePredecodeUrls);
    bool windowContains(const QUrl &url) const;
    bool hasImage(const QUrl &url) const;
    bool isInFlight(const QUrl &url, const std::vector<QUrl> &activePredecodeUrls) const;
    std::optional<PredecodedImage> findImage(const QUrl &url) const;
    void cacheImage(const QUrl &url, const ArchiveDocumentLocation &archiveDocument,
        StaticImagePayload staticImage);
    void cacheDisplayedImage(bool cacheable, const QUrl &url,
        const ArchiveDocumentLocation &archiveDocument, StaticImagePayload staticImage);

private:
    struct CachedImage {
        QUrl url;
        ArchiveDocumentLocation archiveDocument;
        StaticImagePayload staticImage;
        qsizetype byteCost = 0;
    };
    using CachedImageIterator = std::vector<CachedImage>::iterator;
    using ConstCachedImageIterator = std::vector<CachedImage>::const_iterator;

    static bool containsUrl(const std::vector<QUrl> &urls, const QUrl &url);
    static void removeUrl(std::vector<QUrl> &urls, const QUrl &url);
    bool currentDisplayedContains(const QUrl &url) const;
    bool recentDisplayedContains(const QUrl &url) const;
    bool retainedDisplayedContains(const QUrl &url) const;
    std::size_t currentDisplayedPriority(const QUrl &normalizedUrl) const;
    std::size_t recentDisplayedPriority(const QUrl &normalizedUrl) const;
    CachedImageIterator findCachedImage(const QUrl &normalizedUrl);
    ConstCachedImageIterator findCachedImage(const QUrl &normalizedUrl) const;
    void removeCachedImage(const QUrl &normalizedUrl);
    std::size_t windowPriority(const QUrl &normalizedUrl) const;
    void trimImagesToWindow();

    std::vector<QUrl> m_windowUrls;
    std::vector<QUrl> m_currentDisplayedUrls;
    std::vector<QUrl> m_recentDisplayedUrls;
    std::deque<PredecodeRequest> m_queue;
    std::vector<CachedImage> m_images;
    qsizetype m_byteBudget = defaultByteBudget();
};
}

#endif
