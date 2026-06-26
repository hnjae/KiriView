// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEVIEWPORTGEOMETRY_H
#define KIRIVIEW_IMAGEVIEWPORTGEOMETRY_H

#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>

namespace kiriview {
QRectF imageViewportImageRect(QSizeF viewportSize, QSizeF displaySize);
QPointF imageViewportMaximumContentPosition(QSizeF viewportSize, const QRectF& imageRect);
QPointF imageViewportClampedContentPosition(
    QSizeF viewportSize, const QRectF& imageRect, QPointF contentPosition);
QPointF imageViewportPanPosition(
    QSizeF viewportSize, const QRectF& imageRect, QPointF contentPosition, QPointF delta);
QPointF imageViewportNextZScanPosition(QSizeF viewportSize, const QRectF& imageRect,
    QPointF contentPosition, bool rightToLeftReading = false);
QPointF imageViewportPreviousZScanPosition(QSizeF viewportSize, const QRectF& imageRect,
    QPointF contentPosition, bool rightToLeftReading = false);
QPointF imageViewportInitialZScanPosition(
    QSizeF viewportSize, const QRectF& imageRect, bool rightToLeftReading = false);
QPointF imageViewportFinalZScanPosition(
    QSizeF viewportSize, const QRectF& imageRect, bool rightToLeftReading = false);
bool imageViewportPointInsideImage(
    QPointF contentPosition, QPointF viewportPoint, const QRectF& imageRect);
QPointF imageViewportNearestImagePoint(
    QPointF contentPosition, QPointF viewportPoint, const QRectF& imageRect);
QPointF imageViewportContentPositionForZoom(QSizeF viewportSize, QSizeF currentDisplaySize,
    QSizeF nextDisplaySize, QPointF contentPosition, QPointF viewportAnchorPoint);
QSizeF imageViewportDisplaySizeForZoom(QSize imageSize, qreal zoomPercent, qreal devicePixelRatio);
}

#endif
