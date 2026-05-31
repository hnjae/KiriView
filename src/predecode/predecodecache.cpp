// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodecache.h"

#include "location/imageurl.h"
#include "predecodelogging.h"
#include "predecodepolicy.h"

#include <QDebug>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>

namespace KiriView {
bool PredecodeCache::canCacheImage(const StaticImagePayload &staticImage, qsizetype byteBudget)
{
    return staticImage.byteCostWithinBudget(byteBudget).has_value();
}

PredecodeCache::PredecodeCache(qsizetype byteBudget)
    : m_byteBudget(byteBudget)
{
}

void PredecodeCache::clear()
{
    qCDebug(kiriviewPredecodeLog) << "predecode cache clear"
                                  << "windowUrls" << m_windowUrls.size() << "queuedLoads"
                                  << m_queue.size() << "cachedImages" << m_images.size();
    m_windowUrls.clear();
    m_displayedHistory.clear();
    m_queue.clear();
    m_images.clear();
}

void PredecodeCache::clearQueuedLoads()
{
    qCDebug(kiriviewPredecodeLog) << "predecode queue clear"
                                  << "queuedLoads" << m_queue.size();
    m_queue.clear();
}

void PredecodeCache::setWindowUrls(const std::vector<QUrl> &urls)
{
    m_windowUrls.clear();
    m_queue.clear();

    for (const QUrl &url : urls) {
        const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
        if (!normalizedUrl.has_value()) {
            continue;
        }
        if (containsUrl(m_windowUrls, *normalizedUrl)) {
            continue;
        }

        m_windowUrls.push_back(*normalizedUrl);
    }

    trimImagesToWindow();
    qCDebug(kiriviewPredecodeLog) << "predecode window urls set"
                                  << "requested" << urls.size() << "accepted"
                                  << m_windowUrls.size();
}

void PredecodeCache::setDisplayedUrls(const std::vector<QUrl> &urls)
{
    m_displayedHistory.setDisplayedUrls(urls);
    trimImagesToWindow();
}

void PredecodeCache::enqueueMissingWindowLoads(const QUrl &displayedUrl,
    const OpenedCollectionScopeLocation &openedCollectionScope,
    const PredecodeActiveLoads &activeLoads)
{
    const QUrl normalizedDisplayedUrl = normalizedImageUrl(displayedUrl);
    std::vector<PredecodeWindowLoadState> states;
    states.reserve(m_windowUrls.size());

    for (const QUrl &url : m_windowUrls) {
        states.push_back(PredecodeWindowLoadState {
            m_displayedHistory.currentContains(url) || url == normalizedDisplayedUrl,
            hasImage(url),
            isInFlight(url, activeLoads),
        });
    }

    const std::vector<std::size_t> missingIndices = predecodeMissingWindowLoadIndices(states);
    for (std::size_t index : missingIndices) {
        if (index < m_windowUrls.size()) {
            qCDebug(kiriviewPredecodeLog)
                << "predecode enqueue"
                << "url" << m_windowUrls.at(index) << "openedCollectionScope"
                << !openedCollectionScope.isEmpty();
            m_queue.push_back(PredecodeRequest { m_windowUrls.at(index), openedCollectionScope });
        }
    }
    qCDebug(kiriviewPredecodeLog) << "predecode enqueue missing complete"
                                  << "windowUrls" << m_windowUrls.size() << "enqueued"
                                  << missingIndices.size() << "queueSize" << m_queue.size();
}

std::optional<PredecodeRequest> PredecodeCache::takeNextRequest(
    const PredecodeActiveLoads &activeLoads)
{
    std::vector<PredecodeQueuedLoadState> states;
    states.reserve(m_queue.size());

    for (const PredecodeRequest &request : m_queue) {
        states.push_back(PredecodeQueuedLoadState {
            request.url.isValid() && !request.url.isEmpty(),
            windowContains(request.url),
            hasImage(request.url),
            activeLoads.contains(request.url),
        });
    }

    const PredecodeQueuedLoadPlan plan = predecodeNextQueuedLoadPlan(states);
    const std::size_t discardCount
        = std::min(plan.discardCount, static_cast<std::size_t>(m_queue.size()));
    if (!plan.found || plan.index >= m_queue.size()) {
        qCDebug(kiriviewPredecodeLog)
            << "predecode queue exhausted"
            << "discardCount" << discardCount << "queueSize" << m_queue.size();
        m_queue.erase(m_queue.begin(), m_queue.begin() + static_cast<std::ptrdiff_t>(discardCount));
        return std::nullopt;
    }

    auto requestEntry = m_queue.begin() + static_cast<std::ptrdiff_t>(plan.index);
    PredecodeRequest request = std::move(*requestEntry);
    qCDebug(kiriviewPredecodeLog) << "predecode dequeue"
                                  << "url" << request.url << "index" << plan.index << "discardCount"
                                  << discardCount;
    m_queue.erase(m_queue.begin(), m_queue.begin() + static_cast<std::ptrdiff_t>(discardCount));
    return request;
}

bool PredecodeCache::windowContains(const QUrl &url) const
{
    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
    return normalizedUrl.has_value() && containsUrl(m_windowUrls, *normalizedUrl);
}

bool PredecodeCache::hasImage(const QUrl &url) const
{
    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
    if (!normalizedUrl.has_value()) {
        return false;
    }

    return findCachedImage(*normalizedUrl) != m_images.cend();
}

bool PredecodeCache::isInFlight(const QUrl &url, const PredecodeActiveLoads &activeLoads) const
{
    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
    if (!normalizedUrl.has_value()) {
        return false;
    }

    return activeLoads.contains(*normalizedUrl)
        || std::any_of(
            m_queue.cbegin(), m_queue.cend(), [&normalizedUrl](const PredecodeRequest &request) {
                return request.url == *normalizedUrl;
            });
}

std::optional<PredecodedImage> PredecodeCache::findImage(const QUrl &url) const
{
    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
    if (!normalizedUrl.has_value()) {
        return std::nullopt;
    }

    const auto cached = findCachedImage(*normalizedUrl);
    if (cached == m_images.cend()) {
        return std::nullopt;
    }

    const DisplayedImageLocation location
        = DisplayedImageLocation::fromUrl(cached->url, cached->openedCollectionScope);
    return PredecodedImage { cached->staticImage, location };
}

std::optional<PredecodedImage> PredecodeCache::findImage(
    const DisplayedImageLocation &location) const
{
    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(location.imageUrl());
    if (!normalizedUrl.has_value()) {
        return std::nullopt;
    }

    const auto cached = findCachedImage(*normalizedUrl, location.openedCollectionScope());
    if (cached == m_images.cend()) {
        return std::nullopt;
    }

    return PredecodedImage { cached->staticImage,
        DisplayedImageLocation::fromUrl(cached->url, cached->openedCollectionScope) };
}

void PredecodeCache::cacheImage(const QUrl &url,
    const OpenedCollectionScopeLocation &openedCollectionScope, StaticImagePayload staticImage)
{
    const std::optional<qsizetype> byteCost = staticImage.byteCostWithinBudget(m_byteBudget);
    if (!byteCost.has_value()) {
        qCDebug(kiriviewPredecodeLog)
            << "predecode cache store skipped"
            << "reason"
            << "byte-budget"
            << "url" << url << "byteCost" << staticImage.byteCost() << "budget" << m_byteBudget;
        return;
    }

    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
    if (!normalizedUrl.has_value()
        || (!containsUrl(m_windowUrls, *normalizedUrl)
            && !m_displayedHistory.retainedContains(*normalizedUrl))) {
        qCDebug(kiriviewPredecodeLog) << "predecode cache store skipped"
                                      << "reason"
                                      << "outside-window"
                                      << "url" << url;
        return;
    }

    removeCachedImage(*normalizedUrl, openedCollectionScope);
    m_images.push_back(
        CachedImage { *normalizedUrl, openedCollectionScope, std::move(staticImage), *byteCost });
    qCDebug(kiriviewPredecodeLog) << "predecode cache stored"
                                  << "url" << *normalizedUrl << "byteCost" << *byteCost
                                  << "cachedImages" << m_images.size();

    trimImagesToWindow();
}

void PredecodeCache::cacheDisplayedImage(bool cacheable, const QUrl &url,
    const OpenedCollectionScopeLocation &openedCollectionScope, StaticImagePayload staticImage)
{
    if (!cacheable || url.isEmpty()) {
        qCDebug(kiriviewPredecodeLog)
            << "displayed predecode cache skipped"
            << "reason" << (!cacheable ? "not-cacheable" : "empty-url") << "url" << url;
        return;
    }

    cacheImage(url, openedCollectionScope, std::move(staticImage));
}

bool PredecodeCache::containsUrl(const std::vector<QUrl> &urls, const QUrl &url)
{
    return std::find(urls.cbegin(), urls.cend(), url) != urls.cend();
}

PredecodeCache::CachedImageIterator PredecodeCache::findCachedImage(const QUrl &normalizedUrl)
{
    return std::find_if(m_images.begin(), m_images.end(),
        [&normalizedUrl](const CachedImage &entry) { return entry.url == normalizedUrl; });
}

PredecodeCache::ConstCachedImageIterator PredecodeCache::findCachedImage(
    const QUrl &normalizedUrl) const
{
    return std::find_if(m_images.cbegin(), m_images.cend(),
        [&normalizedUrl](const CachedImage &entry) { return entry.url == normalizedUrl; });
}

PredecodeCache::CachedImageIterator PredecodeCache::findCachedImage(
    const QUrl &normalizedUrl, const OpenedCollectionScopeLocation &openedCollectionScope)
{
    return std::find_if(m_images.begin(), m_images.end(),
        [&normalizedUrl, &openedCollectionScope](const CachedImage &entry) {
            return entry.url == normalizedUrl
                && sameOpenedCollectionScopeLocation(
                    entry.openedCollectionScope, openedCollectionScope);
        });
}

PredecodeCache::ConstCachedImageIterator PredecodeCache::findCachedImage(
    const QUrl &normalizedUrl, const OpenedCollectionScopeLocation &openedCollectionScope) const
{
    return std::find_if(m_images.cbegin(), m_images.cend(),
        [&normalizedUrl, &openedCollectionScope](const CachedImage &entry) {
            return entry.url == normalizedUrl
                && sameOpenedCollectionScopeLocation(
                    entry.openedCollectionScope, openedCollectionScope);
        });
}

void PredecodeCache::removeCachedImage(
    const QUrl &normalizedUrl, const OpenedCollectionScopeLocation &openedCollectionScope)
{
    const auto cached = findCachedImage(normalizedUrl, openedCollectionScope);
    if (cached != m_images.end()) {
        m_images.erase(cached);
    }
}

std::size_t PredecodeCache::windowPriority(const QUrl &normalizedUrl) const
{
    const auto priorityEntry = std::find(m_windowUrls.cbegin(), m_windowUrls.cend(), normalizedUrl);
    if (priorityEntry == m_windowUrls.cend()) {
        return m_windowUrls.size();
    }

    return static_cast<std::size_t>(std::distance(m_windowUrls.cbegin(), priorityEntry));
}

void PredecodeCache::trimImagesToWindow()
{
    std::vector<PredecodeCachedImageState> states;
    states.reserve(m_images.size());

    for (const CachedImage &entry : m_images) {
        states.push_back(PredecodeCachedImageState {
            m_displayedHistory.currentContains(entry.url),
            m_displayedHistory.recentContains(entry.url),
            m_displayedHistory.currentPriority(entry.url),
            m_displayedHistory.recentPriority(entry.url),
            windowPriority(entry.url),
            entry.byteCost,
        });
    }

    const std::vector<std::size_t> retainedIndices
        = predecodeRetainedCachedImageIndices(states, m_windowUrls.size(), m_byteBudget);

    std::vector<CachedImage> retainedImages;
    retainedImages.reserve(retainedIndices.size());
    for (std::size_t index : retainedIndices) {
        if (index < m_images.size()) {
            retainedImages.push_back(std::move(m_images[index]));
        }
    }

    m_images = std::move(retainedImages);
}
}
