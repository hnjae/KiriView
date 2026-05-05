// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilesource.h"

#include "heifcontainer.h"
#include "heifsupport.h"
#include "imagebytecost.h"
#include "imagetilesource_p.h"
#include "imageviewtext.h"

#include <libheif/heif_tiling.h>

#include <QPainter>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

namespace {
class HeifTileSource final : public KiriView::ImageTileSource
{
public:
    HeifTileSource(QByteArray data, QSize imageSize, heif_image_tiling tiling, bool tiled)
        : m_data(std::move(data))
        , m_imageSize(std::move(imageSize))
        , m_tiling(tiling)
        , m_tiled(tiled)
        , m_pyramid(m_imageSize)
    {
    }

    QSize imageSize() const override { return m_imageSize; }
    int levelCount() const override { return m_pyramid.levelCount(); }
    QSize levelSize(int level) const override { return m_pyramid.levelSize(level); }
    qsizetype byteCost() const override { return m_data.size(); }

    KiriView::FirstDisplayImageDecodeResult decodeFirstDisplayImage(
        const KiriView::ImageFirstDisplayDecodeContext &context,
        QString *errorString) const override
    {
        Q_UNUSED(context);
        Q_UNUSED(errorString);
        // TODO: Add a HEIF-family first-display path that can avoid full fallback decode.
        return {};
    }

    QImage decodeBlockingDisplayImage(int maximumLongEdge, QString *errorString) const override
    {
        return decodeFullOrScaled(
            KiriView::boundedPreviewSize(m_imageSize, maximumLongEdge), errorString);
    }

    std::optional<KiriView::DecodedTile> decodeTile(
        const KiriView::TileRequest &request, QString *errorString) const override
    {
        if (request.textureLevelRect.isEmpty() || request.sourceRect.isEmpty()) {
            return std::nullopt;
        }

        QImage sourceImage;
        if (m_tiled && request.key.level == 0) {
            sourceImage = decodeGridSourceRect(request.sourceRect, errorString);
        } else {
            sourceImage = decodeFullOrScaled(request.levelSize, errorString);
            sourceImage = KiriView::cropLevelTexture(sourceImage, request.textureLevelRect);
            if (!sourceImage.isNull()) {
                return KiriView::decodedTileFromImage(request, sourceImage);
            }
        }

        if (sourceImage.isNull()) {
            return std::nullopt;
        }

        QImage image = KiriView::scaledTileImage(sourceImage, request.textureLevelRect.size());
        if (image.isNull()) {
            KiriView::setTileSourceError(
                errorString, KiriView::imageViewText("Could not render the selected tile."));
            return std::nullopt;
        }
        return KiriView::decodedTileFromImage(request, std::move(image));
    }

private:
    QImage decodeFullOrScaled(const QSize &targetSize, QString *errorString) const
    {
        if (KiriView::estimatedRgbaByteCost(m_imageSize)
            > KiriView::imageFullDecodeFallbackByteLimit) {
            KiriView::setTileSourceError(errorString,
                KiriView::imageViewText("The selected HEIF image is too large for fallback "
                                        "full-image decoding."));
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
            KiriView::setTileSourceError(
                errorString, KiriView::heifErrorString("decoding the primary image", error));
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
                errorString, KiriView::imageViewText("Could not allocate the selected tile."));
            return {};
        }
        image.fill(Qt::transparent);

        KiriView::HeifDecodingOptions options;
        QPainter painter(&image);
        const int firstTileX
            = std::max(0, sourceRect.left() / static_cast<int>(m_tiling.tile_width));
        const int firstTileY
            = std::max(0, sourceRect.top() / static_cast<int>(m_tiling.tile_height));
        const int lastTileX = std::min<int>(
            m_tiling.num_columns - 1, sourceRect.right() / static_cast<int>(m_tiling.tile_width));
        const int lastTileY = std::min<int>(
            m_tiling.num_rows - 1, sourceRect.bottom() / static_cast<int>(m_tiling.tile_height));

        for (int tileY = firstTileY; tileY <= lastTileY; ++tileY) {
            for (int tileX = firstTileX; tileX <= lastTileX; ++tileX) {
                KiriView::HeifImage heifImage;
                const heif_error error
                    = heif_image_handle_decode_image_tile(opened->handle.get(), heifImage.out(),
                        heif_colorspace_RGB, heif_chroma_interleaved_RGBA, options.get(),
                        static_cast<std::uint32_t>(tileX), static_cast<std::uint32_t>(tileY));
                if (error.code != heif_error_Ok) {
                    KiriView::setTileSourceError(
                        errorString, KiriView::heifErrorString("decoding a HEIF grid tile", error));
                    return {};
                }

                std::optional<QImage> tileImage
                    = KiriView::qImageFromHeifImage(heifImage.get(), errorString);
                if (!tileImage.has_value()) {
                    return {};
                }

                const QPoint target(tileX * static_cast<int>(m_tiling.tile_width) - sourceRect.x(),
                    tileY * static_cast<int>(m_tiling.tile_height) - sourceRect.y());
                painter.drawImage(target, *tileImage);
            }
        }

        return image;
    }

    QByteArray m_data;
    QSize m_imageSize;
    heif_image_tiling m_tiling {};
    bool m_tiled = false;
    KiriView::TilePyramid m_pyramid;
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
        setTileSourceError(errorString,
            imageViewText("Could not decode the selected HEIF image: image size is invalid."));
        return {};
    }

    heif_image_tiling tiling {};
    tiling.version = 1;
    heif_error error = heif_image_handle_get_image_tiling(opened->handle.get(), 1, &tiling);
    const bool tiled = error.code == heif_error_Ok && tiling.num_columns > 0 && tiling.num_rows > 0
        && tiling.tile_width > 0 && tiling.tile_height > 0
        && (tiling.num_columns > 1 || tiling.num_rows > 1);

    return std::make_shared<HeifTileSource>(data, imageSize, tiling, tiled);
}
}
