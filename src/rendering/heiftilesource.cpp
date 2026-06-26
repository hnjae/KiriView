// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "heiftilesource.h"

#include "cache/imagebytecost.h"
#include "decoding/heifcontainer.h"
#include "decoding/heifsupport.h"
#include "imagetilesourcehelpers_p.h"
#include "localization/imageerrortext.h"

#include <libheif/heif_tiling.h>

#include <QPainter>
#include <QRectF>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

namespace kiriview {
HeifTileSource::HeifTileSource(
    QByteArray data, QSize imageSize, std::optional<HeifTileGrid> tileGrid)
    : m_data(std::move(data))
    , m_imageSize(std::move(imageSize))
    , m_tileGrid(std::move(tileGrid))
{
}

QSize HeifTileSource::imageSize() const { return m_imageSize; }

qsizetype HeifTileSource::byteCost() const { return m_data.size(); }

bool HeifTileSource::supportsRasterDisplayRefinement() const { return true; }

QImage HeifTileSource::decodeRasterDisplayImage(const QSize& rasterSize, QString* errorString) const
{
    if (rasterSize.isEmpty()) {
        return {};
    }
    if (m_tileGrid.has_value()) {
        return decodeGridRasterDisplayImage(rasterSize, errorString);
    }
    return decodeFullOrScaled(rasterSize, errorString);
}

QImage HeifTileSource::decodeBlockingDisplayImage(int maximumLongEdge, QString* errorString) const
{
    return decodeFullOrScaled(boundedPreviewSize(m_imageSize, maximumLongEdge), errorString);
}

std::optional<DecodedTile> HeifTileSource::decodeTile(
    const TileRequest& request, QString* errorString) const
{
    if (!tileRequestCanDecode(request)) {
        return std::nullopt;
    }

    QImage sourceImage;
    if (m_tileGrid.has_value() && request.key.level == 0) {
        sourceImage = decodeGridSourceRect(request.sourceRect, errorString);
    } else {
        sourceImage = decodeFullOrScaled(request.levelSize, errorString);
        if (std::optional<DecodedTile> tile = decodedTileFromLevelImage(request, sourceImage)) {
            return tile;
        }
    }

    if (sourceImage.isNull()) {
        return std::nullopt;
    }

    std::optional<DecodedTile> tile = decodedTileFromSourceImage(request, sourceImage);
    if (!tile.has_value()) {
        setTileSourceError(errorString, imageErrorText(ImageErrorTextId::RenderTile));
        return std::nullopt;
    }
    return tile;
}

QImage HeifTileSource::decodeFullOrScaled(const QSize& targetSize, QString* errorString) const
{
    if (estimatedRgbaByteCost(m_imageSize) > imageFullDecodeFallbackByteLimit) {
        setTileSourceError(
            errorString, imageErrorText(ImageErrorTextId::HeifFullDecodeFallbackTooLarge));
        return {};
    }

    std::optional<HeifPrimaryImage> opened = openHeifPrimaryImage(m_data, errorString);
    if (!opened.has_value()) {
        return {};
    }

    HeifDecodingOptions options;
    HeifImage heifImage;
    const heif_error error = heif_decode_image(opened->handle.get(), heifImage.out(),
        heif_colorspace_RGB, heif_chroma_interleaved_RGBA, options.get());
    if (error.code != heif_error_Ok) {
        setTileSourceError(errorString,
            heifErrorString(
                imageErrorActionText(ImageErrorActionTextId::DecodePrimaryImage), error));
        return {};
    }

    std::optional<QImage> image = qImageFromHeifImage(heifImage.get(), errorString);
    if (!image.has_value()) {
        return {};
    }

    return scaledTileImage(*image, targetSize.isEmpty() ? m_imageSize : targetSize);
}

QImage HeifTileSource::decodeGridRasterDisplayImage(
    const QSize& rasterSize, QString* errorString) const
{
    if (!m_tileGrid.has_value()) {
        return {};
    }

    std::optional<HeifPrimaryImage> opened = openHeifPrimaryImage(m_data, errorString);
    if (!opened.has_value()) {
        return {};
    }

    QImage image(rasterSize, QImage::Format_RGBA8888_Premultiplied);
    if (image.isNull()) {
        setTileSourceError(errorString, imageErrorText(ImageErrorTextId::AllocateTile));
        return {};
    }
    image.fill(Qt::transparent);

    const qreal scaleX = static_cast<qreal>(rasterSize.width()) / m_imageSize.width();
    const qreal scaleY = static_cast<qreal>(rasterSize.height()) / m_imageSize.height();
    HeifDecodingOptions options;
    QPainter painter(&image);
    for (const HeifTileDecodeRegion& region :
        heifTileDecodeRegions(*m_tileGrid, QRect(QPoint(0, 0), m_imageSize))) {
        HeifImage heifImage;
        const heif_error error = heif_image_handle_decode_image_tile(opened->handle.get(),
            heifImage.out(), heif_colorspace_RGB, heif_chroma_interleaved_RGBA, options.get(),
            static_cast<std::uint32_t>(region.tileX), static_cast<std::uint32_t>(region.tileY));
        if (error.code != heif_error_Ok) {
            setTileSourceError(errorString,
                heifErrorString(
                    imageErrorActionText(ImageErrorActionTextId::DecodeHeifGridTile), error));
            return {};
        }

        std::optional<QImage> tileImage = qImageFromHeifImage(heifImage.get(), errorString);
        if (!tileImage.has_value()) {
            return {};
        }

        painter.drawImage(QRectF(region.targetPoint.x() * scaleX, region.targetPoint.y() * scaleY,
                              tileImage->width() * scaleX, tileImage->height() * scaleY),
            *tileImage);
    }

    return image;
}

QImage HeifTileSource::decodeGridSourceRect(const QRect& sourceRect, QString* errorString) const
{
    std::optional<HeifPrimaryImage> opened = openHeifPrimaryImage(m_data, errorString);
    if (!opened.has_value()) {
        return {};
    }

    QImage image(sourceRect.size(), QImage::Format_RGBA8888_Premultiplied);
    if (image.isNull()) {
        setTileSourceError(errorString, imageErrorText(ImageErrorTextId::AllocateTile));
        return {};
    }
    image.fill(Qt::transparent);

    HeifDecodingOptions options;
    QPainter painter(&image);
    for (const HeifTileDecodeRegion& region : heifTileDecodeRegions(*m_tileGrid, sourceRect)) {
        HeifImage heifImage;
        const heif_error error = heif_image_handle_decode_image_tile(opened->handle.get(),
            heifImage.out(), heif_colorspace_RGB, heif_chroma_interleaved_RGBA, options.get(),
            static_cast<std::uint32_t>(region.tileX), static_cast<std::uint32_t>(region.tileY));
        if (error.code != heif_error_Ok) {
            setTileSourceError(errorString,
                heifErrorString(
                    imageErrorActionText(ImageErrorActionTextId::DecodeHeifGridTile), error));
            return {};
        }

        std::optional<QImage> tileImage = qImageFromHeifImage(heifImage.get(), errorString);
        if (!tileImage.has_value()) {
            return {};
        }

        painter.drawImage(region.targetPoint, *tileImage);
    }

    return image;
}

std::shared_ptr<HeifTileSource> openHeifTileSource(const QByteArray& data, QString* errorString)
{
    if (!isLikelyHeifStillImageContainer(data)) {
        return {};
    }

    std::optional<HeifPrimaryImage> opened = openHeifPrimaryImage(data, errorString);
    if (!opened.has_value()) {
        return {};
    }

    const QSize imageSize(heif_image_handle_get_width(opened->handle.get()),
        heif_image_handle_get_height(opened->handle.get()));
    if (imageSize.isEmpty()) {
        setTileSourceError(errorString, imageErrorText(ImageErrorTextId::HeifImageSizeInvalid));
        return {};
    }

    heif_image_tiling tiling {};
    tiling.version = 1;
    const heif_error error = heif_image_handle_get_image_tiling(opened->handle.get(), 1, &tiling);
    std::optional<HeifTileGrid> tileGrid;
    if (error.code == heif_error_Ok) {
        tileGrid = heifTileGridForTiling(tiling);
    }

    return std::make_shared<HeifTileSource>(data, imageSize, std::move(tileGrid));
}
}
