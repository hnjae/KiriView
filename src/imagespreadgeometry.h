// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADGEOMETRY_H
#define KIRIVIEW_IMAGESPREADGEOMETRY_H

#include <QRectF>
#include <QSize>
#include <QSizeF>

namespace KiriView {
enum class ImageSpreadSecondaryPageDecision {
    PrimaryOnly,
    LoadNext,
    KeepCurrentSecondary,
};

QSize imageSpreadImageSize(const QSize &primarySize, const QSize &secondarySize);
QSizeF imageSpreadScaledPageDisplaySize(
    const QSize &pageSize, const QSize &spreadImageSize, const QSizeF &spreadDisplaySize);
QRectF imageSpreadPrimaryPageRect(const QSizeF &primaryDisplaySize,
    const QSizeF &secondaryDisplaySize, const QSizeF &spreadDisplaySize, bool rightToLeftReading);
QRectF imageSpreadSecondaryPageRect(const QSizeF &primaryDisplaySize,
    const QSizeF &secondaryDisplaySize, const QSizeF &spreadDisplaySize, bool rightToLeftReading);
bool imageSpreadPageIsWide(const QSize &imageSize);
int imageSpreadPreviousPageTarget(
    int currentPageNumber, bool secondaryPageVisible, bool previousPageIsWide);
int imageSpreadCurrentLastPageNumber(int currentPageNumber, bool secondaryPageVisible);
int imageSpreadRelativePageTarget(int currentPageNumber, int imageCount, int offset);
int imageSpreadNextPageTarget(int currentLastPageNumber, int imageCount);
bool imageSpreadShouldBeginTransition(
    bool twoPageModeActive, int currentPageNumber, int targetPageNumber, int imageCount);
ImageSpreadSecondaryPageDecision imageSpreadSecondaryPageDecision(bool twoPageModeActive,
    int currentPageNumber, int imageCount, bool primaryPageIsWide, bool nextPageAvailable,
    bool nextPageIsWide, bool currentSecondaryMatchesNext);
}

#endif
