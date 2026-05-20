// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageviewportgeometry.h"

#include "kiriview/src/imageviewportgeometry.cxx.h"
#include "qtgeometryconversion.h"

namespace KiriView {
QRectF imageViewportImageRect(const QSizeF &viewportSize, const QSizeF &displaySize)
{
    return Bridge::qtRectF(rustImageViewportImageRect(
        Bridge::rustSizeF<RustSizeF>(viewportSize), Bridge::rustSizeF<RustSizeF>(displaySize)));
}

QPointF imageViewportMaximumContentPosition(const QSizeF &viewportSize, const QRectF &imageRect)
{
    return Bridge::qtPointF(rustImageViewportMaximumContentPosition(
        Bridge::rustSizeF<RustSizeF>(viewportSize), Bridge::rustRectF<RustRectF>(imageRect)));
}

QPointF imageViewportClampedContentPosition(
    const QSizeF &viewportSize, const QRectF &imageRect, const QPointF &contentPosition)
{
    return Bridge::qtPointF(rustImageViewportClampedContentPosition(
        Bridge::rustSizeF<RustSizeF>(viewportSize), Bridge::rustRectF<RustRectF>(imageRect),
        Bridge::rustPointF<RustPointF>(contentPosition)));
}

QPointF imageViewportPanPosition(const QSizeF &viewportSize, const QRectF &imageRect,
    const QPointF &contentPosition, const QPointF &delta)
{
    return Bridge::qtPointF(rustImageViewportPanPosition(Bridge::rustSizeF<RustSizeF>(viewportSize),
        Bridge::rustRectF<RustRectF>(imageRect), Bridge::rustPointF<RustPointF>(contentPosition),
        Bridge::rustPointF<RustPointF>(delta)));
}

QPointF imageViewportNextZScanPosition(const QSizeF &viewportSize, const QRectF &imageRect,
    const QPointF &contentPosition, bool rightToLeftReading)
{
    return Bridge::qtPointF(rustImageViewportNextZScanPosition(
        Bridge::rustSizeF<RustSizeF>(viewportSize), Bridge::rustRectF<RustRectF>(imageRect),
        Bridge::rustPointF<RustPointF>(contentPosition), rightToLeftReading));
}

QPointF imageViewportPreviousZScanPosition(const QSizeF &viewportSize, const QRectF &imageRect,
    const QPointF &contentPosition, bool rightToLeftReading)
{
    return Bridge::qtPointF(rustImageViewportPreviousZScanPosition(
        Bridge::rustSizeF<RustSizeF>(viewportSize), Bridge::rustRectF<RustRectF>(imageRect),
        Bridge::rustPointF<RustPointF>(contentPosition), rightToLeftReading));
}

QPointF imageViewportInitialZScanPosition(
    const QSizeF &viewportSize, const QRectF &imageRect, bool rightToLeftReading)
{
    return Bridge::qtPointF(
        rustImageViewportInitialZScanPosition(Bridge::rustSizeF<RustSizeF>(viewportSize),
            Bridge::rustRectF<RustRectF>(imageRect), rightToLeftReading));
}

QPointF imageViewportFinalZScanPosition(
    const QSizeF &viewportSize, const QRectF &imageRect, bool rightToLeftReading)
{
    return Bridge::qtPointF(
        rustImageViewportFinalZScanPosition(Bridge::rustSizeF<RustSizeF>(viewportSize),
            Bridge::rustRectF<RustRectF>(imageRect), rightToLeftReading));
}

bool imageViewportPointInsideImage(
    const QPointF &contentPosition, const QPointF &viewportPoint, const QRectF &imageRect)
{
    return rustImageViewportPointInsideImage(Bridge::rustPointF<RustPointF>(contentPosition),
        Bridge::rustPointF<RustPointF>(viewportPoint), Bridge::rustRectF<RustRectF>(imageRect));
}

QPointF imageViewportContentPositionForZoom(const QSizeF &viewportSize,
    const QSizeF &currentDisplaySize, const QSizeF &nextDisplaySize, const QPointF &contentPosition,
    const QPointF &viewportAnchorPoint)
{
    return Bridge::qtPointF(
        rustImageViewportContentPositionForZoom(Bridge::rustSizeF<RustSizeF>(viewportSize),
            Bridge::rustSizeF<RustSizeF>(currentDisplaySize),
            Bridge::rustSizeF<RustSizeF>(nextDisplaySize),
            Bridge::rustPointF<RustPointF>(contentPosition),
            Bridge::rustPointF<RustPointF>(viewportAnchorPoint)));
}

QSizeF imageViewportDisplaySizeForZoom(
    const QSize &imageSize, qreal zoomPercent, qreal devicePixelRatio)
{
    return Bridge::qtSizeF(rustImageViewportDisplaySizeForZoom(
        Bridge::rustSize<RustSize>(imageSize), zoomPercent, devicePixelRatio));
}
}
