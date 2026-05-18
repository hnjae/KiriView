// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADSECONDARYPAGEREFRESH_H
#define KIRIVIEW_IMAGESPREADSECONDARYPAGEREFRESH_H

#include "imagenavigationtypes.h"
#include "imagespreadpagecache.h"

#include <QSize>
#include <QUrl>
#include <optional>

namespace KiriView {
enum class ImageSpreadSecondaryPageRefreshAction {
    PrimaryOnly,
    KeepCurrentSecondary,
    LoadTarget,
};

struct ImageSpreadSecondaryPageRefreshRequest {
    bool twoPageModeActive = false;
    bool primaryPageIsWide = false;
    bool secondaryPageVisible = false;
    QUrl currentSecondaryUrl;
    ImagePageNavigationSnapshot navigation;
};

struct ImageSpreadSecondaryPageRefreshResult {
    ImageSpreadSecondaryPageRefreshAction action
        = ImageSpreadSecondaryPageRefreshAction::PrimaryOnly;
    QUrl targetUrl;
};

class ImageSpreadSecondaryPageRefresh final
{
public:
    void cachePageSize(const QUrl &url, const QSize &imageSize);
    std::optional<bool> cachedPageIsWide(const QUrl &url) const;

    ImageSpreadSecondaryPageRefreshResult planRefresh(
        const ImageSpreadSecondaryPageRefreshRequest &request) const;

private:
    ImageSpreadPageCache m_pageCache;
};
}

#endif
