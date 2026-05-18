// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodecache.h"

#include "imageurl.h"
#include "predecodepolicy.h"
#include "systemmemory.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>

namespace KiriView {
qsizetype PredecodeCache::preferredByteBudget() { return predecodePreferredByteBudget(); }

qsizetype PredecodeCache::defaultByteBudget()
{
    return byteBudgetForSystemMemory(physicalSystemMemoryByteSize().value_or(0));
}

qsizetype PredecodeCache::byteBudgetForSystemMemory(qsizetype systemMemoryByteSize)
{
    return predecodeByteBudgetForSystemMemory(systemMemoryByteSize);
}

bool PredecodeCache::canCacheImage(const StaticImagePayload &staticImage)
{
    return canCacheImage(staticImage, defaultByteBudget());
}

bool PredecodeCache::canCacheImage(const StaticImagePayload &staticImage, qsizetype byteBudget)
{
    return staticImage.byteCostWithinBudget(byteBudget).has_value();
}

PredecodeCache::PredecodeCache(qsizetype byteBudget)
    : m_byteBudget(byteBudget > 0 ? byteBudget : defaultByteBudget())
{
}

void PredecodeCache::clear()
{
    m_windowUrls.clear();
    m_displayedHistory.clear();
    m_queue.clear();
    m_images.clear();
}

void PredecodeCache::clearQueuedLoads() { m_queue.clear(); }

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
}

void PredecodeCache::setDisplayedUrls(const std::vector<QUrl> &urls)
{
    m_displayedHistory.setDisplayedUrls(urls);
    trimImagesToWindow();
}

void PredecodeCache::enqueueMissingWindowLoads(const QUrl &displayedUrl,
    const ArchiveDocumentLocation &archiveDocument, const std::vector<QUrl> &activePredecodeUrls)
{
    const QUrl normalizedDisplayedUrl = normalizedImageUrl(displayedUrl);
    std::vector<PredecodeWindowLoadState> states;
    states.reserve(m_windowUrls.size());

    for (const QUrl &url : m_windowUrls) {
        states.push_back(PredecodeWindowLoadState {
            m_displayedHistory.currentContains(url) || url == normalizedDisplayedUrl,
            hasImage(url),
            isInFlight(url, activePredecodeUrls),
        });
    }

    const std::vector<std::size_t> missingIndices = predecodeMissingWindowLoadIndices(states);
    for (std::size_t index : missingIndices) {
        if (index < m_windowUrls.size()) {
            m_queue.push_back(PredecodeRequest { m_windowUrls.at(index), archiveDocument });
        }
    }
}

std::optional<PredecodeRequest> PredecodeCache::takeNextRequest(
    const std::vector<QUrl> &activePredecodeUrls)
{
    std::vector<PredecodeQueuedLoadState> states;
    states.reserve(m_queue.size());

    for (const PredecodeRequest &request : m_queue) {
        states.push_back(PredecodeQueuedLoadState {
            request.url.isValid() && !request.url.isEmpty(),
            windowContains(request.url),
            hasImage(request.url),
            containsUrl(activePredecodeUrls, request.url),
        });
    }

    const PredecodeQueuedLoadPlan plan = predecodeNextQueuedLoadPlan(states);
    const std::size_t discardCount
        = std::min(plan.discardCount, static_cast<std::size_t>(m_queue.size()));
    if (!plan.found || plan.index >= m_queue.size()) {
        m_queue.erase(m_queue.begin(), m_queue.begin() + static_cast<std::ptrdiff_t>(discardCount));
        return std::nullopt;
    }

    auto requestEntry = m_queue.begin() + static_cast<std::ptrdiff_t>(plan.index);
    PredecodeRequest request = std::move(*requestEntry);
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

bool PredecodeCache::isInFlight(const QUrl &url, const std::vector<QUrl> &activePredecodeUrls) const
{
    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
    if (!normalizedUrl.has_value()) {
        return false;
    }

    return containsUrl(activePredecodeUrls, *normalizedUrl)
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
        = DisplayedImageLocation::fromUrl(cached->url, cached->archiveDocument);
    return PredecodedImage { cached->staticImage, location };
}

void PredecodeCache::cacheImage(
    const QUrl &url, const ArchiveDocumentLocation &archiveDocument, StaticImagePayload staticImage)
{
    const std::optional<qsizetype> byteCost = staticImage.byteCostWithinBudget(m_byteBudget);
    if (!byteCost.has_value()) {
        return;
    }

    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
    if (!normalizedUrl.has_value()
        || (!containsUrl(m_windowUrls, *normalizedUrl)
            && !m_displayedHistory.retainedContains(*normalizedUrl))) {
        return;
    }

    removeCachedImage(*normalizedUrl);
    m_images.push_back(
        CachedImage { *normalizedUrl, archiveDocument, std::move(staticImage), *byteCost });

    trimImagesToWindow();
}

void PredecodeCache::cacheDisplayedImage(bool cacheable, const QUrl &url,
    const ArchiveDocumentLocation &archiveDocument, StaticImagePayload staticImage)
{
    if (!cacheable || url.isEmpty()) {
        return;
    }

    cacheImage(url, archiveDocument, std::move(staticImage));
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

void PredecodeCache::removeCachedImage(const QUrl &normalizedUrl)
{
    const auto cached = findCachedImage(normalizedUrl);
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
