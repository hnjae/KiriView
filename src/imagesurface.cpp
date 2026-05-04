// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagesurface.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

#if defined(Q_OS_LINUX)
#include <unistd.h>
#endif

namespace {
std::optional<qsizetype> systemMemoryByteSize()
{
#if defined(Q_OS_LINUX)
    const long pageCount = ::sysconf(_SC_PHYS_PAGES);
    const long pageSize = ::sysconf(_SC_PAGE_SIZE);
    if (pageCount <= 0 || pageSize <= 0) {
        return std::nullopt;
    }

    const qsizetype normalizedPageCount = static_cast<qsizetype>(pageCount);
    const qsizetype normalizedPageSize = static_cast<qsizetype>(pageSize);
    const qsizetype maximumByteSize = std::numeric_limits<qsizetype>::max();
    if (normalizedPageCount > maximumByteSize / normalizedPageSize) {
        return maximumByteSize;
    }

    return normalizedPageCount * normalizedPageSize;
#else
    return std::nullopt;
#endif
}
}

namespace KiriView {
StaticTileSurface::StaticTileSurface(
    std::shared_ptr<ImageTileSource> source, QImage preview, StaticImageDisplayHints displayHints)
    : m_source(std::move(source))
    , m_pyramid(m_source == nullptr ? QSize() : m_source->imageSize())
    , m_preview(std::move(preview))
    , m_displayHints(displayHints)
    , m_tileCache(defaultTileCacheByteBudget())
{
}

bool StaticTileSurface::isValid() const
{
    return m_source != nullptr && !m_source->imageSize().isEmpty() && !m_preview.isNull();
}

std::shared_ptr<ImageTileSource> StaticTileSurface::source() const { return m_source; }

const TilePyramid &StaticTileSurface::pyramid() const { return m_pyramid; }

QSize StaticTileSurface::imageSize() const { return m_pyramid.imageSize(); }

const QImage &StaticTileSurface::preview() const { return m_preview; }

const StaticImageDisplayHints &StaticTileSurface::displayHints() const { return m_displayHints; }

bool StaticTileSurface::containsTile(const TileKey &key) const { return m_tileCache.contains(key); }

std::optional<DecodedTile> StaticTileSurface::tile(const TileKey &key)
{
    return m_tileCache.find(key);
}

std::vector<DecodedTile> StaticTileSurface::tiles() const { return m_tileCache.tiles(); }

void StaticTileSurface::insertTile(DecodedTile tile) { m_tileCache.insert(std::move(tile)); }

void StaticTileSurface::clearTiles() { m_tileCache.clear(); }

qsizetype StaticTileSurface::defaultTileCacheByteBudget()
{
    const std::optional<qsizetype> systemMemory = systemMemoryByteSize();
    if (!systemMemory.has_value()) {
        return imageFullDecodeFallbackByteLimit;
    }

    return tileCacheByteBudgetForSystemMemory(*systemMemory);
}

qsizetype StaticTileSurface::tileCacheByteBudgetForSystemMemory(qsizetype systemMemoryByteSize)
{
    if (systemMemoryByteSize <= 0) {
        return imageFullDecodeFallbackByteLimit;
    }

    return std::min(imageFullDecodeFallbackByteLimit, systemMemoryByteSize / 16);
}

bool staticImageFitsFullImageSurface(
    const ImageTileSource &source, const QImage &preview, int maximumTextureSize)
{
    const QSize imageSize = source.imageSize();
    if (imageSize.isEmpty() || preview.isNull() || preview.size() != imageSize
        || maximumTextureSize <= 0) {
        return false;
    }

    return imageSize.width() <= maximumTextureSize && imageSize.height() <= maximumTextureSize;
}

QSize displayedImageSurfaceSize(const DisplayedImageSurface &surface)
{
    return std::visit(
        [](const auto &payload) -> QSize {
            using Payload = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<Payload, LegacyFrameSurface>) {
                return payload.image.size();
            } else {
                return payload.imageSize();
            }
        },
        surface);
}

bool displayedImageSurfaceIsNull(const DisplayedImageSurface &surface)
{
    return std::visit(
        [](const auto &payload) {
            using Payload = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<Payload, LegacyFrameSurface>) {
                return payload.image.isNull();
            } else {
                return !payload.isValid();
            }
        },
        surface);
}
}
