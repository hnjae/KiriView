// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadpagecache.h"

#include "location/imageurl.h"
#include "presentation/imagespreadgeometry.h"

namespace kiriview {
void ImageSpreadPageCache::cachePageSize(const QUrl& url, QSize imageSize)
{
    const QString key = cacheKey(url);
    if (key.isEmpty() || imageSize.isEmpty()) {
        return;
    }

    m_widePageByUrl[key] = imageSpreadPageIsWide(imageSize);
}

std::optional<bool> ImageSpreadPageCache::cachedPageIsWide(const QUrl& url) const
{
    const QString key = cacheKey(url);
    if (key.isEmpty()) {
        return std::nullopt;
    }

    const auto cached = m_widePageByUrl.find(key);
    if (cached == m_widePageByUrl.cend()) {
        return std::nullopt;
    }

    return cached->second;
}

QString ImageSpreadPageCache::cacheKey(const QUrl& url) { return normalizedUrlIdentityKey(url); }
}
