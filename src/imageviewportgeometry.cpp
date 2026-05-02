// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageviewportgeometry.h"

#include "kiriview/src/imageviewportgeometry.cxx.h"

namespace {
KiriView::RustSize rustSize(const QSize &size)
{
    return KiriView::RustSize { size.width(), size.height() };
}

KiriView::RustSizeF rustSizeF(const QSizeF &size)
{
    return KiriView::RustSizeF { size.width(), size.height() };
}

KiriView::RustPointF rustPointF(const QPointF &point)
{
    return KiriView::RustPointF { point.x(), point.y() };
}

KiriView::RustRectF rustRectF(const QRectF &rect)
{
    return KiriView::RustRectF { rect.x(), rect.y(), rect.width(), rect.height() };
}

QSizeF qtSizeF(const KiriView::RustSizeF &size) { return QSizeF(size.width, size.height); }

QPointF qtPointF(const KiriView::RustPointF &point) { return QPointF(point.x, point.y); }

QRectF qtRectF(const KiriView::RustRectF &rect)
{
    return QRectF(rect.x, rect.y, rect.width, rect.height);
}
}

namespace KiriView {
QRectF imageViewportImageRect(const QSizeF &viewportSize, const QSizeF &displaySize)
{
    return qtRectF(rustImageViewportImageRect(rustSizeF(viewportSize), rustSizeF(displaySize)));
}

QPointF imageViewportMaximumContentPosition(const QSizeF &viewportSize, const QRectF &imageRect)
{
    return qtPointF(
        rustImageViewportMaximumContentPosition(rustSizeF(viewportSize), rustRectF(imageRect)));
}

QPointF imageViewportClampedContentPosition(
    const QSizeF &viewportSize, const QRectF &imageRect, const QPointF &contentPosition)
{
    return qtPointF(rustImageViewportClampedContentPosition(
        rustSizeF(viewportSize), rustRectF(imageRect), rustPointF(contentPosition)));
}

QPointF imageViewportPanPosition(const QSizeF &viewportSize, const QRectF &imageRect,
    const QPointF &contentPosition, const QPointF &delta)
{
    return qtPointF(rustImageViewportPanPosition(rustSizeF(viewportSize), rustRectF(imageRect),
        rustPointF(contentPosition), rustPointF(delta)));
}

QPointF imageViewportNextZScanPosition(
    const QSizeF &viewportSize, const QRectF &imageRect, const QPointF &contentPosition)
{
    return qtPointF(rustImageViewportNextZScanPosition(
        rustSizeF(viewportSize), rustRectF(imageRect), rustPointF(contentPosition)));
}

QPointF imageViewportPreviousZScanPosition(
    const QSizeF &viewportSize, const QRectF &imageRect, const QPointF &contentPosition)
{
    return qtPointF(rustImageViewportPreviousZScanPosition(
        rustSizeF(viewportSize), rustRectF(imageRect), rustPointF(contentPosition)));
}

QPointF imageViewportFinalZScanPosition(const QSizeF &viewportSize, const QRectF &imageRect)
{
    return qtPointF(
        rustImageViewportFinalZScanPosition(rustSizeF(viewportSize), rustRectF(imageRect)));
}

bool imageViewportPointInsideImage(
    const QPointF &contentPosition, const QPointF &viewportPoint, const QRectF &imageRect)
{
    return rustImageViewportPointInsideImage(
        rustPointF(contentPosition), rustPointF(viewportPoint), rustRectF(imageRect));
}

QPointF imageViewportContentPositionForZoom(const QSizeF &viewportSize,
    const QSizeF &currentDisplaySize, const QSizeF &nextDisplaySize, const QPointF &contentPosition,
    const QPointF &viewportAnchorPoint)
{
    return qtPointF(rustImageViewportContentPositionForZoom(rustSizeF(viewportSize),
        rustSizeF(currentDisplaySize), rustSizeF(nextDisplaySize), rustPointF(contentPosition),
        rustPointF(viewportAnchorPoint)));
}

QSizeF imageViewportDisplaySizeForZoom(
    const QSize &imageSize, qreal zoomPercent, qreal devicePixelRatio)
{
    return qtSizeF(
        rustImageViewportDisplaySizeForZoom(rustSize(imageSize), zoomPercent, devicePixelRatio));
}
}
