// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerectmapping.h"

#include <QPoint>
#include <QPointF>
#include <algorithm>
#include <cmath>

namespace {
bool validRect(const QRectF &rect)
{
    return std::isfinite(rect.x()) && std::isfinite(rect.y()) && std::isfinite(rect.width())
        && std::isfinite(rect.height());
}

bool validSourceSize(const QSizeF &size)
{
    return !size.isEmpty() && std::isfinite(size.width()) && std::isfinite(size.height());
}

bool validTargetSize(const QSize &size) { return !size.isEmpty(); }
}

namespace KiriView {
QRect boundedIntegerRect(const QRect &rect, const QSize &boundsSize)
{
    if (rect.isEmpty() || boundsSize.isEmpty()) {
        return {};
    }

    return rect.intersected(QRect(QPoint(0, 0), boundsSize));
}

QRect scaledIntegerRect(const QRectF &rect, const QSizeF &sourceSize, const QSize &targetSize)
{
    if (rect.isEmpty() || !validRect(rect) || !validSourceSize(sourceSize)
        || !validTargetSize(targetSize)) {
        return {};
    }

    const QRectF bounded = rect.intersected(QRectF(QPointF(0.0, 0.0), sourceSize));
    if (bounded.isEmpty()) {
        return {};
    }

    const qreal xScale = targetSize.width() / sourceSize.width();
    const qreal yScale = targetSize.height() / sourceSize.height();
    if (!std::isfinite(xScale) || !std::isfinite(yScale) || xScale <= 0.0 || yScale <= 0.0) {
        return {};
    }

    const int left
        = std::clamp(static_cast<int>(std::floor(bounded.left() * xScale)), 0, targetSize.width());
    const int top
        = std::clamp(static_cast<int>(std::floor(bounded.top() * yScale)), 0, targetSize.height());
    const int right = std::clamp(
        static_cast<int>(std::ceil(bounded.right() * xScale)), left, targetSize.width());
    const int bottom = std::clamp(
        static_cast<int>(std::ceil(bounded.bottom() * yScale)), top, targetSize.height());
    return QRect(left, top, right - left, bottom - top);
}
}
