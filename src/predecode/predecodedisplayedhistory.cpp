// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodedisplayedhistory.h"

#include "location/imageurl.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>

namespace {
constexpr std::size_t recentDisplayedCacheLimit = 4;
}

namespace kiriview {
void PredecodeDisplayedHistory::clear()
{
    m_currentUrls.clear();
    m_recentUrls.clear();
}

void PredecodeDisplayedHistory::setDisplayedUrls(const std::vector<QUrl> &urls)
{
    std::vector<QUrl> displayedUrls;

    for (const QUrl &url : urls) {
        const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
        if (!normalizedUrl.has_value() || containsUrl(displayedUrls, *normalizedUrl)) {
            continue;
        }

        displayedUrls.push_back(*normalizedUrl);
    }

    for (const QUrl &url : m_currentUrls) {
        if (containsUrl(displayedUrls, url)) {
            continue;
        }

        removeUrl(m_recentUrls, url);
        m_recentUrls.insert(m_recentUrls.begin(), url);
    }

    for (const QUrl &url : displayedUrls) {
        removeUrl(m_recentUrls, url);
    }
    if (m_recentUrls.size() > recentDisplayedCacheLimit) {
        m_recentUrls.resize(recentDisplayedCacheLimit);
    }

    m_currentUrls = std::move(displayedUrls);
}

bool PredecodeDisplayedHistory::currentContains(const QUrl &url) const
{
    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
    return normalizedUrl.has_value() && containsUrl(m_currentUrls, *normalizedUrl);
}

bool PredecodeDisplayedHistory::recentContains(const QUrl &url) const
{
    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
    return normalizedUrl.has_value() && containsUrl(m_recentUrls, *normalizedUrl);
}

bool PredecodeDisplayedHistory::retainedContains(const QUrl &url) const
{
    return currentContains(url) || recentContains(url);
}

std::size_t PredecodeDisplayedHistory::currentPriority(const QUrl &url) const
{
    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
    return normalizedUrl.has_value() ? priority(m_currentUrls, *normalizedUrl)
                                     : m_currentUrls.size();
}

std::size_t PredecodeDisplayedHistory::recentPriority(const QUrl &url) const
{
    const std::optional<QUrl> normalizedUrl = normalizedValidImageUrl(url);
    return normalizedUrl.has_value() ? priority(m_recentUrls, *normalizedUrl) : m_recentUrls.size();
}

const std::vector<QUrl> &PredecodeDisplayedHistory::currentUrls() const { return m_currentUrls; }

const std::vector<QUrl> &PredecodeDisplayedHistory::recentUrls() const { return m_recentUrls; }

bool PredecodeDisplayedHistory::containsUrl(const std::vector<QUrl> &urls, const QUrl &url)
{
    return std::find(urls.cbegin(), urls.cend(), url) != urls.cend();
}

void PredecodeDisplayedHistory::removeUrl(std::vector<QUrl> &urls, const QUrl &url)
{
    urls.erase(std::remove(urls.begin(), urls.end(), url), urls.end());
}

std::size_t PredecodeDisplayedHistory::priority(const std::vector<QUrl> &urls, const QUrl &url)
{
    const auto priorityEntry = std::find(urls.cbegin(), urls.cend(), url);
    if (priorityEntry == urls.cend()) {
        return urls.size();
    }

    return static_cast<std::size_t>(std::distance(urls.cbegin(), priorityEntry));
}
}
