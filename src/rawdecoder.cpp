// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rawdecoder.h"

#include "imagebytecost.h"
#include "imageformatregistry.h"
#include "imagerendering.h"
#include "imagetilesource_p.h"
#include "imageviewtext.h"
#include "staticimagedecode.h"

#include <KLocalizedString>
#include <QColorSpace>
#include <cstddef>
#include <libraw/libraw.h>
#include <memory>
#include <optional>
#include <utility>

namespace {
using ProcessedRawImage
    = std::unique_ptr<libraw_processed_image_t, decltype(&LibRaw::dcraw_clear_mem)>;

void setRawDecodeError(QString *errorString, QString message)
{
    if (errorString != nullptr) {
        *errorString = std::move(message);
    }
}

QString rawDecodeErrorString(const QString &action, int errorCode)
{
    QString message = KiriView::imageViewText("Unknown LibRaw error.");
    if (const char *rawMessage = LibRaw::strerror(errorCode); rawMessage != nullptr) {
        message = QString::fromUtf8(rawMessage);
    }

    return ki18nc("@info:status", "Could not decode the selected RAW image: %1: %2")
        .subs(action)
        .subs(message)
        .toString();
}

QSize libRawImageSize(const LibRaw &processor)
{
    const libraw_image_sizes_t &sizes = processor.imgdata.sizes;
    if (sizes.iwidth > 0 && sizes.iheight > 0) {
        return QSize(sizes.iwidth, sizes.iheight);
    }
    return QSize(sizes.width, sizes.height);
}

bool validateRawImageSize(const QSize &size, QString *errorString)
{
    if (size.isEmpty()) {
        setRawDecodeError(errorString,
            KiriView::imageViewText(
                "Could not decode the selected RAW image: decoded image size is invalid."));
        return false;
    }

    if (KiriView::estimatedRgbaByteCost(size) > KiriView::imageFullDecodeFallbackByteLimit) {
        setRawDecodeError(errorString,
            KiriView::imageViewText(
                "The selected RAW image is too large for full-image decoding."));
        return false;
    }

    return true;
}

std::optional<QImage> qImageFromRawProcessedImage(
    const libraw_processed_image_t *processedImage, QString *errorString)
{
    if (processedImage == nullptr) {
        setRawDecodeError(errorString,
            KiriView::imageViewText(
                "Could not decode the selected RAW image: decoded image is invalid."));
        return std::nullopt;
    }

    if (processedImage->type != LIBRAW_IMAGE_BITMAP || processedImage->bits != 8
        || (processedImage->colors != 3 && processedImage->colors != 4)) {
        setRawDecodeError(errorString,
            KiriView::imageViewText(
                "Could not decode the selected RAW image: decoded pixel format is unsupported."));
        return std::nullopt;
    }

    const QSize imageSize(processedImage->width, processedImage->height);
    if (!validateRawImageSize(imageSize, errorString)) {
        return std::nullopt;
    }

    const std::size_t channelCount = processedImage->colors;
    const std::size_t minimumDataSize = static_cast<std::size_t>(processedImage->width)
        * static_cast<std::size_t>(processedImage->height) * channelCount;
    if (processedImage->data_size < minimumDataSize) {
        setRawDecodeError(errorString,
            KiriView::imageViewText(
                "Could not decode the selected RAW image: decoded pixel data is invalid."));
        return std::nullopt;
    }

    QImage image(imageSize, QImage::Format_RGBA8888);
    if (image.isNull()) {
        setRawDecodeError(errorString,
            KiriView::imageViewText(
                "Could not decode the selected RAW image: decoded image allocation failed."));
        return std::nullopt;
    }

    const unsigned char *source = processedImage->data;
    for (int y = 0; y < image.height(); ++y) {
        unsigned char *target = image.scanLine(y);
        for (int x = 0; x < image.width(); ++x) {
            const std::size_t sourceOffset
                = (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width())
                      + static_cast<std::size_t>(x))
                * channelCount;
            const int targetOffset = x * 4;
            target[targetOffset] = source[sourceOffset];
            target[targetOffset + 1] = source[sourceOffset + 1];
            target[targetOffset + 2] = source[sourceOffset + 2];
            target[targetOffset + 3] = channelCount == 4 ? source[sourceOffset + 3] : 255;
        }
    }
    image.setColorSpace(QColorSpace(QColorSpace::SRgb));

    return KiriView::displayReadyImage(image);
}

std::optional<QImage> decodeRawImage(const QByteArray &data, QString *errorString)
{
    LibRaw processor;
    int errorCode = processor.open_buffer(data.constData(), static_cast<std::size_t>(data.size()));
    if (errorCode != LIBRAW_SUCCESS) {
        setRawDecodeError(errorString,
            rawDecodeErrorString(KiriView::imageViewText("reading the RAW image"), errorCode));
        return std::nullopt;
    }

    if (!validateRawImageSize(libRawImageSize(processor), errorString)) {
        return std::nullopt;
    }

    processor.imgdata.params.use_camera_wb = 1;
    processor.imgdata.params.output_color = 1;
    processor.imgdata.params.output_bps = 8;

    errorCode = processor.unpack();
    if (errorCode != LIBRAW_SUCCESS) {
        setRawDecodeError(errorString,
            rawDecodeErrorString(KiriView::imageViewText("unpacking the RAW image"), errorCode));
        return std::nullopt;
    }

    errorCode = processor.dcraw_process();
    if (errorCode != LIBRAW_SUCCESS) {
        setRawDecodeError(errorString,
            rawDecodeErrorString(KiriView::imageViewText("processing the RAW image"), errorCode));
        return std::nullopt;
    }

    int memImageErrorCode = LIBRAW_SUCCESS;
    ProcessedRawImage processedImage(
        processor.dcraw_make_mem_image(&memImageErrorCode), &LibRaw::dcraw_clear_mem);
    if (memImageErrorCode != LIBRAW_SUCCESS) {
        setRawDecodeError(errorString,
            rawDecodeErrorString(
                KiriView::imageViewText("creating the display image"), memImageErrorCode));
        return std::nullopt;
    }

    return qImageFromRawProcessedImage(processedImage.get(), errorString);
}

class RawImageTileSource final : public KiriView::ImageTileSource
{
public:
    explicit RawImageTileSource(QImage image)
        : m_image(std::move(image))
    {
    }

    QSize imageSize() const override { return m_image.size(); }
    qsizetype byteCost() const override { return KiriView::imageByteCost(m_image); }

    QImage decodeBlockingDisplayImage(int maximumLongEdge, QString *) const override
    {
        return KiriView::scaledTileImage(
            m_image, KiriView::boundedPreviewSize(m_image.size(), maximumLongEdge));
    }

    std::optional<KiriView::DecodedTile> decodeTile(
        const KiriView::TileRequest &request, QString *errorString) const override
    {
        if (!KiriView::tileRequestCanDecode(request)) {
            return std::nullopt;
        }

        QImage levelImage = KiriView::scaledTileImage(m_image, request.levelSize);
        if (std::optional<KiriView::DecodedTile> tile
            = KiriView::decodedTileFromLevelImage(request, levelImage)) {
            return tile;
        }

        KiriView::setTileSourceError(
            errorString, KiriView::imageViewText("Could not render the selected RAW tile."));
        return std::nullopt;
    }

private:
    QImage m_image;
};
}

namespace KiriView {
std::optional<DecodedImageResult> decodeRawImageData(
    const QByteArray &data, const ImageDecodeRequest &request)
{
    if (!isSupportedRawImageFileName(request.imageUrl().fileName())) {
        return std::nullopt;
    }

    QString errorString;
    std::optional<QImage> image = decodeRawImage(data, &errorString);
    if (!image.has_value()) {
        return failedDecodedImageResult(std::move(errorString));
    }

    std::shared_ptr<ImageTileSource> source
        = std::make_shared<RawImageTileSource>(std::move(*image));
    return staticDecodedImageResult(std::move(source), request.firstDisplay(), &errorString);
}
}
