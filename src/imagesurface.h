// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESURFACE_H
#define KIRIVIEW_IMAGESURFACE_H

#include "imagetile.h"

#include <QImage>
#include <QSize>
#include <QtGlobal>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace KiriView {
inline constexpr int imagePreviewLongEdgeMax = 2048;
inline constexpr qsizetype imageFullDecodeFallbackByteLimit = 512 * 1024 * 1024;

class ImageTileSource
{
public:
    virtual ~ImageTileSource() = default;

    virtual QSize imageSize() const = 0;
    virtual int levelCount() const = 0;
    virtual QSize levelSize(int level) const = 0;
    virtual std::optional<DecodedTile> decodeTile(
        const TileRequest &request, QString *errorString) const
        = 0;
    virtual QImage decodePreview(int maximumLongEdge, QString *errorString) const = 0;
    virtual qsizetype byteCost() const = 0;
};

class StaticTileSurface
{
public:
    explicit StaticTileSurface(std::shared_ptr<ImageTileSource> source = {}, QImage preview = {});

    bool isValid() const;
    std::shared_ptr<ImageTileSource> source() const;
    const TilePyramid &pyramid() const;
    QSize imageSize() const;
    const QImage &preview() const;
    bool containsTile(const TileKey &key) const;
    std::optional<DecodedTile> tile(const TileKey &key);
    std::vector<DecodedTile> tiles() const;
    void insertTile(DecodedTile tile);
    void clearTiles();

    static qsizetype defaultTileCacheByteBudget();
    static qsizetype tileCacheByteBudgetForSystemMemory(qsizetype systemMemoryByteSize);

private:
    std::shared_ptr<ImageTileSource> m_source;
    TilePyramid m_pyramid;
    QImage m_preview;
    DecodedTileCache m_tileCache;
};

struct LegacyFrameSurface {
    QImage image;
};

using DisplayedImageSurface = std::variant<LegacyFrameSurface, StaticTileSurface>;

QSize displayedImageSurfaceSize(const DisplayedImageSurface &surface);
bool displayedImageSurfaceIsNull(const DisplayedImageSurface &surface);
}

#endif
