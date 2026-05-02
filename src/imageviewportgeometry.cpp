// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageviewportgeometry.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr qreal scanStepViewportRatio = 0.75;
constexpr qreal positionEpsilon = 0.001;

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

bool axisPannable(qreal maximum) { return maximum > positionEpsilon; }

qreal scanStepLength(qreal viewportLength)
{
    return normalizedLength(viewportLength) * scanStepViewportRatio;
}

qreal nextAxisScanPosition(qreal position, qreal step, qreal maximum)
{
    const qreal current = clampedValue(position, 0.0, maximum);
    if (!axisPannable(maximum) || step <= positionEpsilon || current >= maximum - positionEpsilon) {
        return current;
    }

    const qreal nextPosition = (std::floor(current / step) + 1.0) * step;
    return std::min(maximum, nextPosition);
}

qreal previousAxisScanPosition(qreal position, qreal step, qreal maximum)
{
    const qreal current = clampedValue(position, 0.0, maximum);
    if (!axisPannable(maximum) || step <= positionEpsilon || current <= positionEpsilon) {
        return current;
    }

    const qreal previousPosition = (std::ceil(current / step) - 1.0) * step;
    return std::max<qreal>(0.0, previousPosition);
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

QPointF imageViewportNextZScanPosition(
    const QSizeF &viewportSize, const QRectF &imageRect, const QPointF &contentPosition)
{
    const QSizeF viewport = normalizedSize(viewportSize);
    const QPointF maximum = imageViewportMaximumContentPosition(viewport, imageRect);
    const QPointF current
        = imageViewportClampedContentPosition(viewport, imageRect, contentPosition);

    if (axisPannable(maximum.x())) {
        const qreal nextX
            = nextAxisScanPosition(current.x(), scanStepLength(viewport.width()), maximum.x());
        if (nextX != current.x()) {
            return QPointF(nextX, current.y());
        }
    }

    if (axisPannable(maximum.y())) {
        const qreal nextY
            = nextAxisScanPosition(current.y(), scanStepLength(viewport.height()), maximum.y());
        if (nextY != current.y()) {
            return QPointF(axisPannable(maximum.x()) ? 0.0 : current.x(), nextY);
        }
    }

    return current;
}

QPointF imageViewportPreviousZScanPosition(
    const QSizeF &viewportSize, const QRectF &imageRect, const QPointF &contentPosition)
{
    const QSizeF viewport = normalizedSize(viewportSize);
    const QPointF maximum = imageViewportMaximumContentPosition(viewport, imageRect);
    const QPointF current
        = imageViewportClampedContentPosition(viewport, imageRect, contentPosition);

    if (axisPannable(maximum.x())) {
        const qreal previousX
            = previousAxisScanPosition(current.x(), scanStepLength(viewport.width()), maximum.x());
        if (previousX != current.x()) {
            return QPointF(previousX, current.y());
        }
    }

    if (axisPannable(maximum.y())) {
        const qreal previousY
            = previousAxisScanPosition(current.y(), scanStepLength(viewport.height()), maximum.y());
        if (previousY != current.y()) {
            return QPointF(axisPannable(maximum.x()) ? maximum.x() : current.x(), previousY);
        }
    }

    return current;
}

QPointF imageViewportFinalZScanPosition(const QSizeF &viewportSize, const QRectF &imageRect)
{
    return imageViewportMaximumContentPosition(viewportSize, imageRect);
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
