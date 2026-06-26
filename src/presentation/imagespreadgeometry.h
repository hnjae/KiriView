// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADGEOMETRY_H
#define KIRIVIEW_IMAGESPREADGEOMETRY_H

#include <QRectF>
#include <QSize>
#include <QSizeF>

namespace kiriview {
QSize imageSpreadImageSize(QSize primarySize, QSize secondarySize);
QSizeF imageSpreadScaledPageDisplaySize(
    QSize pageSize, QSize spreadImageSize, QSizeF spreadDisplaySize);
QRectF imageSpreadPrimaryPageRect(QSizeF primaryDisplaySize, QSizeF secondaryDisplaySize,
    QSizeF spreadDisplaySize, bool rightToLeftReading);
QRectF imageSpreadSecondaryPageRect(QSizeF primaryDisplaySize, QSizeF secondaryDisplaySize,
    QSizeF spreadDisplaySize, bool rightToLeftReading);
QRectF imageSpreadVisiblePageRect(const QRectF& visibleRect, const QRectF& pageRect);
bool imageSpreadPageIsWide(QSize imageSize);
}

#endif
