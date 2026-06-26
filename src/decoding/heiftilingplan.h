// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_HEIFTILINGPLAN_H
#define KIRIVIEW_HEIFTILINGPLAN_H

#include <libheif/heif_tiling.h>

#include <QPoint>
#include <QRect>
#include <optional>
#include <vector>

namespace kiriview {
struct HeifTileGrid
{
    int columns = 0;
    int rows = 0;
    int tileWidth = 0;
    int tileHeight = 0;
};

struct HeifTileDecodeRegion
{
    int tileX = 0;
    int tileY = 0;
    QPoint targetPoint;
};

std::optional<HeifTileGrid> heifTileGridForTiling(const heif_image_tiling& tiling);
std::vector<HeifTileDecodeRegion> heifTileDecodeRegions(
    const HeifTileGrid& grid, const QRect& sourceRect);
}

#endif
