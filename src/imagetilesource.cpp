// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilesource.h"

#include "imagerendering.h"
#include "imageviewtext.h"

#include <libheif/heif.h>
#include <libheif/heif_tiling.h>

#include <QBuffer>
#include <QColorSpace>
#include <QIODevice>
#include <QImageIOHandler>
#include <QImageReader>
#include <QMutexLocker>
#include <QPainter>
#include <QRectF>
#include <QSvgRenderer>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <limits>
#include <mutex>
#include <optional>
#include <utility>

namespace {
QSize boundedPreviewSize(const QSize &imageSize, int maximumLongEdge)
{
    if (imageSize.isEmpty() || maximumLongEdge <= 0) {
        return {};
    }

    const int longEdge = std::max(imageSize.width(), imageSize.height());
    if (longEdge <= maximumLongEdge) {
        return imageSize;
    }

    const qreal scale = static_cast<qreal>(maximumLongEdge) / longEdge;
    return QSize(std::max(1, static_cast<int>(std::ceil(imageSize.width() * scale))),
        std::max(1, static_cast<int>(std::ceil(imageSize.height() * scale))));
}

qsizetype estimatedRgbaByteCost(const QSize &size)
{
    if (size.isEmpty()) {
        return 0;
    }
    constexpr qsizetype bytesPerPixel = 4;
    const qsizetype width = static_cast<qsizetype>(size.width());
    const qsizetype height = static_cast<qsizetype>(size.height());
    if (width > std::numeric_limits<qsizetype>::max() / height
        || width * height > std::numeric_limits<qsizetype>::max() / bytesPerPixel) {
        return std::numeric_limits<qsizetype>::max();
    }
    return width * height * bytesPerPixel;
}

QImage scaledTileImage(const QImage &image, const QSize &size)
{
    if (image.isNull() || size.isEmpty()) {
        return {};
    }
    if (image.size() == size) {
        return KiriView::displayReadyImage(image);
    }
    return KiriView::displayReadyImage(
        image.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

QImage cropLevelTexture(const QImage &levelImage, const QRect &textureLevelRect)
{
    if (levelImage.isNull() || textureLevelRect.isEmpty()) {
        return {};
    }
    return KiriView::displayReadyImage(levelImage.copy(textureLevelRect));
}

KiriView::DecodedTile decodedTileFromImage(const KiriView::TileRequest &request, QImage image)
{
    return KiriView::DecodedTile {
        request.key,
        request.levelSize,
        request.levelRect,
        request.textureLevelRect,
        KiriView::displayReadyImage(image),
    };
}

void setError(QString *errorString, const QString &message)
{
    if (errorString != nullptr) {
        *errorString = message;
    }
}

QSize svgIntrinsicSize(const QSvgRenderer &renderer)
{
    const QSize defaultSize = renderer.defaultSize();
    if (!defaultSize.isEmpty()) {
        return defaultSize;
    }

    const QRectF viewBox = renderer.viewBoxF();
    if (!viewBox.isValid()) {
        return {};
    }

    return QSize(std::max(1, static_cast<int>(std::ceil(viewBox.width()))),
        std::max(1, static_cast<int>(std::ceil(viewBox.height()))));
}

QString heifErrorString(const char *action, const heif_error &error)
{
    const QString message = error.message != nullptr
        ? QString::fromUtf8(error.message)
        : KiriView::imageViewText("Unknown libheif error.");
    return KiriView::imageViewText("Could not decode the selected HEIF image: ")
        + QString::fromUtf8(action) + QStringLiteral(": ") + message;
}

std::optional<QString> initializeHeifLibrary()
{
    static std::once_flag initFlag;
    static heif_error initError {};
    std::call_once(initFlag, [] { initError = heif_init(nullptr); });

    if (initError.code != heif_error_Ok) {
        return heifErrorString("initializing libheif", initError);
    }
    return std::nullopt;
}

class HeifContext
{
public:
    HeifContext()
        : m_context(heif_context_alloc())
    {
    }

    ~HeifContext()
    {
        if (m_context != nullptr) {
            heif_context_free(m_context);
        }
    }

    HeifContext(const HeifContext &) = delete;
    HeifContext &operator=(const HeifContext &) = delete;
    HeifContext(HeifContext &&other) noexcept
        : m_context(std::exchange(other.m_context, nullptr))
    {
    }
    HeifContext &operator=(HeifContext &&other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        if (m_context != nullptr) {
            heif_context_free(m_context);
        }
        m_context = std::exchange(other.m_context, nullptr);
        return *this;
    }

    heif_context *get() const { return m_context; }

private:
    heif_context *m_context = nullptr;
};

class HeifImageHandle
{
public:
    HeifImageHandle() = default;

    ~HeifImageHandle()
    {
        if (m_handle != nullptr) {
            heif_image_handle_release(m_handle);
        }
    }

    HeifImageHandle(const HeifImageHandle &) = delete;
    HeifImageHandle &operator=(const HeifImageHandle &) = delete;
    HeifImageHandle(HeifImageHandle &&other) noexcept
        : m_handle(std::exchange(other.m_handle, nullptr))
    {
    }
    HeifImageHandle &operator=(HeifImageHandle &&other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        if (m_handle != nullptr) {
            heif_image_handle_release(m_handle);
        }
        m_handle = std::exchange(other.m_handle, nullptr);
        return *this;
    }

    heif_image_handle **out() { return &m_handle; }
    const heif_image_handle *get() const { return m_handle; }

private:
    heif_image_handle *m_handle = nullptr;
};

class HeifImage
{
public:
    HeifImage() = default;

    ~HeifImage()
    {
        if (m_image != nullptr) {
            heif_image_release(m_image);
        }
    }

    HeifImage(const HeifImage &) = delete;
    HeifImage &operator=(const HeifImage &) = delete;

    heif_image **out() { return &m_image; }
    const heif_image *get() const { return m_image; }

private:
    heif_image *m_image = nullptr;
};

class HeifDecodingOptions
{
public:
    HeifDecodingOptions()
        : m_options(heif_decoding_options_alloc())
    {
        if (m_options != nullptr) {
            m_options->convert_hdr_to_8bit = 1;
        }
    }

    ~HeifDecodingOptions()
    {
        if (m_options != nullptr) {
            heif_decoding_options_free(m_options);
        }
    }

    HeifDecodingOptions(const HeifDecodingOptions &) = delete;
    HeifDecodingOptions &operator=(const HeifDecodingOptions &) = delete;

    const heif_decoding_options *get() const { return m_options; }

private:
    heif_decoding_options *m_options = nullptr;
};

std::optional<QImage> qImageFromHeifImage(const heif_image *heifImage, QString *errorString)
{
    if (heifImage == nullptr) {
        setError(errorString,
            KiriView::imageViewText(
                "Could not decode the selected HEIF image: decoded image is invalid."));
        return std::nullopt;
    }

    const int imageWidth = heif_image_get_width(heifImage, heif_channel_interleaved);
    const int imageHeight = heif_image_get_height(heifImage, heif_channel_interleaved);
    if (imageWidth <= 0 || imageHeight <= 0) {
        setError(errorString,
            KiriView::imageViewText(
                "Could not decode the selected HEIF image: decoded image size is invalid."));
        return std::nullopt;
    }

    size_t sourceStride = 0;
    const uint8_t *source
        = heif_image_get_plane_readonly2(heifImage, heif_channel_interleaved, &sourceStride);
    constexpr size_t bytesPerPixel = 4;
    const size_t rowBytes = static_cast<size_t>(imageWidth) * bytesPerPixel;
    if (source == nullptr || sourceStride < rowBytes) {
        setError(errorString,
            KiriView::imageViewText(
                "Could not decode the selected HEIF image: decoded pixel data is invalid."));
        return std::nullopt;
    }

    QImage image(imageWidth, imageHeight, QImage::Format_RGBA8888);
    if (image.isNull()) {
        setError(errorString,
            KiriView::imageViewText(
                "Could not decode the selected HEIF image: decoded image allocation failed."));
        return std::nullopt;
    }

    for (int y = 0; y < imageHeight; ++y) {
        std::memcpy(image.scanLine(y), source + (static_cast<size_t>(y) * sourceStride), rowBytes);
    }
    image.setColorSpace(QColorSpace(QColorSpace::SRgb));
    return KiriView::displayReadyImage(image);
}

quint32 readBigEndianUint32(const char *data)
{
    return (static_cast<quint32>(static_cast<unsigned char>(data[0])) << 24)
        | (static_cast<quint32>(static_cast<unsigned char>(data[1])) << 16)
        | (static_cast<quint32>(static_cast<unsigned char>(data[2])) << 8)
        | static_cast<quint32>(static_cast<unsigned char>(data[3]));
}

bool isHeifBrand(const char *brand)
{
    static constexpr const char *brands[] = {
        "avci",
        "avcs",
        "avif",
        "avis",
        "heic",
        "heim",
        "heis",
        "heix",
        "hevc",
        "hevm",
        "hevs",
        "hevx",
        "j2is",
        "j2ki",
        "jpgs",
        "jpeg",
        "mif1",
        "mif2",
        "msf1",
        "vvic",
        "vvis",
    };

    return std::any_of(std::begin(brands), std::end(brands),
        [brand](const char *candidate) { return std::memcmp(brand, candidate, 4) == 0; });
}

bool isLikelyHeifContainer(const QByteArray &data)
{
    if (data.size() < 16 || std::memcmp(data.constData() + 4, "ftyp", 4) != 0) {
        return false;
    }

    const quint32 boxSize = readBigEndianUint32(data.constData());
    if (boxSize < 16 || boxSize > static_cast<quint32>(data.size())) {
        return false;
    }

    if (isHeifBrand(data.constData() + 8)) {
        return true;
    }

    for (quint32 offset = 16; offset + 4 <= boxSize; offset += 4) {
        if (isHeifBrand(data.constData() + offset)) {
            return true;
        }
    }

    return false;
}

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

    QImage decodePreview(int maximumLongEdge, QString *errorString) const override
    {
        return decodeFullOrScaled(boundedPreviewSize(m_imageSize, maximumLongEdge), errorString);
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
            sourceImage = cropLevelTexture(sourceImage, request.textureLevelRect);
            if (!sourceImage.isNull()) {
                return decodedTileFromImage(request, sourceImage);
            }
        }

        if (sourceImage.isNull()) {
            return std::nullopt;
        }

        QImage image = scaledTileImage(sourceImage, request.textureLevelRect.size());
        if (image.isNull()) {
            setError(errorString, KiriView::imageViewText("Could not render the selected tile."));
            return std::nullopt;
        }
        return decodedTileFromImage(request, std::move(image));
    }

private:
    std::optional<std::pair<HeifContext, HeifImageHandle>> openHandle(QString *errorString) const
    {
        if (std::optional<QString> initError = initializeHeifLibrary()) {
            setError(errorString, *initError);
            return std::nullopt;
        }

        HeifContext context;
        if (context.get() == nullptr) {
            setError(errorString,
                KiriView::imageViewText("Could not decode the selected HEIF image: libheif could "
                                        "not allocate a context."));
            return std::nullopt;
        }

        heif_error error = heif_context_read_from_memory_without_copy(
            context.get(), m_data.constData(), static_cast<size_t>(m_data.size()), nullptr);
        if (error.code != heif_error_Ok) {
            setError(errorString, heifErrorString("reading the HEIF container", error));
            return std::nullopt;
        }

        HeifImageHandle handle;
        error = heif_context_get_primary_image_handle(context.get(), handle.out());
        if (error.code != heif_error_Ok) {
            setError(errorString, heifErrorString("reading the primary image", error));
            return std::nullopt;
        }

        return std::make_pair(std::move(context), std::move(handle));
    }

    QImage decodeFullOrScaled(const QSize &targetSize, QString *errorString) const
    {
        if (estimatedRgbaByteCost(m_imageSize) > KiriView::imageFullDecodeFallbackByteLimit) {
            setError(errorString,
                KiriView::imageViewText("The selected HEIF image is too large for fallback "
                                        "full-image decoding."));
            return {};
        }

        std::optional<std::pair<HeifContext, HeifImageHandle>> opened = openHandle(errorString);
        if (!opened.has_value()) {
            return {};
        }

        HeifDecodingOptions options;
        HeifImage heifImage;
        heif_error error = heif_decode_image(opened->second.get(), heifImage.out(),
            heif_colorspace_RGB, heif_chroma_interleaved_RGBA, options.get());
        if (error.code != heif_error_Ok) {
            setError(errorString, heifErrorString("decoding the primary image", error));
            return {};
        }

        std::optional<QImage> image = qImageFromHeifImage(heifImage.get(), errorString);
        if (!image.has_value()) {
            return {};
        }

        return scaledTileImage(*image, targetSize.isEmpty() ? m_imageSize : targetSize);
    }

    QImage decodeGridSourceRect(const QRect &sourceRect, QString *errorString) const
    {
        std::optional<std::pair<HeifContext, HeifImageHandle>> opened = openHandle(errorString);
        if (!opened.has_value()) {
            return {};
        }

        QImage image(sourceRect.size(), QImage::Format_RGBA8888_Premultiplied);
        if (image.isNull()) {
            setError(errorString, KiriView::imageViewText("Could not allocate the selected tile."));
            return {};
        }
        image.fill(Qt::transparent);

        HeifDecodingOptions options;
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
                HeifImage heifImage;
                const heif_error error = heif_image_handle_decode_image_tile(opened->second.get(),
                    heifImage.out(), heif_colorspace_RGB, heif_chroma_interleaved_RGBA,
                    options.get(), static_cast<uint32_t>(tileX), static_cast<uint32_t>(tileY));
                if (error.code != heif_error_Ok) {
                    setError(errorString, heifErrorString("decoding a HEIF grid tile", error));
                    return {};
                }

                std::optional<QImage> tileImage = qImageFromHeifImage(heifImage.get(), errorString);
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
std::shared_ptr<QImageReaderTileSource> QImageReaderTileSource::open(
    const QByteArray &data, QString *errorString)
{
    QBuffer buffer;
    buffer.setData(data);
    if (!buffer.open(QIODevice::ReadOnly)) {
        setError(errorString, imageViewText("Could not read the selected image data."));
        return {};
    }

    QImageReader reader(&buffer);
    reader.setAutoTransform(true);
    const QSize imageSize = reader.size();
    const QByteArray format = reader.format();
    if (imageSize.isEmpty()) {
        setError(errorString, reader.errorString());
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
        setError(errorString, imageViewText("Could not read the selected image data."));
        return {};
    }

    QImageReader reader(&buffer, m_format);
    reader.setAutoTransform(true);
    if (!scaledSize.isEmpty()) {
        reader.setScaledSize(scaledSize);
    }

    QImage image = reader.read();
    if (image.isNull()) {
        setError(errorString, reader.errorString());
        return {};
    }
    return displayReadyImage(image);
}

QImage QImageReaderTileSource::readFullImage(QString *errorString) const
{
    if (estimatedRgbaByteCost(m_imageSize) > imageFullDecodeFallbackByteLimit) {
        setError(errorString,
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
        setError(errorString, imageViewText("Could not read the selected image data."));
        return {};
    }

    QImageReader reader(&buffer, m_format);
    reader.setAutoTransform(false);
    reader.setClipRect(sourceRect);
    QImage image = reader.read();
    if (image.isNull()) {
        setError(errorString, reader.errorString());
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
        setError(errorString, imageViewText("Could not determine the selected SVG image size."));
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
        setError(errorString, imageViewText("Could not decode the selected SVG image."));
        return std::nullopt;
    }

    QImage image(request.textureLevelRect.size(), QImage::Format_RGBA8888_Premultiplied);
    if (image.isNull()) {
        setError(errorString, imageViewText("Could not render the selected SVG tile."));
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
        setError(errorString, imageViewText("Could not render the selected SVG image."));
        return {};
    }
    return preview;
}

qsizetype SvgTileSource::byteCost() const { return m_data.size(); }

std::shared_ptr<ImageTileSource> openHeifTileSource(const QByteArray &data, QString *errorString)
{
    if (!isLikelyHeifContainer(data)) {
        return {};
    }
    if (std::optional<QString> initError = initializeHeifLibrary()) {
        setError(errorString, *initError);
        return {};
    }

    HeifContext context;
    if (context.get() == nullptr) {
        setError(errorString,
            imageViewText(
                "Could not decode the selected HEIF image: libheif could not allocate a context."));
        return {};
    }

    heif_error error = heif_context_read_from_memory_without_copy(
        context.get(), data.constData(), static_cast<size_t>(data.size()), nullptr);
    if (error.code != heif_error_Ok) {
        setError(errorString, heifErrorString("reading the HEIF container", error));
        return {};
    }

    HeifImageHandle handle;
    error = heif_context_get_primary_image_handle(context.get(), handle.out());
    if (error.code != heif_error_Ok) {
        setError(errorString, heifErrorString("reading the primary image", error));
        return {};
    }

    const QSize imageSize(
        heif_image_handle_get_width(handle.get()), heif_image_handle_get_height(handle.get()));
    if (imageSize.isEmpty()) {
        setError(errorString,
            imageViewText("Could not decode the selected HEIF image: image size is invalid."));
        return {};
    }

    heif_image_tiling tiling {};
    tiling.version = 1;
    error = heif_image_handle_get_image_tiling(handle.get(), 1, &tiling);
    const bool tiled = error.code == heif_error_Ok && tiling.num_columns > 0 && tiling.num_rows > 0
        && tiling.tile_width > 0 && tiling.tile_height > 0
        && (tiling.num_columns > 1 || tiling.num_rows > 1);

    return std::make_shared<HeifTileSource>(data, imageSize, tiling, tiled);
}
}
