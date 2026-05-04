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
inline constexpr int imageBlockingDisplayLongEdgeMax = 2048;
inline constexpr qsizetype imageFullDecodeFallbackByteLimit = 512 * 1024 * 1024;

struct ImageFirstDisplayDecodeContext {
    QSize physicalViewportSize;

    bool isValid() const { return !physicalViewportSize.isEmpty(); }
};

enum class FirstDisplayImageDecodeStatus {
    Ready,
    NotImplemented,
    Error,
};

struct FirstDisplayImageDecodeResult {
    FirstDisplayImageDecodeStatus status = FirstDisplayImageDecodeStatus::NotImplemented;
    QImage image;
    qreal displayPixelsPerSourcePixel = 0.0;
};

struct StaticImageDisplayHints {
    qreal firstDisplayPixelsPerSourcePixel = 0.0;
};

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
    virtual FirstDisplayImageDecodeResult decodeFirstDisplayImage(
        const ImageFirstDisplayDecodeContext &context, QString *errorString) const
        = 0;
    virtual QImage decodeBlockingDisplayImage(int maximumLongEdge, QString *errorString) const = 0;
    virtual qsizetype byteCost() const = 0;
};

class StaticTileSurface
{
public:
    explicit StaticTileSurface(std::shared_ptr<ImageTileSource> source = {}, QImage preview = {},
        StaticImageDisplayHints displayHints = {});

    bool isValid() const;
    std::shared_ptr<ImageTileSource> source() const;
    const TilePyramid &pyramid() const;
    QSize imageSize() const;
    const QImage &preview() const;
    const StaticImageDisplayHints &displayHints() const;
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
    StaticImageDisplayHints m_displayHints;
    DecodedTileCache m_tileCache;
};

struct LegacyFrameSurface {
    QImage image;
};

using DisplayedImageSurface = std::variant<LegacyFrameSurface, StaticTileSurface>;

bool staticImageFitsFullImageSurface(
    const ImageTileSource &source, const QImage &preview, int maximumTextureSize);
QSize displayedImageSurfaceSize(const DisplayedImageSurface &surface);
bool displayedImageSurfaceIsNull(const DisplayedImageSurface &surface);
}

#endif
