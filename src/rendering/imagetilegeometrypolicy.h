// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILEGEOMETRYPOLICY_H
#define KIRIVIEW_IMAGETILEGEOMETRYPOLICY_H

#include "imagetile.h"

#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QtGlobal>
#include <vector>

namespace KiriView::ImageTileGeometryPolicy {
std::vector<TileLevel> tilePyramidLevels(const QSize &imageSize);
QSize tilePyramidTileGridSize(const QSize &imageSize, int tileSize, int level);
bool tilePyramidContainsLevel(int levelCount, int level);
bool tilePyramidContainsTile(const QSize &imageSize, int tileSize, const TileKey &key);
int tilePyramidSelectLevelForDisplayScale(
    const QSize &imageSize, qreal displayPixelsPerSourcePixel);
QRect tilePyramidLevelTileRect(const QSize &imageSize, int tileSize, const TileKey &key);
QRect tilePyramidLevelTileTextureRect(
    const QSize &imageSize, int tileSize, int apronSourcePixels, const TileKey &key);
QRect tilePyramidSourceRectForLevelRect(const QSize &imageSize, int level, const QRect &levelRect);
TileRequest tilePyramidRequestForTile(
    const QSize &imageSize, int tileSize, int apronSourcePixels, const TileKey &key);
std::vector<TileKey> tilePyramidTilesIntersectingLevelRect(
    const QSize &imageSize, int tileSize, int level, const QRect &levelRect);
qreal tilePyramidLevelPixelsPerSourcePixel(const QSize &imageSize, int level);
ActiveTileLayer activeTileLayer(const QSize &imageSize, const QSizeF &displaySize,
    qreal devicePixelRatio, int rotationDegrees, bool resolutionIndependent);

qreal tileDisplayPixelsPerSourcePixel(
    const QSize &imageSize, const QSizeF &displaySize, qreal devicePixelRatio);
bool tileFirstDisplayIsSufficient(const QSize &imageSize, const QSizeF &displaySize,
    qreal devicePixelRatio, qreal firstDisplayPixelsPerSourcePixel);
QRect tileLevelRectForItemRect(
    const QSize &imageSize, int level, const QSizeF &displaySize, const QRectF &itemRect);
std::vector<TileKey> visibleTileKeys(const QSize &imageSize, int tileSize,
    const QSizeF &displaySize, const QRectF &visibleItemRect, qreal devicePixelRatio);
std::vector<TileRequest> svgRasterTileRequests(const QSize &imageSize, int tileSize,
    int apronSourcePixels, const QSizeF &displaySize, const QRectF &visibleItemRect,
    qreal devicePixelRatio);

}

#endif
