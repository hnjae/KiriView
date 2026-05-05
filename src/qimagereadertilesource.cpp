// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilesource.h"

#include "bufferedimagereader.h"
#include "imagebytecost.h"
#include "imagerendering.h"
#include "imagetilesource_p.h"
#include "imageviewtext.h"

#include <QImageIOHandler>
#include <QMutexLocker>
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>

namespace KiriView {
std::shared_ptr<QImageReaderTileSource> QImageReaderTileSource::open(
    const QByteArray &data, QString *errorString)
{
    BufferedImageReader reader(data);
    if (!reader) {
        setTileSourceError(errorString, imageViewText("Could not read the selected image data."));
        return {};
    }

    const QSize imageSize = reader.size();
    const QByteArray format = reader.format();
    if (imageSize.isEmpty()) {
        setTileSourceError(errorString, reader.errorString());
        return {};
    }

    const bool hasTransform = reader.transformation() != QImageIOHandler::TransformationNone;
    return std::make_shared<QImageReaderTileSource>(data, format, imageSize, hasTransform);
}

QImageReaderTileSource::QImageReaderTileSource(
    QByteArray data, QByteArray format, QSize imageSize, bool hasTransform)
    : m_data(std::move(data))
    , m_format(std::move(format))
    , m_imageSize(std::move(imageSize))
    , m_hasTransform(hasTransform)
    , m_pyramid(m_imageSize)
{
}

QSize QImageReaderTileSource::imageSize() const { return m_imageSize; }

int QImageReaderTileSource::levelCount() const { return m_pyramid.levelCount(); }

QSize QImageReaderTileSource::levelSize(int level) const { return m_pyramid.levelSize(level); }

std::optional<DecodedTile> QImageReaderTileSource::decodeTile(
    const TileRequest &request, QString *errorString) const
{
    if (request.textureLevelRect.isEmpty() || request.sourceRect.isEmpty()) {
        return std::nullopt;
    }

    if (!m_hasTransform) {
        BufferedImageReader reader(m_data, m_format);
        if (reader) {
            reader.setScaledSize(request.levelSize);
            reader.setScaledClipRect(request.textureLevelRect);
            QImage image = reader.read();
            if (!image.isNull() && image.size() == request.textureLevelRect.size()) {
                return decodedTileFromImage(request, std::move(image));
            }
        }

        QImage clipped = readSourceClip(request.sourceRect, errorString);
        if (!clipped.isNull()) {
            if (std::optional<DecodedTile> tile = decodedTileFromSourceImage(request, clipped)) {
                return tile;
            }
        }
    }

    if (std::optional<QImage> cached = cachedScaledLevel(request.key.level)) {
        if (std::optional<DecodedTile> tile = decodedTileFromLevelImage(request, *cached)) {
            return tile;
        }
    }

    QImage levelImage = readScaledImage(request.levelSize, errorString);
    if (!levelImage.isNull()) {
        cacheScaledLevel(request.key.level, levelImage);
        if (std::optional<DecodedTile> tile = decodedTileFromLevelImage(request, levelImage)) {
            return tile;
        }
    }

    QImage fullImage = readFullImage(errorString);
    if (fullImage.isNull()) {
        return std::nullopt;
    }

    QImage levelFallback = scaledTileImage(fullImage, request.levelSize);
    return decodedTileFromLevelImage(request, levelFallback);
}

FirstDisplayImageDecodeResult QImageReaderTileSource::decodeFirstDisplayImage(
    const ImageFirstDisplayDecodeContext &context, QString *errorString) const
{
    if (!context.isValid() || !supportsJpegScaledFirstDisplay()) {
        return {};
    }

    const QSize scaledSize = firstDisplayScaledSize(context.physicalViewportSize);
    if (scaledSize.isEmpty()) {
        return {};
    }

    QImage image = readScaledImage(scaledSize, errorString);
    if (image.isNull()) {
        return { FirstDisplayImageDecodeStatus::Error, {}, 0.0 };
    }

    const qreal displayPixelsPerSourcePixel
        = std::min(static_cast<qreal>(image.width()) / m_imageSize.width(),
            static_cast<qreal>(image.height()) / m_imageSize.height());
    if (!std::isfinite(displayPixelsPerSourcePixel) || displayPixelsPerSourcePixel <= 0.0) {
        setTileSourceError(errorString,
            imageViewText("Could not determine the selected JPEG first-display size."));
        return { FirstDisplayImageDecodeStatus::Error, {}, 0.0 };
    }

    return { FirstDisplayImageDecodeStatus::Ready, std::move(image), displayPixelsPerSourcePixel };
}

QImage QImageReaderTileSource::decodeBlockingDisplayImage(
    int maximumLongEdge, QString *errorString) const
{
    return readScaledImage(boundedPreviewSize(m_imageSize, maximumLongEdge), errorString);
}

qsizetype QImageReaderTileSource::byteCost() const { return m_data.size(); }

bool QImageReaderTileSource::supportsJpegScaledFirstDisplay() const
{
    const QByteArray format = m_format.toLower();
    return format == QByteArrayLiteral("jpg") || format == QByteArrayLiteral("jpeg");
}

QSize QImageReaderTileSource::firstDisplayScaledSize(const QSize &physicalViewportSize) const
{
    if (m_imageSize.isEmpty() || physicalViewportSize.isEmpty()) {
        return {};
    }
    if (m_imageSize.width() <= physicalViewportSize.width()
        && m_imageSize.height() <= physicalViewportSize.height()) {
        return {};
    }

    const qreal scale = std::min(
        static_cast<qreal>(physicalViewportSize.width()) / static_cast<qreal>(m_imageSize.width()),
        static_cast<qreal>(physicalViewportSize.height())
            / static_cast<qreal>(m_imageSize.height()));
    if (!std::isfinite(scale) || scale <= 0.0 || scale >= 1.0) {
        return {};
    }

    const int width = std::clamp(
        static_cast<int>(std::ceil(m_imageSize.width() * scale)), 1, m_imageSize.width());
    const int height = std::clamp(
        static_cast<int>(std::ceil(m_imageSize.height() * scale)), 1, m_imageSize.height());
    if (width >= m_imageSize.width() && height >= m_imageSize.height()) {
        return {};
    }

    return QSize(width, height);
}

QImage QImageReaderTileSource::readScaledImage(const QSize &scaledSize, QString *errorString) const
{
    BufferedImageReader reader(m_data, m_format);
    if (!reader) {
        setTileSourceError(errorString, imageViewText("Could not read the selected image data."));
        return {};
    }

    if (!scaledSize.isEmpty()) {
        reader.setScaledSize(scaledSize);
    }

    QImage image = reader.read();
    if (image.isNull()) {
        setTileSourceError(errorString, reader.errorString());
        return {};
    }
    return displayReadyImage(image);
}

QImage QImageReaderTileSource::readFullImage(QString *errorString) const
{
    if (estimatedRgbaByteCost(m_imageSize) > imageFullDecodeFallbackByteLimit) {
        setTileSourceError(errorString,
            imageViewText("The selected image is too large for fallback full-image decoding."));
        return {};
    }
    return readScaledImage(m_imageSize, errorString);
}

QImage QImageReaderTileSource::readSourceClip(const QRect &sourceRect, QString *errorString) const
{
    BufferedImageReader reader(m_data, m_format, false);
    if (!reader) {
        setTileSourceError(errorString, imageViewText("Could not read the selected image data."));
        return {};
    }

    reader.setClipRect(sourceRect);
    QImage image = reader.read();
    if (image.isNull()) {
        setTileSourceError(errorString, reader.errorString());
        return {};
    }
    return displayReadyImage(image);
}

std::optional<QImage> QImageReaderTileSource::cachedScaledLevel(int level) const
{
    QMutexLocker locker(&m_scaledLevelCacheMutex);
    const auto cached = std::find_if(m_scaledLevelCache.cbegin(), m_scaledLevelCache.cend(),
        [level](const auto &entry) { return entry.first == level; });
    if (cached == m_scaledLevelCache.cend()) {
        return std::nullopt;
    }
    return cached->second;
}

void QImageReaderTileSource::cacheScaledLevel(int level, const QImage &image) const
{
    if (image.isNull() || estimatedRgbaByteCost(image.size()) > imageFullDecodeFallbackByteLimit) {
        return;
    }

    QMutexLocker locker(&m_scaledLevelCacheMutex);
    const auto cached = std::find_if(m_scaledLevelCache.begin(), m_scaledLevelCache.end(),
        [level](const auto &entry) { return entry.first == level; });
    if (cached != m_scaledLevelCache.end()) {
        cached->second = image;
        return;
    }
    m_scaledLevelCache.push_back({ level, image });
}
}
