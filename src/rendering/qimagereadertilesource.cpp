// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "qimagereadertilesource.h"

#include "cache/imagebytecost.h"
#include "decoding/bufferedimagereader.h"
#include "imagerendering.h"
#include "imagetilesourcehelpers_p.h"
#include "localization/imageerrortext.h"

#include <QImageIOHandler>
#include <memory>
#include <optional>
#include <utility>

namespace {
QString imageDataReadError()
{
    return KiriView::imageErrorText(KiriView::ImageErrorTextId::ReadImageData);
}

template <typename ConfigureReader>
QImage readBufferedImage(const QByteArray &data, const QByteArray &format, bool autoTransform,
    ConfigureReader configureReader, QString *errorString)
{
    KiriView::BufferedImageReader reader(data, format, autoTransform);
    if (!reader) {
        KiriView::setTileSourceError(errorString, imageDataReadError());
        return {};
    }

    configureReader(reader);
    QImage image = reader.read();
    if (image.isNull()) {
        KiriView::setTileSourceError(errorString, reader.errorString());
        return {};
    }

    return KiriView::displayReadyImage(image);
}
}

namespace KiriView {
std::shared_ptr<QImageReaderTileSource> QImageReaderTileSource::open(
    const QByteArray &data, const QByteArray &format, QString *errorString)
{
    BufferedImageReader reader(data, format);
    if (!reader) {
        setTileSourceError(errorString, imageDataReadError());
        return {};
    }

    const QSize imageSize = reader.size();
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
{
}

QSize QImageReaderTileSource::imageSize() const { return m_imageSize; }

std::optional<DecodedTile> QImageReaderTileSource::decodeTile(
    const TileRequest &request, QString *errorString) const
{
    if (!tileRequestCanDecode(request)) {
        return std::nullopt;
    }

    if (std::optional<DecodedTile> tile = decodeReaderClipTile(request, errorString)) {
        return tile;
    }

    if (std::optional<DecodedTile> tile = decodeCachedOrScaledLevelTile(request, errorString)) {
        return tile;
    }

    return decodeFullImageFallbackTile(request, errorString);
}

std::optional<DecodedTile> QImageReaderTileSource::decodeReaderClipTile(
    const TileRequest &request, QString *errorString) const
{
    if (m_hasTransform) {
        return std::nullopt;
    }

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
    if (clipped.isNull()) {
        return std::nullopt;
    }

    return decodedTileFromSourceImage(request, clipped);
}

std::optional<DecodedTile> QImageReaderTileSource::decodeCachedOrScaledLevelTile(
    const TileRequest &request, QString *errorString) const
{
    if (std::optional<QImage> cached = m_scaledLevelCache.find(request.key.level)) {
        if (std::optional<DecodedTile> tile = decodedTileFromLevelImage(request, *cached)) {
            return tile;
        }
    }

    QImage levelImage = readScaledImage(request.levelSize, errorString);
    if (levelImage.isNull()) {
        return std::nullopt;
    }

    m_scaledLevelCache.insert(request.key.level, levelImage);
    return decodedTileFromLevelImage(request, levelImage);
}

std::optional<DecodedTile> QImageReaderTileSource::decodeFullImageFallbackTile(
    const TileRequest &request, QString *errorString) const
{
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

    const QSize scaledSize = firstDisplayScaledImageSize(m_imageSize, context.physicalViewportSize);
    if (scaledSize.isEmpty()) {
        return {};
    }

    QImage image = readScaledImage(scaledSize, errorString);
    if (image.isNull()) {
        return { FirstDisplayImageDecodeStatus::Error, {}, 0.0 };
    }

    const qreal displayPixelsPerSourcePixel = imagePixelsPerSourcePixel(m_imageSize, image.size());
    if (displayPixelsPerSourcePixel <= 0.0) {
        setTileSourceError(
            errorString, imageErrorText(ImageErrorTextId::DetermineJpegFirstDisplaySize));
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

QImage QImageReaderTileSource::readScaledImage(const QSize &scaledSize, QString *errorString) const
{
    return readBufferedImage(
        m_data, m_format, true,
        [&scaledSize](BufferedImageReader &reader) {
            if (!scaledSize.isEmpty()) {
                reader.setScaledSize(scaledSize);
            }
        },
        errorString);
}

QImage QImageReaderTileSource::readFullImage(QString *errorString) const
{
    if (estimatedRgbaByteCost(m_imageSize) > imageFullDecodeFallbackByteLimit) {
        setTileSourceError(
            errorString, imageErrorText(ImageErrorTextId::ImageFullDecodeFallbackTooLarge));
        return {};
    }
    return readScaledImage(m_imageSize, errorString);
}

QImage QImageReaderTileSource::readSourceClip(const QRect &sourceRect, QString *errorString) const
{
    return readBufferedImage(
        m_data, m_format, false,
        [&sourceRect](BufferedImageReader &reader) { reader.setClipRect(sourceRect); },
        errorString);
}
}
