// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADGEOMETRY_H
#define KIRIVIEW_IMAGESPREADGEOMETRY_H

#include <QRectF>
#include <QSize>
#include <QSizeF>

namespace kiriview {
QSize imageSpreadImageSize(const QSize &primarySize, const QSize &secondarySize);
QSizeF imageSpreadScaledPageDisplaySize(
    const QSize &pageSize, const QSize &spreadImageSize, const QSizeF &spreadDisplaySize);
QRectF imageSpreadPrimaryPageRect(const QSizeF &primaryDisplaySize,
    const QSizeF &secondaryDisplaySize, const QSizeF &spreadDisplaySize, bool rightToLeftReading);
QRectF imageSpreadSecondaryPageRect(const QSizeF &primaryDisplaySize,
    const QSizeF &secondaryDisplaySize, const QSizeF &spreadDisplaySize, bool rightToLeftReading);
QRectF imageSpreadVisiblePageRect(const QRectF &visibleRect, const QRectF &pageRect);
bool imageSpreadPageIsWide(const QSize &imageSize);
}

#endif
