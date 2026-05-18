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

struct ImageSpreadSecondaryPageRefreshState {
    bool twoPageModeActive = false;
    int currentPageNumber = 0;
    int imageCount = 0;
    bool primaryPageIsWide = false;
    bool nextPageAvailable = false;
    bool nextPageIsWide = false;
    bool currentSecondaryMatchesNext = false;
};

struct ImageSpreadSecondaryPageRefreshPlan {
    ImageSpreadSecondaryPageDecision decision = ImageSpreadSecondaryPageDecision::PrimaryOnly;
    int targetPageNumber = 0;
};

struct ImageSpreadReadingAvailability {
    bool hasImage = false;
    bool hasDisplayedImage = false;
    bool displayedDocumentIsComicBook = false;
};

struct ImageSpreadTwoPageModeChange {
    bool changed = false;
    bool resetSpreadZoom = false;
    bool finishTransition = false;
    bool clearSecondaryPage = false;
    bool restorePrimaryZoom = false;
    bool refreshSecondaryPage = false;
    bool notifyTwoPageMode = false;
};

QSize imageSpreadImageSize(const QSize &primarySize, const QSize &secondarySize);
QSizeF imageSpreadScaledPageDisplaySize(
    const QSize &pageSize, const QSize &spreadImageSize, const QSizeF &spreadDisplaySize);
QRectF imageSpreadPrimaryPageRect(const QSizeF &primaryDisplaySize,
    const QSizeF &secondaryDisplaySize, const QSizeF &spreadDisplaySize, bool rightToLeftReading);
QRectF imageSpreadSecondaryPageRect(const QSizeF &primaryDisplaySize,
    const QSizeF &secondaryDisplaySize, const QSizeF &spreadDisplaySize, bool rightToLeftReading);
QRectF imageSpreadVisiblePageRect(const QRectF &visibleRect, const QRectF &pageRect);
bool imageSpreadPageIsWide(const QSize &imageSize);
ImageSpreadSecondaryPageRefreshPlan imageSpreadSecondaryPageRefreshPlan(
    const ImageSpreadSecondaryPageRefreshState &state);
bool imageSpreadReadingControlsAvailable(const ImageSpreadReadingAvailability &availability);
ImageSpreadTwoPageModeChange imageSpreadTwoPageModeChange(
    bool currentEnabled, bool nextEnabled, bool secondaryPageVisible);
}

#endif
