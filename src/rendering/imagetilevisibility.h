// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILEVISIBILITY_H
#define KIRIVIEW_IMAGETILEVISIBILITY_H

#include "imagetile.h"

#include <QRect>
#include <QRectF>
#include <QSizeF>
#include <QtGlobal>
#include <vector>

namespace KiriView {
struct TileVisibilityContext {
    QSizeF displaySize;
    QRectF visibleItemRect;
    qreal devicePixelRatio = 1.0;
    int rotationDegrees = 0;
};

struct TileSourceVisibilityContext {
    QSizeF displaySize;
    QRectF visibleItemRect;
};

TileSourceVisibilityContext tileSourceVisibilityContext(const TileVisibilityContext &context);
qreal tileDisplayPixelsPerSourcePixel(
    const TilePyramid &pyramid, const QSizeF &displaySize, qreal devicePixelRatio);
ActiveTileLayer activeTileLayer(
    const TilePyramid &pyramid, const TileVisibilityContext &context, bool resolutionIndependent);
bool tileFirstDisplayIsSufficient(const TilePyramid &pyramid, const QSizeF &displaySize,
    qreal devicePixelRatio, qreal firstDisplayPixelsPerSourcePixel);
QRect tileLevelRectForItemRect(
    const TilePyramid &pyramid, int level, const QSizeF &displaySize, const QRectF &itemRect);
std::vector<TileKey> visibleTileKeys(
    const TilePyramid &pyramid, const TileVisibilityContext &context);
}

#endif
