// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadsecondarypagerefresh.h"

#include "imageurl.h"
#include "presentation/imagespreadgeometry.h"

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

int ImageSpreadSecondaryPageRefresh::currentLastPageNumber(
    const ImageSpreadPageNavigationContext &context) const
{
    return imageSpreadNavigationCurrentLastPageNumber(navigationState(context));
}

ImageSpreadPageNavigationTarget ImageSpreadSecondaryPageRefresh::pageNavigationTarget(
    NavigationDirection direction, const ImageSpreadPageNavigationContext &context) const
{
    ImageSpreadNavigationState state = navigationState(context);
    if (direction == NavigationDirection::Previous) {
        state.previousPageIsWide = previousPageIsWideForNavigation(context);
    }

    return imageSpreadPageNavigationTarget(direction, state);
}

int ImageSpreadSecondaryPageRefresh::relativePageNavigationTarget(
    int offset, const ImageSpreadPageNavigationContext &context) const
{
    return imageSpreadRelativePageNavigationTarget(navigationState(context), offset);
}

bool ImageSpreadSecondaryPageRefresh::shouldBeginNavigationTransition(
    int targetPageNumber, const ImageSpreadPageNavigationContext &context) const
{
    return imageSpreadShouldBeginNavigationTransition(navigationState(context), targetPageNumber);
}

bool ImageSpreadSecondaryPageRefresh::primarySelectionMatchesDisplayed(
    const ImagePageNavigationSnapshot &navigation, const QUrl &displayedUrl) const
{
    const int pageNumber = navigation.currentPageNumber();
    if (pageNumber <= 0) {
        return true;
    }

    const std::optional<QUrl> pageUrl = navigation.urlAtPage(pageNumber);
    if (!pageUrl.has_value()) {
        return true;
    }

    return sameNormalizedUrl(*pageUrl, displayedUrl);
}

ImageSpreadNavigationState ImageSpreadSecondaryPageRefresh::navigationState(
    const ImageSpreadPageNavigationContext &context, bool previousPageIsWide) const
{
    return ImageSpreadNavigationState { context.twoPageModeActive,
        context.navigation.currentPageNumber(), context.navigation.imageCount(),
        context.secondaryPageVisible, previousPageIsWide };
}

bool ImageSpreadSecondaryPageRefresh::previousPageIsWideForNavigation(
    const ImageSpreadPageNavigationContext &context) const
{
    const int pageNumber = context.navigation.currentPageNumber();
    if (!context.secondaryPageVisible || pageNumber <= 2) {
        return false;
    }

    const std::optional<QUrl> previousUrl = context.navigation.urlAtPage(pageNumber - 1);
    return previousUrl.has_value() ? cachedPageIsWide(*previousUrl).value_or(false) : false;
}
}
