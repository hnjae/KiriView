// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADPAGECACHE_H
#define KIRIVIEW_IMAGESPREADPAGECACHE_H

#include <QSize>
#include <QString>
#include <QUrl>
#include <map>
#include <optional>

namespace kiriview {
class ImageSpreadPageCache
{
public:
    void cachePageSize(const QUrl& url, const QSize& imageSize);
    std::optional<bool> cachedPageIsWide(const QUrl& url) const;

    static QString cacheKey(const QUrl& url);

private:
    std::map<QString, bool> m_widePageByUrl;
};
}

#endif
