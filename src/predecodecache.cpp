// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodecache.h"

#include "imagebytecost.h"
#include "imageurl.h"
#include "kiriview/src/predecodecachepolicy.cxx.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>

namespace {
std::optional<QUrl> normalizedValidImageUrl(const QUrl &url)
{
    const QUrl normalizedUrl = KiriView::normalizedImageUrl(url);
    if (!normalizedUrl.isValid() || normalizedUrl.isEmpty()) {
        return std::nullopt;
    }

    return normalizedUrl;
}

qsizetype qtByteSize(std::int64_t byteSize) { return static_cast<qsizetype>(byteSize); }

std::int64_t rustByteSize(qsizetype byteSize) { return static_cast<std::int64_t>(byteSize); }
}

namespace KiriView {
qsizetype PredecodeCache::preferredByteBudget()
{
    return qtByteSize(rustPredecodePreferredByteBudget());
}

qsizetype PredecodeCache::defaultByteBudget()
{
    return byteBudgetForSystemMemory(systemMemoryByteSize().value_or(0));
}

qsizetype PredecodeCache::byteBudgetForSystemMemory(qsizetype systemMemoryByteSize)
{
    return qtByteSize(rustPredecodeByteBudgetForSystemMemory(rustByteSize(systemMemoryByteSize)));
}

bool PredecodeCache::canCacheImage(const StaticImagePayload &staticImage)
{
    return canCacheImage(staticImage, defaultByteBudget());
}

bool PredecodeCache::canCacheImage(const StaticImagePayload &staticImage, qsizetype byteBudget)
{
    return cacheableByteCost(staticImage, byteBudget).has_value();
}

PredecodeCache::PredecodeCache(qsizetype byteBudget)
    : m_byteBudget(byteBudget > 0 ? byteBudget : defaultByteBudget())
{
}

void PredecodeCache::clear()
{
    m_windowUrls.clear();
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

void PredecodeCache::enqueueMissingWindowLoads(const QUrl &displayedUrl,
    const ArchiveDocumentLocation &archiveDocument, const QUrl &activePredecodeUrl)
{
    const QUrl normalizedDisplayedUrl = normalizedImageUrl(displayedUrl);
    for (const QUrl &url : m_windowUrls) {
        if (url == normalizedDisplayedUrl) {
            continue;
        }
        if (!hasImage(url) && !isInFlight(url, activePredecodeUrl)) {
            m_queue.push_back(PredecodeRequest { url, archiveDocument });
        }
    }
}

std::optional<PredecodeRequest> PredecodeCache::takeNextRequest(const QUrl &activePredecodeUrl)
{
    if (!activePredecodeUrl.isEmpty()) {
        return std::nullopt;
    }

    while (!m_queue.empty()) {
        PredecodeRequest request = std::move(m_queue.front());
        m_queue.pop_front();

        if (!request.url.isValid() || request.url.isEmpty() || !windowContains(request.url)
            || hasImage(request.url)) {
            continue;
        }

        return request;
    }

    return std::nullopt;
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

bool PredecodeCache::isInFlight(const QUrl &url, const QUrl &activePredecodeUrl) const
{
    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
    if (!normalizedUrl.has_value()) {
        return false;
    }

    return *normalizedUrl == activePredecodeUrl
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
    const std::optional<qsizetype> byteCost = cacheableByteCost(staticImage, m_byteBudget);
    if (!byteCost.has_value()) {
        return;
    }

    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
    if (!normalizedUrl.has_value() || !containsUrl(m_windowUrls, *normalizedUrl)) {
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

std::optional<qsizetype> PredecodeCache::cacheableByteCost(
    const StaticImagePayload &staticImage, qsizetype byteBudget)
{
    const RustPredecodeCacheableByteCost byteCost = rustPredecodeCacheableByteCost(
        rustByteSize(staticImage.byteCost()), rustByteSize(byteBudget));
    if (!byteCost.cacheable) {
        return std::nullopt;
    }

    return qtByteSize(byteCost.byte_cost);
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
    rust::Vec<std::size_t> windowPriorities;
    rust::Vec<std::int64_t> byteCosts;
    windowPriorities.reserve(m_images.size());
    byteCosts.reserve(m_images.size());

    for (const CachedImage &entry : m_images) {
        windowPriorities.push_back(windowPriority(entry.url));
        byteCosts.push_back(rustByteSize(entry.byteCost));
    }

    const rust::Vec<std::size_t> retainedIndices
        = rustPredecodeRetainedCachedImageIndices(std::move(windowPriorities), std::move(byteCosts),
            m_windowUrls.size(), rustByteSize(m_byteBudget));

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
