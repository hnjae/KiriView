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
    virtual std::optional<DecodedTile> decodeTile(
        const TileRequest &request, QString *errorString) const
        = 0;
    virtual FirstDisplayImageDecodeResult decodeFirstDisplayImage(
        const ImageFirstDisplayDecodeContext &context, QString *errorString) const
        = 0;
    virtual QImage decodeBlockingDisplayImage(int maximumLongEdge, QString *errorString) const = 0;
    virtual qsizetype byteCost() const = 0;
};

struct StaticImagePayload {
    std::shared_ptr<ImageTileSource> source;
    QImage preview;
    StaticImageDisplayHints displayHints;

    bool isValid() const;
    qsizetype byteCost() const;
    std::optional<qsizetype> byteCostWithinBudget(qsizetype byteBudget) const;
};

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

using DisplayedImageSurface = std::variant<LegacyFrameSurface, StaticTileSurface>;

bool staticImageFitsFullImageSurface(const StaticImagePayload &image, int maximumTextureSize);
QSize displayedImageSurfaceSize(const DisplayedImageSurface &surface);
bool displayedImageSurfaceIsNull(const DisplayedImageSurface &surface);
}

#endif
