// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILESOURCEHELPERS_P_H
#define KIRIVIEW_IMAGETILESOURCEHELPERS_P_H

#include "imagetile.h"

#include <QImage>
#include <QSize>
#include <QString>
#include <optional>

namespace kiriview {
QSize boundedPreviewSize(const QSize& imageSize, int maximumLongEdge);
bool tileRequestCanDecode(const TileRequest& request);
QImage scaledTileImage(const QImage& image, const QSize& size);
QImage cropLevelTexture(const QImage& levelImage, const QRect& textureLevelRect);
DecodedTile decodedTileFromImage(const TileRequest& request, QImage image);
std::optional<DecodedTile> decodedTileFromLevelImage(
    const TileRequest& request, const QImage& levelImage);
std::optional<DecodedTile> decodedTileFromSourceImage(
    const TileRequest& request, const QImage& sourceImage);
void setTileSourceError(QString* errorString, const QString& message);
}

#endif
