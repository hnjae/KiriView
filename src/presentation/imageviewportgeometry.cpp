// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageviewportgeometry.h"

#include "bridge/qtgeometryconversion.h"
#include "kiriview/src/policy/imageviewportgeometry.cxx.h"

namespace kiriview {
QRectF imageViewportImageRect(QSizeF viewportSize, QSizeF displaySize)
{
    return Bridge::qtRectF(rustImageViewportImageRect(
        Bridge::rustSizeF<RustSizeF>(viewportSize), Bridge::rustSizeF<RustSizeF>(displaySize)));
}

QPointF imageViewportMaximumContentPosition(QSizeF viewportSize, const QRectF& imageRect)
{
    return Bridge::qtPointF(rustImageViewportMaximumContentPosition(
        Bridge::rustSizeF<RustSizeF>(viewportSize), Bridge::rustRectF<RustRectF>(imageRect)));
}

QPointF imageViewportClampedContentPosition(
    QSizeF viewportSize, const QRectF& imageRect, QPointF contentPosition)
{
    return Bridge::qtPointF(rustImageViewportClampedContentPosition(
        Bridge::rustSizeF<RustSizeF>(viewportSize), Bridge::rustRectF<RustRectF>(imageRect),
        Bridge::rustPointF<RustPointF>(contentPosition)));
}

QPointF imageViewportPanPosition(
    QSizeF viewportSize, const QRectF& imageRect, QPointF contentPosition, QPointF delta)
{
    return Bridge::qtPointF(rustImageViewportPanPosition(Bridge::rustSizeF<RustSizeF>(viewportSize),
        Bridge::rustRectF<RustRectF>(imageRect), Bridge::rustPointF<RustPointF>(contentPosition),
        Bridge::rustPointF<RustPointF>(delta)));
}

QPointF imageViewportNextZScanPosition(
    QSizeF viewportSize, const QRectF& imageRect, QPointF contentPosition, bool rightToLeftReading)
{
    return Bridge::qtPointF(rustImageViewportNextZScanPosition(
        Bridge::rustSizeF<RustSizeF>(viewportSize), Bridge::rustRectF<RustRectF>(imageRect),
        Bridge::rustPointF<RustPointF>(contentPosition), rightToLeftReading));
}

QPointF imageViewportPreviousZScanPosition(
    QSizeF viewportSize, const QRectF& imageRect, QPointF contentPosition, bool rightToLeftReading)
{
    return Bridge::qtPointF(rustImageViewportPreviousZScanPosition(
        Bridge::rustSizeF<RustSizeF>(viewportSize), Bridge::rustRectF<RustRectF>(imageRect),
        Bridge::rustPointF<RustPointF>(contentPosition), rightToLeftReading));
}

QPointF imageViewportInitialZScanPosition(
    QSizeF viewportSize, const QRectF& imageRect, bool rightToLeftReading)
{
    return Bridge::qtPointF(
        rustImageViewportInitialZScanPosition(Bridge::rustSizeF<RustSizeF>(viewportSize),
            Bridge::rustRectF<RustRectF>(imageRect), rightToLeftReading));
}

QPointF imageViewportFinalZScanPosition(
    QSizeF viewportSize, const QRectF& imageRect, bool rightToLeftReading)
{
    return Bridge::qtPointF(
        rustImageViewportFinalZScanPosition(Bridge::rustSizeF<RustSizeF>(viewportSize),
            Bridge::rustRectF<RustRectF>(imageRect), rightToLeftReading));
}

bool imageViewportPointInsideImage(
    QPointF contentPosition, QPointF viewportPoint, const QRectF& imageRect)
{
    return rustImageViewportPointInsideImage(Bridge::rustPointF<RustPointF>(contentPosition),
        Bridge::rustPointF<RustPointF>(viewportPoint), Bridge::rustRectF<RustRectF>(imageRect));
}

QPointF imageViewportNearestImagePoint(
    QPointF contentPosition, QPointF viewportPoint, const QRectF& imageRect)
{
    return Bridge::qtPointF(rustImageViewportNearestImagePoint(
        Bridge::rustPointF<RustPointF>(contentPosition),
        Bridge::rustPointF<RustPointF>(viewportPoint), Bridge::rustRectF<RustRectF>(imageRect)));
}

QPointF imageViewportContentPositionForZoom(QSizeF viewportSize, QSizeF currentDisplaySize,
    QSizeF nextDisplaySize, QPointF contentPosition, QPointF viewportAnchorPoint)
{
    return Bridge::qtPointF(
        rustImageViewportContentPositionForZoom(Bridge::rustSizeF<RustSizeF>(viewportSize),
            Bridge::rustSizeF<RustSizeF>(currentDisplaySize),
            Bridge::rustSizeF<RustSizeF>(nextDisplaySize),
            Bridge::rustPointF<RustPointF>(contentPosition),
            Bridge::rustPointF<RustPointF>(viewportAnchorPoint)));
}

QSizeF imageViewportDisplaySizeForZoom(QSize imageSize, qreal zoomPercent, qreal devicePixelRatio)
{
    return Bridge::qtSizeF(rustImageViewportDisplaySizeForZoom(
        Bridge::rustSize<RustSize>(imageSize), zoomPercent, devicePixelRatio));
}
}
