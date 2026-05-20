// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilevisibility.h"

#include "imagetilegeometrybridge.h"
#include "presentation/imagerotation.h"

namespace KiriView {
qreal tileDisplayPixelsPerSourcePixel(
    const TilePyramid &pyramid, const QSizeF &displaySize, qreal devicePixelRatio)
{
    return ImageTileGeometryBridge::tileDisplayPixelsPerSourcePixel(
        pyramid.imageSize(), displaySize, devicePixelRatio);
}

bool tileFirstDisplayIsSufficient(const TilePyramid &pyramid, const QSizeF &displaySize,
    qreal devicePixelRatio, qreal firstDisplayPixelsPerSourcePixel)
{
    return ImageTileGeometryBridge::tileFirstDisplayIsSufficient(
        pyramid.imageSize(), displaySize, devicePixelRatio, firstDisplayPixelsPerSourcePixel);
}

QRect tileLevelRectForItemRect(
    const TilePyramid &pyramid, int level, const QSizeF &displaySize, const QRectF &itemRect)
{
    return ImageTileGeometryBridge::tileLevelRectForItemRect(
        pyramid.imageSize(), level, displaySize, itemRect);
}

std::vector<TileKey> visibleTileKeys(
    const TilePyramid &pyramid, const TileVisibilityContext &context)
{
    const QSizeF sourceDisplaySize = rotatedImageSize(context.displaySize, context.rotationDegrees);
    const QRectF sourceVisibleItemRect = unrotatedVisibleRectForRotation(
        sourceDisplaySize, context.visibleItemRect, context.rotationDegrees);
    return ImageTileGeometryBridge::visibleTileKeys(pyramid.imageSize(), pyramid.tileSize(),
        sourceDisplaySize, sourceVisibleItemRect, context.devicePixelRatio);
}
}
