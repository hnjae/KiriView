// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodecache.h"

#include "kiriimagedecoder.h"
#include "kiriimagenavigation.h"

#include <algorithm>
#include <cstddef>
#include <iterator>

namespace KiriView {
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
        const QUrl normalizedUrl = normalizedImageUrl(url);
        const bool alreadyInWindow
            = std::find(m_windowUrls.cbegin(), m_windowUrls.cend(), normalizedUrl)
            != m_windowUrls.cend();
        if (!normalizedUrl.isValid() || normalizedUrl.isEmpty() || alreadyInWindow) {
            continue;
        }

        m_windowUrls.push_back(normalizedUrl);
    }

    trimImagesToWindow();
}

void PredecodeCache::enqueueMissingWindowLoads(
    const QUrl &displayedUrl, const QUrl &comicBookRootUrl, const QUrl &activePredecodeUrl)
{
    const QUrl normalizedDisplayedUrl = normalizedImageUrl(displayedUrl);
    for (const QUrl &url : m_windowUrls) {
        if (url == normalizedDisplayedUrl) {
            continue;
        }
        if (!hasImage(url) && !isInFlight(url, activePredecodeUrl)) {
            m_queue.push_back(PredecodeRequest { url, comicBookRootUrl });
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
        m_queue.erase(m_queue.begin());

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
    const QUrl normalizedUrl = normalizedImageUrl(url);
    return std::find(m_windowUrls.cbegin(), m_windowUrls.cend(), normalizedUrl)
        != m_windowUrls.cend();
}

bool PredecodeCache::hasImage(const QUrl &url) const
{
    const QUrl normalizedUrl = normalizedImageUrl(url);
    return std::any_of(m_images.cbegin(), m_images.cend(),
        [&normalizedUrl](const PredecodedImage &entry) { return entry.url == normalizedUrl; });
}

bool PredecodeCache::isInFlight(const QUrl &url, const QUrl &activePredecodeUrl) const
{
    const QUrl normalizedUrl = normalizedImageUrl(url);
    return normalizedUrl == activePredecodeUrl
        || std::any_of(
            m_queue.cbegin(), m_queue.cend(), [&normalizedUrl](const PredecodeRequest &request) {
                return request.url == normalizedUrl;
            });
}

bool PredecodeCache::findImage(const QUrl &url, QImage *image, QUrl *comicBookRootUrl) const
{
    const QUrl normalizedUrl = normalizedImageUrl(url);
    const auto cached = std::find_if(m_images.cbegin(), m_images.cend(),
        [&normalizedUrl](const PredecodedImage &entry) { return entry.url == normalizedUrl; });
    if (cached == m_images.cend()) {
        return false;
    }

    *image = cached->image;
    *comicBookRootUrl = cached->comicBookRootUrl;
    return true;
}

void PredecodeCache::cacheImage(const QUrl &url, const QUrl &comicBookRootUrl, const QImage &image)
{
    const qsizetype byteCost = imageByteCost(image);
    if (byteCost <= 0 || byteCost > byteBudget()) {
        return;
    }

    const QUrl normalizedUrl = normalizedImageUrl(url);
    if (!windowContains(normalizedUrl)) {
        return;
    }

    m_images.erase(
        std::remove_if(m_images.begin(), m_images.end(),
            [&normalizedUrl](const PredecodedImage &entry) { return entry.url == normalizedUrl; }),
        m_images.end());
    m_images.push_back(PredecodedImage { normalizedUrl, comicBookRootUrl, image, byteCost });

    trimImagesToWindow();
}

void PredecodeCache::cacheDisplayedImage(
    bool cacheable, const QUrl &url, const QUrl &comicBookRootUrl, const QImage &image)
{
    if (!cacheable || url.isEmpty() || image.isNull()) {
        return;
    }

    cacheImage(url, comicBookRootUrl, image);
}

void PredecodeCache::trimImagesToWindow()
{
    auto priority = [this](const QUrl &url) {
        const auto priorityEntry = std::find(m_windowUrls.cbegin(), m_windowUrls.cend(), url);
        if (priorityEntry == m_windowUrls.cend()) {
            return m_windowUrls.size();
        }

        return static_cast<std::size_t>(std::distance(m_windowUrls.cbegin(), priorityEntry));
    };

    const auto outsideWindow
        = [this](const PredecodedImage &entry) { return !windowContains(entry.url); };
    m_images.erase(std::remove_if(m_images.begin(), m_images.end(), outsideWindow), m_images.end());

    std::stable_sort(m_images.begin(), m_images.end(),
        [&priority](const PredecodedImage &left, const PredecodedImage &right) {
            return priority(left.url) < priority(right.url);
        });

    qsizetype totalByteCost = 0;
    for (const PredecodedImage &entry : m_images) {
        totalByteCost += entry.byteCost;
    }

    while (totalByteCost > byteBudget() && !m_images.empty()) {
        totalByteCost -= m_images.back().byteCost;
        m_images.pop_back();
    }
}
}
