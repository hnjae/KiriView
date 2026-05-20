// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADSECONDARYPAGEREFRESH_H
#define KIRIVIEW_IMAGESPREADSECONDARYPAGEREFRESH_H

#include "navigation/imagenavigationtypes.h"
#include "presentation/imagespreadnavigation.h"
#include "presentation/imagespreadpagecache.h"

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

struct ImageSpreadPageNavigationContext {
    bool twoPageModeActive = false;
    bool secondaryPageVisible = false;
    ImagePageNavigationSnapshot navigation;
};

class ImageSpreadSecondaryPageRefresh final
{
public:
    void cachePageSize(const QUrl &url, const QSize &imageSize);
    std::optional<bool> cachedPageIsWide(const QUrl &url) const;

    ImageSpreadSecondaryPageRefreshResult planRefresh(
        const ImageSpreadSecondaryPageRefreshRequest &request) const;
    int currentLastPageNumber(const ImageSpreadPageNavigationContext &context) const;
    ImageSpreadPageNavigationTarget pageNavigationTarget(
        NavigationDirection direction, const ImageSpreadPageNavigationContext &context) const;
    int relativePageNavigationTarget(
        int offset, const ImageSpreadPageNavigationContext &context) const;
    bool shouldBeginNavigationTransition(
        int targetPageNumber, const ImageSpreadPageNavigationContext &context) const;
    bool primarySelectionMatchesDisplayed(
        const ImagePageNavigationSnapshot &navigation, const QUrl &displayedUrl) const;

private:
    ImageSpreadNavigationState navigationState(
        const ImageSpreadPageNavigationContext &context, bool previousPageIsWide = false) const;
    bool previousPageIsWideForNavigation(const ImageSpreadPageNavigationContext &context) const;
    ImageSpreadPageCache m_pageCache;
};
}

#endif
