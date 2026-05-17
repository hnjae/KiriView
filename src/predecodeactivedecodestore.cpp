// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodeactivedecodestore.h"

#include "imagedecodejob.h"
#include "imageurl.h"

#include <algorithm>
#include <utility>

namespace {
std::optional<QUrl> normalizedValidUrl(const QUrl &url)
{
    const QUrl normalizedUrl = KiriView::normalizedImageUrl(url);
    if (!normalizedUrl.isValid() || normalizedUrl.isEmpty()) {
        return std::nullopt;
    }

    return normalizedUrl;
}
}

namespace KiriView {
PredecodeActiveDecodeStore::~PredecodeActiveDecodeStore() { cancel(); }

void PredecodeActiveDecodeStore::add(ImageDecodeRequest request, ImageDecodeJob *decodeJob)
{
    const std::optional<QUrl> normalizedUrl = normalizedValidUrl(request.imageUrl());
    if (request.isEmpty() || decodeJob == nullptr || !normalizedUrl.has_value()
        || containsUrl(*normalizedUrl)) {
        return;
    }

    m_entries.push_back(Entry { std::move(request), *normalizedUrl, decodeJob });
}

std::size_t PredecodeActiveDecodeStore::size() const { return m_entries.size(); }

bool PredecodeActiveDecodeStore::containsUrl(const QUrl &url) const
{
    const std::optional<QUrl> normalizedUrl = normalizedValidUrl(url);
    if (!normalizedUrl.has_value()) {
        return false;
    }

    return std::any_of(m_entries.cbegin(), m_entries.cend(),
        [&normalizedUrl](const Entry &entry) { return entry.normalizedUrl == *normalizedUrl; });
}

std::vector<QUrl> PredecodeActiveDecodeStore::urls() const
{
    std::vector<QUrl> urls;
    urls.reserve(m_entries.size());
    for (const Entry &entry : m_entries) {
        urls.push_back(entry.normalizedUrl);
    }
    return urls;
}

std::optional<ImageDecodeRequest> PredecodeActiveDecodeStore::finish(
    const ImageDecodeRequest &request)
{
    const auto entry = std::find_if(m_entries.begin(), m_entries.end(),
        [&request](const Entry &candidate) { return candidate.request.matches(request); });
    if (entry == m_entries.end()) {
        return std::nullopt;
    }

    ImageDecodeRequest finishedRequest = entry->request;
    if (entry->decodeJob != nullptr) {
        entry->decodeJob->deleteLater();
    }
    m_entries.erase(entry);
    return finishedRequest;
}

void PredecodeActiveDecodeStore::cancel()
{
    for (const Entry &entry : m_entries) {
        if (entry.decodeJob == nullptr) {
            continue;
        }

        entry.decodeJob->cancel();
        entry.decodeJob->deleteLater();
    }
    m_entries.clear();
}
}
