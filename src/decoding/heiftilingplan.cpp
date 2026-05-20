// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "heiftilingplan.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace {
std::optional<int> positiveInt(std::uint32_t value)
{
    if (value == 0 || value > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        return std::nullopt;
    }

    return static_cast<int>(value);
}
}

namespace KiriView {
std::optional<HeifTileGrid> heifTileGridForTiling(const heif_image_tiling &tiling)
{
    const std::optional<int> columns = positiveInt(tiling.num_columns);
    const std::optional<int> rows = positiveInt(tiling.num_rows);
    const std::optional<int> tileWidth = positiveInt(tiling.tile_width);
    const std::optional<int> tileHeight = positiveInt(tiling.tile_height);
    if (!columns.has_value() || !rows.has_value() || !tileWidth.has_value()
        || !tileHeight.has_value()) {
        return std::nullopt;
    }

    if (*columns == 1 && *rows == 1) {
        return std::nullopt;
    }

    return HeifTileGrid {
        *columns,
        *rows,
        *tileWidth,
        *tileHeight,
    };
}

std::vector<HeifTileDecodeRegion> heifTileDecodeRegions(
    const HeifTileGrid &grid, const QRect &sourceRect)
{
    if (sourceRect.isEmpty() || grid.columns <= 0 || grid.rows <= 0 || grid.tileWidth <= 0
        || grid.tileHeight <= 0) {
        return {};
    }

    const int firstTileX = std::max(0, sourceRect.left() / grid.tileWidth);
    const int firstTileY = std::max(0, sourceRect.top() / grid.tileHeight);
    const int lastTileX = std::min(grid.columns - 1, sourceRect.right() / grid.tileWidth);
    const int lastTileY = std::min(grid.rows - 1, sourceRect.bottom() / grid.tileHeight);
    if (firstTileX > lastTileX || firstTileY > lastTileY) {
        return {};
    }

    std::vector<HeifTileDecodeRegion> regions;
    const std::size_t columnCount = static_cast<std::size_t>(lastTileX - firstTileX + 1);
    const std::size_t rowCount = static_cast<std::size_t>(lastTileY - firstTileY + 1);
    regions.reserve(columnCount * rowCount);
    for (int tileY = firstTileY; tileY <= lastTileY; ++tileY) {
        for (int tileX = firstTileX; tileX <= lastTileX; ++tileX) {
            regions.push_back(HeifTileDecodeRegion {
                tileX,
                tileY,
                QPoint(tileX * grid.tileWidth - sourceRect.x(),
                    tileY * grid.tileHeight - sourceRect.y()),
            });
        }
    }

    return regions;
}
}
