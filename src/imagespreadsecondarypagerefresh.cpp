// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadsecondarypagerefresh.h"

#include "imagespreadgeometry.h"

#include <limits>

namespace {
KiriView::ImageSpreadSecondaryPageRefreshResult primaryOnlyRefresh()
{
    return { KiriView::ImageSpreadSecondaryPageRefreshAction::PrimaryOnly, {} };
}

KiriView::ImageSpreadSecondaryPageRefreshResult keepCurrentSecondaryRefresh()
{
    return { KiriView::ImageSpreadSecondaryPageRefreshAction::KeepCurrentSecondary, {} };
}

KiriView::ImageSpreadSecondaryPageRefreshResult loadTargetRefresh(const QUrl &targetUrl)
{
    return { KiriView::ImageSpreadSecondaryPageRefreshAction::LoadTarget, targetUrl };
}
}

namespace KiriView {
void ImageSpreadSecondaryPageRefresh::cachePageSize(const QUrl &url, const QSize &imageSize)
{
    m_pageCache.cachePageSize(url, imageSize);
}

std::optional<bool> ImageSpreadSecondaryPageRefresh::cachedPageIsWide(const QUrl &url) const
{
    return m_pageCache.cachedPageIsWide(url);
}

ImageSpreadSecondaryPageRefreshResult ImageSpreadSecondaryPageRefresh::planRefresh(
    const ImageSpreadSecondaryPageRefreshRequest &request) const
{
    const int currentPage = request.navigation.currentPageNumber();
    const int nextPageNumber = currentPage == std::numeric_limits<int>::max() ? 0 : currentPage + 1;
    const std::optional<QUrl> nextUrl = request.navigation.urlAtPage(nextPageNumber);
    const bool nextPageIsWide = nextUrl.has_value() && cachedPageIsWide(*nextUrl).value_or(false);
    const bool currentSecondaryMatchesNext = nextUrl.has_value() && request.secondaryPageVisible
        && request.currentSecondaryUrl == *nextUrl;

    const ImageSpreadSecondaryPageRefreshPlan plan
        = imageSpreadSecondaryPageRefreshPlan(ImageSpreadSecondaryPageRefreshState {
            request.twoPageModeActive,
            currentPage,
            request.navigation.imageCount(),
            request.primaryPageIsWide,
            nextUrl.has_value(),
            nextPageIsWide,
            currentSecondaryMatchesNext,
        });

    switch (plan.decision) {
    case ImageSpreadSecondaryPageDecision::PrimaryOnly:
        return primaryOnlyRefresh();
    case ImageSpreadSecondaryPageDecision::KeepCurrentSecondary:
        return keepCurrentSecondaryRefresh();
    case ImageSpreadSecondaryPageDecision::LoadNext:
        break;
    }

    const std::optional<QUrl> targetUrl = plan.targetPageNumber == nextPageNumber
        ? nextUrl
        : request.navigation.urlAtPage(plan.targetPageNumber);
    if (!targetUrl.has_value()) {
        return primaryOnlyRefresh();
    }

    return loadTargetRefresh(*targetUrl);
}
}
