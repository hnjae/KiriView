// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagesurface.h"

#include "kiriview/src/imagerendergeometry.cxx.h"
#include "qtgeometryconversion.h"
#include "systemmemory.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

namespace {
KiriView::RustImageRenderSize rustImageRenderSize(const QSize &size)
{
    return KiriView::Bridge::rustSize<KiriView::RustImageRenderSize>(size);
}

qsizetype qtByteSize(std::int64_t byteSize)
{
    constexpr qsizetype maximumByteSize = std::numeric_limits<qsizetype>::max();
    constexpr qsizetype minimumByteSize = std::numeric_limits<qsizetype>::min();
    if (byteSize > static_cast<std::int64_t>(maximumByteSize)) {
        return maximumByteSize;
    }
    if (byteSize < static_cast<std::int64_t>(minimumByteSize)) {
        return minimumByteSize;
    }

    return static_cast<qsizetype>(byteSize);
}

std::int64_t rustByteSize(qsizetype byteSize) { return static_cast<std::int64_t>(byteSize); }
}

namespace KiriView {
StaticTileSurface::StaticTileSurface(StaticImagePayload image)
    : m_image(std::move(image))
    , m_pyramid(m_image.source == nullptr ? QSize() : m_image.source->imageSize())
    , m_tileCache(defaultTileCacheByteBudget())
{
}

bool StaticTileSurface::isValid() const
{
    return m_image.isValid() && !m_image.source->imageSize().isEmpty();
}

const StaticImagePayload &StaticTileSurface::image() const { return m_image; }

std::shared_ptr<ImageTileSource> StaticTileSurface::source() const { return m_image.source; }

const TilePyramid &StaticTileSurface::pyramid() const { return m_pyramid; }

QSize StaticTileSurface::imageSize() const { return m_pyramid.imageSize(); }

const QImage &StaticTileSurface::preview() const { return m_image.preview; }

const StaticImageDisplayHints &StaticTileSurface::displayHints() const
{
    return m_image.displayHints;
}

bool StaticTileSurface::containsTile(const TileKey &key) const { return m_tileCache.contains(key); }

std::optional<DecodedTile> StaticTileSurface::tile(const TileKey &key)
{
    return m_tileCache.find(key);
}

std::vector<DecodedTile> StaticTileSurface::tiles() const { return m_tileCache.tiles(); }

bool StaticTileSurface::insertTile(DecodedTile tile) { return m_tileCache.insert(std::move(tile)); }

void StaticTileSurface::clearTiles() { m_tileCache.clear(); }

qsizetype StaticTileSurface::defaultTileCacheByteBudget()
{
    return tileCacheByteBudgetForSystemMemory(physicalSystemMemoryByteSize().value_or(0));
}

qsizetype StaticTileSurface::tileCacheByteBudgetForSystemMemory(qsizetype systemMemoryByteSize)
{
    return qtByteSize(rustStaticTileCacheByteBudgetForSystemMemory(
        rustByteSize(systemMemoryByteSize), rustByteSize(imageFullDecodeFallbackByteLimit)));
}

bool staticImageFitsFullImageSurface(const StaticImagePayload &image, int maximumTextureSize)
{
    const QSize imageSize = image.source == nullptr ? QSize() : image.source->imageSize();
    return rustStaticImageFitsFullImageSurface(image.isValid(), rustImageRenderSize(imageSize),
        rustImageRenderSize(image.preview.size()), maximumTextureSize);
}

DisplayedImageSurface::DisplayedImageSurface(LegacyFrameSurface surface)
    : m_payload(std::move(surface))
{
}

DisplayedImageSurface::DisplayedImageSurface(StaticTileSurface surface)
    : m_payload(std::move(surface))
{
}

QSize DisplayedImageSurface::imageSize() const
{
    if (const auto *surface = legacyFrameSurface()) {
        return surface->image.size();
    }
    if (const auto *surface = staticTileSurface()) {
        return surface->imageSize();
    }

    return {};
}

bool DisplayedImageSurface::isNull() const
{
    if (const auto *surface = legacyFrameSurface()) {
        return surface->image.isNull();
    }
    if (const auto *surface = staticTileSurface()) {
        return !surface->isValid();
    }

    return true;
}

LegacyFrameSurface *DisplayedImageSurface::legacyFrameSurface()
{
    return std::get_if<LegacyFrameSurface>(&m_payload);
}

const LegacyFrameSurface *DisplayedImageSurface::legacyFrameSurface() const
{
    return std::get_if<LegacyFrameSurface>(&m_payload);
}

StaticTileSurface *DisplayedImageSurface::staticTileSurface()
{
    return std::get_if<StaticTileSurface>(&m_payload);
}

const StaticTileSurface *DisplayedImageSurface::staticTileSurface() const
{
    return std::get_if<StaticTileSurface>(&m_payload);
}

}
