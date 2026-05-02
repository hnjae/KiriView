// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEVIEWPORTGEOMETRY_H
#define KIRIVIEW_IMAGEVIEWPORTGEOMETRY_H

#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>

namespace KiriView {
QRectF imageViewportImageRect(const QSizeF &viewportSize, const QSizeF &displaySize);
QPointF imageViewportMaximumContentPosition(const QSizeF &viewportSize, const QRectF &imageRect);
QPointF imageViewportClampedContentPosition(
    const QSizeF &viewportSize, const QRectF &imageRect, const QPointF &contentPosition);
QPointF imageViewportPanPosition(const QSizeF &viewportSize, const QRectF &imageRect,
    const QPointF &contentPosition, const QPointF &delta);
QPointF imageViewportNextZScanPosition(
    const QSizeF &viewportSize, const QRectF &imageRect, const QPointF &contentPosition);
QPointF imageViewportPreviousZScanPosition(
    const QSizeF &viewportSize, const QRectF &imageRect, const QPointF &contentPosition);
QPointF imageViewportFinalZScanPosition(const QSizeF &viewportSize, const QRectF &imageRect);
bool imageViewportPointInsideImage(
    const QPointF &contentPosition, const QPointF &viewportPoint, const QRectF &imageRect);
QPointF imageViewportContentPositionForZoom(const QSizeF &viewportSize,
    const QSizeF &currentDisplaySize, const QSizeF &nextDisplaySize, const QPointF &contentPosition,
    const QPointF &viewportAnchorPoint);
QSizeF imageViewportDisplaySizeForZoom(
    const QSize &imageSize, qreal zoomPercent, qreal devicePixelRatio);
}

#endif
