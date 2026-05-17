// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilesource_p.h"

#include "imagerendering.h"

#include <optional>
#include <utility>

namespace KiriView {
QSize boundedPreviewSize(const QSize &imageSize, int maximumLongEdge)
{
    return scaledImageSizeToFit(QSizeF(imageSize), QSize(maximumLongEdge, maximumLongEdge));
}

bool tileRequestCanDecode(const TileRequest &request)
{
    return !request.textureLevelRect.isEmpty() && !request.sourceRect.isEmpty();
}

QImage scaledTileImage(const QImage &image, const QSize &size)
{
    if (image.isNull() || size.isEmpty()) {
        return {};
    }
    if (image.size() == size) {
        return displayReadyImage(image);
    }
    return displayReadyImage(image.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

QImage cropLevelTexture(const QImage &levelImage, const QRect &textureLevelRect)
{
    if (levelImage.isNull() || textureLevelRect.isEmpty()) {
        return {};
    }
    return displayReadyImage(levelImage.copy(textureLevelRect));
}

DecodedTile decodedTileFromImage(const TileRequest &request, QImage image)
{
    return DecodedTile {
        request.key,
        request.levelSize,
        request.levelRect,
        request.textureLevelRect,
        displayReadyImage(image),
    };
}

std::optional<DecodedTile> decodedTileFromLevelImage(
    const TileRequest &request, const QImage &levelImage)
{
    QImage image = cropLevelTexture(levelImage, request.textureLevelRect);
    if (image.isNull()) {
        return std::nullopt;
    }

    return decodedTileFromImage(request, std::move(image));
}

std::optional<DecodedTile> decodedTileFromSourceImage(
    const TileRequest &request, const QImage &sourceImage)
{
    QImage image = scaledTileImage(sourceImage, request.textureLevelRect.size());
    if (image.isNull()) {
        return std::nullopt;
    }

    return decodedTileFromImage(request, std::move(image));
}

void setTileSourceError(QString *errorString, const QString &message)
{
    if (errorString != nullptr) {
        *errorString = message;
    }
}
}
