// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESURFACE_H
#define KIRIVIEW_IMAGESURFACE_H

#include "imagetile.h"
#include "staticimage.h"

#include <QImage>
#include <QSize>
#include <QtGlobal>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace KiriView {
class StaticTileSurface
{
public:
    explicit StaticTileSurface(StaticImagePayload image = {});

    bool isValid() const;
    const StaticImagePayload &image() const;
    std::shared_ptr<ImageTileSource> source() const;
    const TilePyramid &pyramid() const;
    QSize imageSize() const;
    const QImage &preview() const;
    const StaticImageDisplayHints &displayHints() const;
    bool containsTile(const TileKey &key) const;
    std::optional<DecodedTile> tile(const TileKey &key);
    std::vector<DecodedTile> tiles() const;
    bool insertTile(DecodedTile tile);
    void clearTiles();

    static qsizetype defaultTileCacheByteBudget();
    static qsizetype tileCacheByteBudgetForSystemMemory(qsizetype systemMemoryByteSize);

private:
    StaticImagePayload m_image;
    TilePyramid m_pyramid;
    DecodedTileCache m_tileCache;
};

struct LegacyFrameSurface {
    QImage image;
};

class DisplayedImageSurface
{
public:
    DisplayedImageSurface() = default;
    explicit DisplayedImageSurface(LegacyFrameSurface surface);
    explicit DisplayedImageSurface(StaticTileSurface surface);

    QSize imageSize() const;
    bool isNull() const;
    LegacyFrameSurface *legacyFrameSurface();
    const LegacyFrameSurface *legacyFrameSurface() const;
    StaticTileSurface *staticTileSurface();
    const StaticTileSurface *staticTileSurface() const;

private:
    using Payload = std::variant<LegacyFrameSurface, StaticTileSurface>;

    Payload m_payload;
};

bool staticImageFitsFullImageSurface(const StaticImagePayload &image, int maximumTextureSize);
QSize displayedImageSurfaceSize(const DisplayedImageSurface &surface);
bool displayedImageSurfaceIsNull(const DisplayedImageSurface &surface);
}

#endif
