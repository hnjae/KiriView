// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilesource.h"

#include "imagerendering.h"
#include "imagetilesource_p.h"
#include "imageviewtext.h"

#include <QBuffer>
#include <QIODevice>
#include <QImageIOHandler>
#include <QImageReader>
#include <QMutexLocker>
#include <QPainter>
#include <QRectF>
#include <QSvgRenderer>
#include <algorithm>
#include <optional>
#include <utility>

namespace KiriView {
std::shared_ptr<QImageReaderTileSource> QImageReaderTileSource::open(
    const QByteArray &data, QString *errorString)
{
    QBuffer buffer;
    buffer.setData(data);
    if (!buffer.open(QIODevice::ReadOnly)) {
        setTileSourceError(errorString, imageViewText("Could not read the selected image data."));
        return {};
    }

    QImageReader reader(&buffer);
    reader.setAutoTransform(true);
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
        QBuffer buffer;
        buffer.setData(m_data);
        if (buffer.open(QIODevice::ReadOnly)) {
            QImageReader reader(&buffer, m_format);
            reader.setAutoTransform(true);
            reader.setScaledSize(request.levelSize);
            reader.setScaledClipRect(request.textureLevelRect);
            QImage image = reader.read();
            if (!image.isNull() && image.size() == request.textureLevelRect.size()) {
                return decodedTileFromImage(request, std::move(image));
            }
        }

        QImage clipped = readSourceClip(request.sourceRect, errorString);
        if (!clipped.isNull()) {
            QImage scaled = scaledTileImage(clipped, request.textureLevelRect.size());
            if (!scaled.isNull()) {
                return decodedTileFromImage(request, std::move(scaled));
            }
        }
    }

    if (std::optional<QImage> cached = cachedScaledLevel(request.key.level)) {
        QImage image = cropLevelTexture(*cached, request.textureLevelRect);
        if (!image.isNull()) {
            return decodedTileFromImage(request, std::move(image));
        }
    }

    QImage levelImage = readScaledImage(request.levelSize, errorString);
    if (!levelImage.isNull()) {
        cacheScaledLevel(request.key.level, levelImage);
        QImage image = cropLevelTexture(levelImage, request.textureLevelRect);
        if (!image.isNull()) {
            return decodedTileFromImage(request, std::move(image));
        }
    }

    QImage fullImage = readFullImage(errorString);
    if (fullImage.isNull()) {
        return std::nullopt;
    }

    QImage levelFallback = scaledTileImage(fullImage, request.levelSize);
    QImage image = cropLevelTexture(levelFallback, request.textureLevelRect);
    if (image.isNull()) {
        return std::nullopt;
    }
    return decodedTileFromImage(request, std::move(image));
}

QImage QImageReaderTileSource::decodePreview(int maximumLongEdge, QString *errorString) const
{
    return readScaledImage(boundedPreviewSize(m_imageSize, maximumLongEdge), errorString);
}

qsizetype QImageReaderTileSource::byteCost() const { return m_data.size(); }

QImage QImageReaderTileSource::readScaledImage(const QSize &scaledSize, QString *errorString) const
{
    QBuffer buffer;
    buffer.setData(m_data);
    if (!buffer.open(QIODevice::ReadOnly)) {
        setTileSourceError(errorString, imageViewText("Could not read the selected image data."));
        return {};
    }

    QImageReader reader(&buffer, m_format);
    reader.setAutoTransform(true);
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
    QBuffer buffer;
    buffer.setData(m_data);
    if (!buffer.open(QIODevice::ReadOnly)) {
        setTileSourceError(errorString, imageViewText("Could not read the selected image data."));
        return {};
    }

    QImageReader reader(&buffer, m_format);
    reader.setAutoTransform(false);
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

std::shared_ptr<SvgTileSource> SvgTileSource::open(const QByteArray &data, QString *errorString)
{
    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        return {};
    }

    const QSize intrinsicSize = svgIntrinsicSize(renderer);
    if (intrinsicSize.isEmpty()) {
        setTileSourceError(
            errorString, imageViewText("Could not determine the selected SVG image size."));
        return {};
    }

    return std::make_shared<SvgTileSource>(data, intrinsicSize);
}

SvgTileSource::SvgTileSource(QByteArray data, QSize imageSize)
    : m_data(std::move(data))
    , m_imageSize(std::move(imageSize))
    , m_pyramid(m_imageSize)
{
}

QSize SvgTileSource::imageSize() const { return m_imageSize; }

int SvgTileSource::levelCount() const { return m_pyramid.levelCount(); }

QSize SvgTileSource::levelSize(int level) const { return m_pyramid.levelSize(level); }

std::optional<DecodedTile> SvgTileSource::decodeTile(
    const TileRequest &request, QString *errorString) const
{
    QSvgRenderer renderer(m_data);
    if (!renderer.isValid()) {
        setTileSourceError(errorString, imageViewText("Could not decode the selected SVG image."));
        return std::nullopt;
    }

    QImage image(request.textureLevelRect.size(), QImage::Format_RGBA8888_Premultiplied);
    if (image.isNull()) {
        setTileSourceError(errorString, imageViewText("Could not render the selected SVG tile."));
        return std::nullopt;
    }

    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.scale(static_cast<qreal>(image.width()) / request.textureLevelRect.width(),
        static_cast<qreal>(image.height()) / request.textureLevelRect.height());
    painter.translate(-request.textureLevelRect.x(), -request.textureLevelRect.y());
    renderer.render(&painter, QRectF(QPointF(0.0, 0.0), QSizeF(request.levelSize)));
    return decodedTileFromImage(request, std::move(image));
}

QImage SvgTileSource::decodePreview(int maximumLongEdge, QString *errorString) const
{
    const QSize previewSize = boundedPreviewSize(m_imageSize, maximumLongEdge);
    const QImage preview = renderSvgImage(m_data, previewSize);
    if (preview.isNull()) {
        setTileSourceError(errorString, imageViewText("Could not render the selected SVG image."));
        return {};
    }
    return preview;
}

qsizetype SvgTileSource::byteCost() const { return m_data.size(); }
}
