// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadgeometry.h"

#include "bridge/qtgeometryconversion.h"
#include "kiriview/src/policy/imagespreadgeometry.cxx.h"

namespace {
KiriView::ImageSpreadSecondaryPageDecision imageSpreadSecondaryPageDecision(
    KiriView::RustImageSpreadSecondaryPageDecision decision)
{
    switch (decision) {
    case KiriView::RustImageSpreadSecondaryPageDecision::PrimaryOnly:
        return KiriView::ImageSpreadSecondaryPageDecision::PrimaryOnly;
    case KiriView::RustImageSpreadSecondaryPageDecision::LoadNext:
        return KiriView::ImageSpreadSecondaryPageDecision::LoadNext;
    case KiriView::RustImageSpreadSecondaryPageDecision::KeepCurrentSecondary:
        return KiriView::ImageSpreadSecondaryPageDecision::KeepCurrentSecondary;
    }

    return KiriView::ImageSpreadSecondaryPageDecision::PrimaryOnly;
}

KiriView::RustImageSpreadSecondaryPageRefreshState rustSecondaryPageRefreshState(
    const KiriView::ImageSpreadSecondaryPageRefreshState &state)
{
    return KiriView::RustImageSpreadSecondaryPageRefreshState { state.twoPageModeActive,
        state.currentPageNumber, state.imageCount, state.primaryPageIsWide, state.nextPageAvailable,
        state.nextPageIsWide, state.currentSecondaryMatchesNext };
}

KiriView::RustImageSpreadReadingAvailability rustReadingAvailability(
    const KiriView::ImageSpreadReadingAvailability &availability)
{
    return KiriView::RustImageSpreadReadingAvailability { availability.hasImage,
        availability.hasDisplayedImage, availability.displayedDocumentIsComicBook };
}
}

namespace KiriView {
QSize imageSpreadImageSize(const QSize &primarySize, const QSize &secondarySize)
{
    return Bridge::qtSize(
        rustImageSpreadImageSize(Bridge::rustSize<RustImageSpreadSize>(primarySize),
            Bridge::rustSize<RustImageSpreadSize>(secondarySize)));
}

QSizeF imageSpreadScaledPageDisplaySize(
    const QSize &pageSize, const QSize &spreadImageSize, const QSizeF &spreadDisplaySize)
{
    return Bridge::qtSizeF(
        rustImageSpreadScaledPageDisplaySize(Bridge::rustSize<RustImageSpreadSize>(pageSize),
            Bridge::rustSize<RustImageSpreadSize>(spreadImageSize),
            Bridge::rustSizeF<RustImageSpreadSizeF>(spreadDisplaySize)));
}

QRectF imageSpreadPrimaryPageRect(const QSizeF &primaryDisplaySize,
    const QSizeF &secondaryDisplaySize, const QSizeF &spreadDisplaySize, bool rightToLeftReading)
{
    return Bridge::qtRectF(
        rustImageSpreadPrimaryPageRect(Bridge::rustSizeF<RustImageSpreadSizeF>(primaryDisplaySize),
            Bridge::rustSizeF<RustImageSpreadSizeF>(secondaryDisplaySize),
            Bridge::rustSizeF<RustImageSpreadSizeF>(spreadDisplaySize), rightToLeftReading));
}

QRectF imageSpreadSecondaryPageRect(const QSizeF &primaryDisplaySize,
    const QSizeF &secondaryDisplaySize, const QSizeF &spreadDisplaySize, bool rightToLeftReading)
{
    return Bridge::qtRectF(rustImageSpreadSecondaryPageRect(
        Bridge::rustSizeF<RustImageSpreadSizeF>(primaryDisplaySize),
        Bridge::rustSizeF<RustImageSpreadSizeF>(secondaryDisplaySize),
        Bridge::rustSizeF<RustImageSpreadSizeF>(spreadDisplaySize), rightToLeftReading));
}

QRectF imageSpreadVisiblePageRect(const QRectF &visibleRect, const QRectF &pageRect)
{
    return Bridge::qtRectF(
        rustImageSpreadVisiblePageRect(Bridge::rustRectF<RustImageSpreadRectF>(visibleRect),
            Bridge::rustRectF<RustImageSpreadRectF>(pageRect)));
}

bool imageSpreadPageIsWide(const QSize &imageSize)
{
    return rustImageSpreadPageIsWide(Bridge::rustSize<RustImageSpreadSize>(imageSize));
}

ImageSpreadSecondaryPageRefreshPlan imageSpreadSecondaryPageRefreshPlan(
    const ImageSpreadSecondaryPageRefreshState &state)
{
    const RustImageSpreadSecondaryPageRefreshPlan plan
        = rustImageSpreadSecondaryPageRefreshPlan(rustSecondaryPageRefreshState(state));
    return ImageSpreadSecondaryPageRefreshPlan {
        ::imageSpreadSecondaryPageDecision(plan.decision),
        plan.target_page_number,
    };
}

bool imageSpreadReadingControlsAvailable(const ImageSpreadReadingAvailability &availability)
{
    return rustImageSpreadReadingControlsAvailable(rustReadingAvailability(availability));
}

ImageSpreadTwoPageModeChange imageSpreadTwoPageModeChange(
    bool currentEnabled, bool nextEnabled, bool secondaryPageVisible)
{
    const RustImageSpreadTwoPageModeChange change
        = rustImageSpreadTwoPageModeChange(currentEnabled, nextEnabled, secondaryPageVisible);
    return ImageSpreadTwoPageModeChange { change.changed, change.reset_spread_zoom,
        change.finish_transition, change.clear_secondary_page, change.restore_primary_zoom,
        change.refresh_secondary_page, change.notify_two_page_mode };
}
}
