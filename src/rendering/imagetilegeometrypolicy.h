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

namespace kiriview::ImageTileGeometryPolicy {
std::vector<TileLevel> tilePyramidLevels(QSize imageSize);
QSize tilePyramidTileGridSize(QSize imageSize, int tileSize, int level);
bool tilePyramidContainsLevel(int levelCount, int level);
bool tilePyramidContainsTile(QSize imageSize, int tileSize, TileKey key);
int tilePyramidSelectLevelForDisplayScale(QSize imageSize, qreal displayPixelsPerSourcePixel);
QRect tilePyramidLevelTileRect(QSize imageSize, int tileSize, TileKey key);
QRect tilePyramidLevelTileTextureRect(
    QSize imageSize, int tileSize, int apronSourcePixels, TileKey key);
QRect tilePyramidSourceRectForLevelRect(QSize imageSize, int level, QRect levelRect);
TileRequest tilePyramidRequestForTile(
    QSize imageSize, int tileSize, int apronSourcePixels, TileKey key);
std::vector<TileKey> tilePyramidTilesIntersectingLevelRect(
    QSize imageSize, int tileSize, int level, QRect levelRect);
qreal tilePyramidLevelPixelsPerSourcePixel(QSize imageSize, int level);
ActiveTileLayer activeTileLayer(QSize imageSize, QSizeF displaySize, qreal devicePixelRatio,
    int rotationDegrees, bool resolutionIndependent);

qreal tileDisplayPixelsPerSourcePixel(QSize imageSize, QSizeF displaySize, qreal devicePixelRatio);
bool tileFirstDisplayIsSufficient(QSize imageSize, QSizeF displaySize, qreal devicePixelRatio,
    qreal firstDisplayPixelsPerSourcePixel);
QRect tileLevelRectForItemRect(
    QSize imageSize, int level, QSizeF displaySize, const QRectF& itemRect);
std::vector<TileKey> visibleTileKeys(QSize imageSize, int tileSize, QSizeF displaySize,
    const QRectF& visibleItemRect, qreal devicePixelRatio);
std::vector<TileRequest> svgRasterTileRequests(QSize imageSize, int tileSize, int apronSourcePixels,
    QSizeF displaySize, const QRectF& visibleItemRect, qreal devicePixelRatio);

}

#endif
