// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "qimagereadertilesource.h"

#include "cache/imagebytecost.h"
#include "decoding/bufferedimagereader.h"
#include "imagerendering.h"
#include "imagetilesourcehelpers_p.h"
#include "localization/imageerrortext.h"

#include <QImageIOHandler>
#include <QTransform>
#include <memory>
#include <optional>
#include <utility>

namespace {
QString imageDataReadError()
{
    return kiriview::imageErrorText(kiriview::ImageErrorTextId::ReadImageData);
}

QSize transformedImageSize(const QSize& size, QImageIOHandler::Transformations transformations)
{
    if (size.isEmpty()) {
        return {};
    }
    if (transformations.toInt() & QImageIOHandler::TransformationRotate90) {
        return size.transposed();
    }
    return size;
}

QImage transformedImage(QImage image, QImageIOHandler::Transformations transformations)
{
    if (image.isNull() || transformations == QImageIOHandler::TransformationNone) {
        return image;
    }

    switch (static_cast<QImageIOHandler::Transformation>(transformations.toInt())) {
    case QImageIOHandler::TransformationNone:
        return image;
    case QImageIOHandler::TransformationMirror:
        return image.flipped(Qt::Horizontal);
    case QImageIOHandler::TransformationFlip:
        return image.flipped(Qt::Vertical);
    case QImageIOHandler::TransformationRotate180:
        return image.transformed(QTransform().rotate(180));
    case QImageIOHandler::TransformationRotate90:
        return image.transformed(QTransform().rotate(90));
    case QImageIOHandler::TransformationMirrorAndRotate90:
        return image.flipped(Qt::Horizontal).transformed(QTransform().rotate(90));
    case QImageIOHandler::TransformationFlipAndRotate90:
        return image.flipped(Qt::Vertical).transformed(QTransform().rotate(90));
    case QImageIOHandler::TransformationRotate270:
        return image.transformed(QTransform().rotate(270));
    }

    return image;
}

void appendTileDecodeFailure(kiriview::QImageReaderTileDecodeDiagnostics* diagnostics,
    kiriview::QImageReaderTileDecodeAttemptKind kind, const QString& errorString)
{
    if (diagnostics == nullptr) {
        return;
    }

    diagnostics->failures.push_back(kiriview::QImageReaderTileDecodeAttemptFailure {
        kind,
        errorString.isEmpty() ? imageDataReadError() : errorString,
    });
}

void appendDisplayDecodeFailure(kiriview::QImageReaderDisplayDecodeDiagnostics* diagnostics,
    kiriview::QImageReaderDisplayDecodeOperation operation, const QString& errorString)
{
    if (diagnostics == nullptr) {
        return;
    }

    const QString message = errorString.isEmpty() ? imageDataReadError() : errorString;
    diagnostics->failures.push_back(kiriview::QImageReaderDisplayDecodeFailure {
        operation,
        message,
        message,
    });
}

template <typename ConfigureReader>
QImage readBufferedImage(const QByteArray& data, const QByteArray& format, bool autoTransform,
    ConfigureReader configureReader, QString* errorString)
{
    kiriview::BufferedImageReader reader(data, format, autoTransform);
    if (!reader) {
        kiriview::setTileSourceError(errorString, imageDataReadError());
        return {};
    }

    configureReader(reader);
    QImage image = reader.read();
    if (image.isNull()) {
        kiriview::setTileSourceError(errorString, reader.errorString());
        return {};
    }

    return kiriview::displayReadyImage(image);
}
}

namespace kiriview {
QString QImageReaderTileDecodeDiagnostics::userMessage() const
{
    for (auto failure = failures.crbegin(); failure != failures.crend(); ++failure) {
        if (!failure->errorString.isEmpty()) {
            return failure->errorString;
        }
    }
    return {};
}

std::shared_ptr<QImageReaderTileSource> QImageReaderTileSource::open(
    const QByteArray& data, const QByteArray& format, QString* errorString)
{
    BufferedImageReader reader(data, format);
    if (!reader) {
        setTileSourceError(errorString, imageDataReadError());
        return {};
    }

    const QImageIOHandler::Transformations transformations = reader.transformation();
    const QSize imageSize = transformedImageSize(reader.size(), transformations);
    if (imageSize.isEmpty()) {
        setTileSourceError(errorString, reader.errorString());
        return {};
    }

    return std::make_shared<QImageReaderTileSource>(
        data, format, imageSize, StaticImageReaderTransform { transformations });
}

QImageReaderTileSource::QImageReaderTileSource(
    QByteArray data, QByteArray format, QSize imageSize, StaticImageReaderTransform transform)
    : m_data(std::move(data))
    , m_format(std::move(format))
    , m_imageSize(std::move(imageSize))
    , m_transform(transform)
{
}

QSize QImageReaderTileSource::imageSize() const { return m_imageSize; }

std::optional<DecodedTile> QImageReaderTileSource::decodeTile(
    const TileRequest& request, QString* errorString) const
{
    QImageReaderTileDecodeResult result = decodeTileWithDiagnostics(request);
    if (!result.tile.has_value() && !result.diagnostics.failures.empty()) {
        setTileSourceError(errorString, result.diagnostics.userMessage());
    }
    return std::move(result.tile);
}

QImageReaderTileDecodeResult QImageReaderTileSource::decodeTileWithDiagnostics(
    const TileRequest& request) const
{
    QImageReaderTileDecodeResult result;
    if (!tileRequestCanDecode(request)) {
        return result;
    }

    if (std::optional<DecodedTile> tile = decodeReaderClipTile(request, &result.diagnostics)) {
        result.tile = std::move(tile);
        return result;
    }

    if (std::optional<DecodedTile> tile
        = decodeCachedOrScaledLevelTile(request, &result.diagnostics)) {
        result.tile = std::move(tile);
        return result;
    }

    result.tile = decodeFullImageFallbackTile(request, &result.diagnostics);
    return result;
}

std::optional<DecodedTile> QImageReaderTileSource::decodeReaderClipTile(
    const TileRequest& request, QImageReaderTileDecodeDiagnostics* diagnostics) const
{
    if (m_transform.hasTransform()) {
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
        appendTileDecodeFailure(
            diagnostics, QImageReaderTileDecodeAttemptKind::ReaderClip, reader.errorString());
    }

    QString sourceClipError;
    QImage clipped = readSourceClip(request.sourceRect, &sourceClipError);
    if (clipped.isNull()) {
        appendTileDecodeFailure(
            diagnostics, QImageReaderTileDecodeAttemptKind::SourceClip, sourceClipError);
        return std::nullopt;
    }

    return decodedTileFromSourceImage(request, clipped);
}

std::optional<DecodedTile> QImageReaderTileSource::decodeCachedOrScaledLevelTile(
    const TileRequest& request, QImageReaderTileDecodeDiagnostics* diagnostics) const
{
    if (std::optional<QImage> cached = m_scaledLevelCache.find(request.key.level)) {
        if (std::optional<DecodedTile> tile = decodedTileFromLevelImage(request, *cached)) {
            return tile;
        }
    }

    QString scaledLevelError;
    QImage levelImage = readScaledImage(request.levelSize, &scaledLevelError);
    if (levelImage.isNull()) {
        appendTileDecodeFailure(
            diagnostics, QImageReaderTileDecodeAttemptKind::ScaledLevel, scaledLevelError);
        return std::nullopt;
    }

    m_scaledLevelCache.insert(request.key.level, levelImage);
    return decodedTileFromLevelImage(request, levelImage);
}

std::optional<DecodedTile> QImageReaderTileSource::decodeFullImageFallbackTile(
    const TileRequest& request, QImageReaderTileDecodeDiagnostics* diagnostics) const
{
    QString fullImageError;
    QImage fullImage = readFullImage(&fullImageError);
    if (fullImage.isNull()) {
        appendTileDecodeFailure(
            diagnostics, QImageReaderTileDecodeAttemptKind::FullImageFallback, fullImageError);
        return std::nullopt;
    }

    QImage levelFallback = scaledTileImage(fullImage, request.levelSize);
    return decodedTileFromLevelImage(request, levelFallback);
}

QImageReaderFirstDisplayDecodeResult QImageReaderTileSource::decodeFirstDisplayImageWithDiagnostics(
    const ImageFirstDisplayDecodeContext& context) const
{
    QImageReaderFirstDisplayDecodeResult result;
    if (!context.isValid() || !supportsJpegScaledFirstDisplay()) {
        return result;
    }

    const QSize scaledSize = firstDisplayScaledImageSize(m_imageSize, context.physicalViewportSize);
    if (scaledSize.isEmpty()) {
        return result;
    }

    QString errorString;
    QImage image = readScaledImage(scaledSize, &errorString);
    if (image.isNull()) {
        appendDisplayDecodeFailure(&result.diagnostics,
            QImageReaderDisplayDecodeOperation::FirstDisplayImage, errorString);
        result.firstDisplay.status = FirstDisplayImageDecodeStatus::Error;
        return result;
    }

    const qreal displayPixelsPerSourcePixel = imagePixelsPerSourcePixel(m_imageSize, image.size());
    if (displayPixelsPerSourcePixel <= 0.0) {
        appendDisplayDecodeFailure(&result.diagnostics,
            QImageReaderDisplayDecodeOperation::FirstDisplayImage,
            imageErrorText(ImageErrorTextId::DetermineJpegFirstDisplaySize));
        result.firstDisplay.status = FirstDisplayImageDecodeStatus::Error;
        return result;
    }

    result.firstDisplay
        = { FirstDisplayImageDecodeStatus::Ready, std::move(image), displayPixelsPerSourcePixel };
    return result;
}

FirstDisplayImageDecodeResult QImageReaderTileSource::decodeFirstDisplayImage(
    const ImageFirstDisplayDecodeContext& context, QString* errorString) const
{
    QImageReaderFirstDisplayDecodeResult result = decodeFirstDisplayImageWithDiagnostics(context);
    if (result.firstDisplay.status == FirstDisplayImageDecodeStatus::Error
        && !result.diagnostics.failures.empty()) {
        setTileSourceError(errorString, result.diagnostics.userMessage());
    }
    return std::move(result.firstDisplay);
}

bool QImageReaderTileSource::supportsRasterDisplayRefinement() const { return true; }

QImageReaderDisplayDecodeResult QImageReaderTileSource::decodeRasterDisplayImageWithDiagnostics(
    const QSize& rasterSize) const
{
    if (rasterSize.isEmpty()) {
        return {};
    }

    return readScaledDisplayImage(
        rasterSize, QImageReaderDisplayDecodeOperation::RasterDisplayImage);
}

QImage QImageReaderTileSource::decodeRasterDisplayImage(
    const QSize& rasterSize, QString* errorString) const
{
    QImageReaderDisplayDecodeResult result = decodeRasterDisplayImageWithDiagnostics(rasterSize);
    if (result.image.isNull() && !result.diagnostics.failures.empty()) {
        setTileSourceError(errorString, result.diagnostics.userMessage());
    }
    return std::move(result.image);
}

QImageReaderDisplayDecodeResult QImageReaderTileSource::decodeBlockingDisplayImageWithDiagnostics(
    int maximumLongEdge) const
{
    return readScaledDisplayImage(boundedPreviewSize(m_imageSize, maximumLongEdge),
        QImageReaderDisplayDecodeOperation::BlockingDisplayImage);
}

QImage QImageReaderTileSource::decodeBlockingDisplayImage(
    int maximumLongEdge, QString* errorString) const
{
    QImageReaderDisplayDecodeResult result
        = decodeBlockingDisplayImageWithDiagnostics(maximumLongEdge);
    if (result.image.isNull() && !result.diagnostics.failures.empty()) {
        setTileSourceError(errorString, result.diagnostics.userMessage());
    }
    return std::move(result.image);
}

qsizetype QImageReaderTileSource::byteCost() const { return m_data.size(); }

StaticImageReaderTransform QImageReaderTileSource::imageReaderTransform() const
{
    return m_transform;
}

bool QImageReaderTileSource::supportsJpegScaledFirstDisplay() const
{
    const QByteArray format = m_format.toLower();
    return format == QByteArrayLiteral("jpg") || format == QByteArrayLiteral("jpeg");
}

QImageReaderDisplayDecodeResult QImageReaderTileSource::readScaledDisplayImage(
    const QSize& scaledSize, QImageReaderDisplayDecodeOperation operation) const
{
    QImageReaderDisplayDecodeResult result;
    QString errorString;
    result.image = readScaledImage(scaledSize, &errorString);
    if (result.image.isNull()) {
        appendDisplayDecodeFailure(&result.diagnostics, operation, errorString);
    }
    return result;
}

QImage QImageReaderTileSource::readScaledImage(const QSize& scaledSize, QString* errorString) const
{
    const bool hasTransform = m_transform.hasTransform();
    const QSize readerScaledSize
        = hasTransform ? transformedImageSize(scaledSize, m_transform.transformations) : scaledSize;
    QImage image = readBufferedImage(
        m_data, m_format, !hasTransform,
        [&readerScaledSize](BufferedImageReader& reader) {
            if (!readerScaledSize.isEmpty()) {
                reader.setScaledSize(readerScaledSize);
            }
        },
        errorString);
    if (image.isNull() || !hasTransform) {
        return image;
    }
    return displayReadyImage(transformedImage(std::move(image), m_transform.transformations));
}

QImage QImageReaderTileSource::readFullImage(QString* errorString) const
{
    if (estimatedRgbaByteCost(m_imageSize) > imageFullDecodeFallbackByteLimit) {
        setTileSourceError(
            errorString, imageErrorText(ImageErrorTextId::ImageFullDecodeFallbackTooLarge));
        return {};
    }
    return readScaledImage(m_imageSize, errorString);
}

QImage QImageReaderTileSource::readSourceClip(const QRect& sourceRect, QString* errorString) const
{
    return readBufferedImage(
        m_data, m_format, false,
        [&sourceRect](BufferedImageReader& reader) { reader.setClipRect(sourceRect); },
        errorString);
}
}
