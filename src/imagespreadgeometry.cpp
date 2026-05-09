// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadgeometry.h"

#include "kiriview/src/imagespreadgeometry.cxx.h"
#include "qtgeometryconversion.h"

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

int imageSpreadPreviousPageTarget(
    int currentPageNumber, bool secondaryPageVisible, bool previousPageIsWide)
{
    return rustImageSpreadPreviousPageTarget(
        currentPageNumber, secondaryPageVisible, previousPageIsWide);
}

int imageSpreadCurrentLastPageNumber(int currentPageNumber, bool secondaryPageVisible)
{
    return rustImageSpreadCurrentLastPageNumber(currentPageNumber, secondaryPageVisible);
}

int imageSpreadRelativePageTarget(int currentPageNumber, int imageCount, int offset)
{
    return rustImageSpreadRelativePageTarget(currentPageNumber, imageCount, offset);
}

int imageSpreadNextPageTarget(int currentLastPageNumber, int imageCount)
{
    return rustImageSpreadNextPageTarget(currentLastPageNumber, imageCount);
}

bool imageSpreadShouldBeginTransition(
    bool twoPageModeActive, int currentPageNumber, int targetPageNumber, int imageCount)
{
    return rustImageSpreadShouldBeginTransition(
        twoPageModeActive, currentPageNumber, targetPageNumber, imageCount);
}

ImageSpreadSecondaryPageDecision imageSpreadSecondaryPageDecision(bool twoPageModeActive,
    int currentPageNumber, int imageCount, bool primaryPageIsWide, bool nextPageAvailable,
    bool nextPageIsWide, bool currentSecondaryMatchesNext)
{
    return ::imageSpreadSecondaryPageDecision(
        rustImageSpreadSecondaryPageDecision(twoPageModeActive, currentPageNumber, imageCount,
            primaryPageIsWide, nextPageAvailable, nextPageIsWide, currentSecondaryMatchesNext));
}
}
