// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilevisibility.h"

#include "imagetilegeometrypolicy.h"
#include "presentation/imagerotation.h"

namespace KiriView {
qreal tileDisplayPixelsPerSourcePixel(
    const TilePyramid &pyramid, const QSizeF &displaySize, qreal devicePixelRatio)
{
    return ImageTileGeometryPolicy::tileDisplayPixelsPerSourcePixel(
        pyramid.imageSize(), displaySize, devicePixelRatio);
}

bool tileFirstDisplayIsSufficient(const TilePyramid &pyramid, const QSizeF &displaySize,
    qreal devicePixelRatio, qreal firstDisplayPixelsPerSourcePixel)
{
    return ImageTileGeometryPolicy::tileFirstDisplayIsSufficient(
        pyramid.imageSize(), displaySize, devicePixelRatio, firstDisplayPixelsPerSourcePixel);
}

QRect tileLevelRectForItemRect(
    const TilePyramid &pyramid, int level, const QSizeF &displaySize, const QRectF &itemRect)
{
    return ImageTileGeometryPolicy::tileLevelRectForItemRect(
        pyramid.imageSize(), level, displaySize, itemRect);
}

std::vector<TileKey> visibleTileKeys(
    const TilePyramid &pyramid, const TileVisibilityContext &context)
{
    const QSizeF sourceDisplaySize = rotatedImageSize(context.displaySize, context.rotationDegrees);
    const QRectF sourceVisibleItemRect = unrotatedVisibleRectForRotation(
        sourceDisplaySize, context.visibleItemRect, context.rotationDegrees);
    return ImageTileGeometryPolicy::visibleTileKeys(pyramid.imageSize(), pyramid.tileSize(),
        sourceDisplaySize, sourceVisibleItemRect, context.devicePixelRatio);
}
}
