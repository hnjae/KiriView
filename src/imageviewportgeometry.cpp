// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageviewportgeometry.h"

#include <algorithm>
#include <cmath>

namespace {
qreal normalizedLength(qreal length)
{
    return std::isfinite(length) ? std::max<qreal>(0.0, length) : 0.0;
}

QSizeF normalizedSize(const QSizeF &size)
{
    return QSizeF(normalizedLength(size.width()), normalizedLength(size.height()));
}

QPointF normalizedPoint(const QPointF &point, const QPointF &fallback)
{
    return QPointF(std::isfinite(point.x()) ? point.x() : fallback.x(),
        std::isfinite(point.y()) ? point.y() : fallback.y());
}

qreal clampedValue(qreal value, qreal minimum, qreal maximum)
{
    return std::clamp(std::isfinite(value) ? value : minimum, minimum, maximum);
}
}

namespace KiriView {
QRectF imageViewportImageRect(const QSizeF &viewportSize, const QSizeF &displaySize)
{
    const QSizeF viewport = normalizedSize(viewportSize);
    const QSizeF display = normalizedSize(displaySize);
    return QRectF(std::max<qreal>(0.0, (viewport.width() - display.width()) / 2.0),
        std::max<qreal>(0.0, (viewport.height() - display.height()) / 2.0), display.width(),
        display.height());
}

QPointF imageViewportMaximumContentPosition(const QSizeF &viewportSize, const QRectF &imageRect)
{
    const QSizeF viewport = normalizedSize(viewportSize);
    return QPointF(
        std::max<qreal>(
            0.0, std::max(viewport.width(), imageRect.x() + imageRect.width()) - viewport.width()),
        std::max<qreal>(0.0,
            std::max(viewport.height(), imageRect.y() + imageRect.height()) - viewport.height()));
}

QPointF imageViewportClampedContentPosition(
    const QSizeF &viewportSize, const QRectF &imageRect, const QPointF &contentPosition)
{
    const QPointF maximum = imageViewportMaximumContentPosition(viewportSize, imageRect);
    return QPointF(clampedValue(contentPosition.x(), 0.0, maximum.x()),
        clampedValue(contentPosition.y(), 0.0, maximum.y()));
}

QPointF imageViewportPanPosition(const QSizeF &viewportSize, const QRectF &imageRect,
    const QPointF &contentPosition, const QPointF &delta)
{
    return imageViewportClampedContentPosition(viewportSize, imageRect,
        QPointF(contentPosition.x() + delta.x(), contentPosition.y() + delta.y()));
}

bool imageViewportPointInsideImage(
    const QPointF &contentPosition, const QPointF &viewportPoint, const QRectF &imageRect)
{
    if (imageRect.width() <= 0.0 || imageRect.height() <= 0.0) {
        return false;
    }

    return imageRect.contains(contentPosition + viewportPoint);
}

QPointF imageViewportContentPositionForZoom(const QSizeF &viewportSize,
    const QSizeF &currentDisplaySize, const QSizeF &nextDisplaySize, const QPointF &contentPosition,
    const QPointF &viewportAnchorPoint)
{
    const QSizeF viewport = normalizedSize(viewportSize);
    const QRectF currentImageRect = imageViewportImageRect(viewport, currentDisplaySize);
    const QPointF fallbackAnchor(viewport.width() / 2.0, viewport.height() / 2.0);
    const QPointF anchor = normalizedPoint(viewportAnchorPoint, fallbackAnchor);
    const QPointF clampedAnchor(clampedValue(anchor.x(), 0.0, viewport.width()),
        clampedValue(anchor.y(), 0.0, viewport.height()));

    const qreal anchorImageX = contentPosition.x() + clampedAnchor.x() - currentImageRect.x();
    const qreal anchorImageY = contentPosition.y() + clampedAnchor.y() - currentImageRect.y();
    const qreal anchorRatioX = currentImageRect.width() > 0.0
        ? clampedValue(anchorImageX / currentImageRect.width(), 0.0, 1.0)
        : 0.5;
    const qreal anchorRatioY = currentImageRect.height() > 0.0
        ? clampedValue(anchorImageY / currentImageRect.height(), 0.0, 1.0)
        : 0.5;

    const QRectF nextImageRect = imageViewportImageRect(viewport, nextDisplaySize);
    return imageViewportClampedContentPosition(viewport, nextImageRect,
        QPointF(nextImageRect.x() + anchorRatioX * nextImageRect.width() - clampedAnchor.x(),
            nextImageRect.y() + anchorRatioY * nextImageRect.height() - clampedAnchor.y()));
}

QSizeF imageViewportDisplaySizeForZoom(
    const QSize &imageSize, qreal zoomPercent, qreal devicePixelRatio)
{
    if (imageSize.isEmpty() || !std::isfinite(zoomPercent) || zoomPercent <= 0.0
        || !std::isfinite(devicePixelRatio) || devicePixelRatio <= 0.0) {
        return {};
    }

    const qreal scale = zoomPercent / (devicePixelRatio * 100.0);
    return QSizeF(imageSize.width() * scale, imageSize.height() * scale);
}
}
