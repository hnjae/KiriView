// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "heiftilesource.h"

#include "decoding/heifcontainer.h"
#include "decoding/heifsupport.h"
#include "decoding/heiftilingplan.h"
#include "imagebytecost.h"
#include "imageerrortext.h"
#include "imagetilesourcehelpers_p.h"

#include <libheif/heif_tiling.h>

#include <QPainter>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

namespace {
class HeifTileSource final : public KiriView::ImageTileSource
{
public:
    HeifTileSource(QByteArray data, QSize imageSize, std::optional<KiriView::HeifTileGrid> tileGrid)
        : m_data(std::move(data))
        , m_imageSize(std::move(imageSize))
        , m_tileGrid(std::move(tileGrid))
    {
    }

    QSize imageSize() const override { return m_imageSize; }
    qsizetype byteCost() const override { return m_data.size(); }

    QImage decodeBlockingDisplayImage(int maximumLongEdge, QString *errorString) const override
    {
        return decodeFullOrScaled(
            KiriView::boundedPreviewSize(m_imageSize, maximumLongEdge), errorString);
    }

    std::optional<KiriView::DecodedTile> decodeTile(
        const KiriView::TileRequest &request, QString *errorString) const override
    {
        if (!KiriView::tileRequestCanDecode(request)) {
            return std::nullopt;
        }

        QImage sourceImage;
        if (m_tileGrid.has_value() && request.key.level == 0) {
            sourceImage = decodeGridSourceRect(request.sourceRect, errorString);
        } else {
            sourceImage = decodeFullOrScaled(request.levelSize, errorString);
            if (std::optional<KiriView::DecodedTile> tile
                = KiriView::decodedTileFromLevelImage(request, sourceImage)) {
                return tile;
            }
        }

        if (sourceImage.isNull()) {
            return std::nullopt;
        }

        std::optional<KiriView::DecodedTile> tile
            = KiriView::decodedTileFromSourceImage(request, sourceImage);
        if (!tile.has_value()) {
            KiriView::setTileSourceError(
                errorString, KiriView::imageErrorText(KiriView::ImageErrorTextId::RenderTile));
            return std::nullopt;
        }
        return tile;
    }

private:
    QImage decodeFullOrScaled(const QSize &targetSize, QString *errorString) const
    {
        if (KiriView::estimatedRgbaByteCost(m_imageSize)
            > KiriView::imageFullDecodeFallbackByteLimit) {
            KiriView::setTileSourceError(errorString,
                KiriView::imageErrorText(
                    KiriView::ImageErrorTextId::HeifFullDecodeFallbackTooLarge));
            return {};
        }

        std::optional<KiriView::HeifPrimaryImage> opened
            = KiriView::openHeifPrimaryImage(m_data, errorString);
        if (!opened.has_value()) {
            return {};
        }

        KiriView::HeifDecodingOptions options;
        KiriView::HeifImage heifImage;
        heif_error error = heif_decode_image(opened->handle.get(), heifImage.out(),
            heif_colorspace_RGB, heif_chroma_interleaved_RGBA, options.get());
        if (error.code != heif_error_Ok) {
            KiriView::setTileSourceError(errorString,
                KiriView::heifErrorString(KiriView::imageErrorActionText(
                                              KiriView::ImageErrorActionTextId::DecodePrimaryImage),
                    error));
            return {};
        }

        std::optional<QImage> image = KiriView::qImageFromHeifImage(heifImage.get(), errorString);
        if (!image.has_value()) {
            return {};
        }

        return KiriView::scaledTileImage(*image, targetSize.isEmpty() ? m_imageSize : targetSize);
    }

    QImage decodeGridSourceRect(const QRect &sourceRect, QString *errorString) const
    {
        std::optional<KiriView::HeifPrimaryImage> opened
            = KiriView::openHeifPrimaryImage(m_data, errorString);
        if (!opened.has_value()) {
            return {};
        }

        QImage image(sourceRect.size(), QImage::Format_RGBA8888_Premultiplied);
        if (image.isNull()) {
            KiriView::setTileSourceError(
                errorString, KiriView::imageErrorText(KiriView::ImageErrorTextId::AllocateTile));
            return {};
        }
        image.fill(Qt::transparent);

        KiriView::HeifDecodingOptions options;
        QPainter painter(&image);
        for (const KiriView::HeifTileDecodeRegion &region :
            KiriView::heifTileDecodeRegions(*m_tileGrid, sourceRect)) {
            KiriView::HeifImage heifImage;
            const heif_error error = heif_image_handle_decode_image_tile(opened->handle.get(),
                heifImage.out(), heif_colorspace_RGB, heif_chroma_interleaved_RGBA, options.get(),
                static_cast<std::uint32_t>(region.tileX), static_cast<std::uint32_t>(region.tileY));
            if (error.code != heif_error_Ok) {
                KiriView::setTileSourceError(errorString,
                    KiriView::heifErrorString(
                        KiriView::imageErrorActionText(
                            KiriView::ImageErrorActionTextId::DecodeHeifGridTile),
                        error));
                return {};
            }

            std::optional<QImage> tileImage
                = KiriView::qImageFromHeifImage(heifImage.get(), errorString);
            if (!tileImage.has_value()) {
                return {};
            }

            painter.drawImage(region.targetPoint, *tileImage);
        }

        return image;
    }

    QByteArray m_data;
    QSize m_imageSize;
    std::optional<KiriView::HeifTileGrid> m_tileGrid;
};
}

namespace KiriView {
std::shared_ptr<ImageTileSource> openHeifTileSource(const QByteArray &data, QString *errorString)
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
    heif_error error = heif_image_handle_get_image_tiling(opened->handle.get(), 1, &tiling);
    std::optional<HeifTileGrid> tileGrid;
    if (error.code == heif_error_Ok) {
        tileGrid = heifTileGridForTiling(tiling);
    }

    return std::make_shared<HeifTileSource>(data, imageSize, std::move(tileGrid));
}
}
