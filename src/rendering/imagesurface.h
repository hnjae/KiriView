// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESURFACE_H
#define KIRIVIEW_IMAGESURFACE_H

#include "decodedtilecache.h"
#include "imagerendercontext.h"
#include "staticimage.h"

#include <QImage>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QtGlobal>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace KiriView {
class StaticTileSurface
{
public:
    StaticTileSurface(StaticImagePayload image, qsizetype tileCacheByteBudget);

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
    qsizetype tileCacheByteBudget() const;

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
    DisplayedImageSurface();
    explicit DisplayedImageSurface(LegacyFrameSurface surface);
    explicit DisplayedImageSurface(StaticTileSurface surface);
    DisplayedImageSurface(const DisplayedImageSurface &other);
    DisplayedImageSurface &operator=(const DisplayedImageSurface &other);
    DisplayedImageSurface(DisplayedImageSurface &&other) noexcept = default;
    DisplayedImageSurface &operator=(DisplayedImageSurface &&other) noexcept = default;

    quint64 identity() const;
    QSize imageSize() const;
    bool isNull() const;
    LegacyFrameSurface *legacyFrameSurface();
    const LegacyFrameSurface *legacyFrameSurface() const;
    StaticTileSurface *staticTileSurface();
    const StaticTileSurface *staticTileSurface() const;

private:
    using Payload = std::variant<LegacyFrameSurface, StaticTileSurface>;

    static quint64 nextIdentity();

    quint64 m_identity;
    Payload m_payload;
};

struct DisplayedImageRenderSnapshot {
    std::shared_ptr<DisplayedImageSurface> surface;
    quint64 revision = 0;
    QSize imageSize;
    QSizeF displaySize;
    QRectF visibleItemRect;
    qreal devicePixelRatio = 1.0;
    int rotationDegrees = 0;
    DisplayedPageRole pageRole = DisplayedPageRole::Primary;

    bool isRenderable() const { return surface != nullptr && !surface->isNull(); }
};

bool staticImageFitsFullImageSurface(const StaticImagePayload &image, int maximumTextureSize);
}

#endif
