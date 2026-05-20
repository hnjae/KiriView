// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rawdecoder.h"

#include "imagebytecost.h"
#include "imageerrortext.h"
#include "imageformatregistry.h"
#include "imagerendering.h"
#include "imagetilesourcehelpers_p.h"
#include "staticimagedecode.h"

#include <QColorSpace>
#include <cstddef>
#include <libraw/libraw.h>
#include <memory>
#include <optional>
#include <utility>

namespace {
using ProcessedRawImage
    = std::unique_ptr<libraw_processed_image_t, decltype(&LibRaw::dcraw_clear_mem)>;

enum class TiffByteOrder {
    LittleEndian,
    BigEndian,
};

constexpr quint16 tiffPhotometricInterpretationTag = 262;
constexpr quint16 tiffDngVersionTag = 50706;
constexpr quint16 tiffShortType = 3;
constexpr quint16 tiffCfaPhotometricInterpretation = 32803;

void setRawDecodeError(QString *errorString, QString message)
{
    if (errorString != nullptr) {
        *errorString = std::move(message);
    }
}

QString rawDecodeErrorString(const QString &action, int errorCode)
{
    QString message = KiriView::imageErrorText(KiriView::ImageErrorTextId::UnknownLibrawError);
    if (const char *rawMessage = LibRaw::strerror(errorCode); rawMessage != nullptr) {
        message = QString::fromUtf8(rawMessage);
    }

    return KiriView::rawDecodeErrorText(action, message);
}

bool byteRangeIsValid(const QByteArray &data, qsizetype offset, qsizetype size)
{
    return offset >= 0 && size >= 0 && offset <= data.size() && size <= data.size() - offset;
}

std::optional<TiffByteOrder> tiffByteOrder(const QByteArray &data)
{
    if (!byteRangeIsValid(data, 0, 4)) {
        return std::nullopt;
    }

    if (data[0] == 'I' && data[1] == 'I' && static_cast<unsigned char>(data[2]) == 42
        && static_cast<unsigned char>(data[3]) == 0) {
        return TiffByteOrder::LittleEndian;
    }
    if (data[0] == 'M' && data[1] == 'M' && static_cast<unsigned char>(data[2]) == 0
        && static_cast<unsigned char>(data[3]) == 42) {
        return TiffByteOrder::BigEndian;
    }
    return std::nullopt;
}

quint16 tiffUInt16(const QByteArray &data, qsizetype offset, TiffByteOrder byteOrder)
{
    const auto first = static_cast<quint16>(static_cast<unsigned char>(data[offset]));
    const auto second = static_cast<quint16>(static_cast<unsigned char>(data[offset + 1]));
    if (byteOrder == TiffByteOrder::LittleEndian) {
        return static_cast<quint16>(first | (second << 8));
    }
    return static_cast<quint16>((first << 8) | second);
}

quint32 tiffUInt32(const QByteArray &data, qsizetype offset, TiffByteOrder byteOrder)
{
    const auto first = static_cast<quint32>(static_cast<unsigned char>(data[offset]));
    const auto second = static_cast<quint32>(static_cast<unsigned char>(data[offset + 1]));
    const auto third = static_cast<quint32>(static_cast<unsigned char>(data[offset + 2]));
    const auto fourth = static_cast<quint32>(static_cast<unsigned char>(data[offset + 3]));
    if (byteOrder == TiffByteOrder::LittleEndian) {
        return first | (second << 8) | (third << 16) | (fourth << 24);
    }
    return (first << 24) | (second << 16) | (third << 8) | fourth;
}

std::optional<quint16> tiffShortValue(
    const QByteArray &data, qsizetype tagEntryOffset, quint32 valueCount, TiffByteOrder byteOrder)
{
    if (valueCount == 0) {
        return std::nullopt;
    }

    if (valueCount <= 2) {
        return tiffUInt16(data, tagEntryOffset + 8, byteOrder);
    }

    const quint32 valueOffset = tiffUInt32(data, tagEntryOffset + 8, byteOrder);
    if (!byteRangeIsValid(data, static_cast<qsizetype>(valueOffset), 2)) {
        return std::nullopt;
    }
    return tiffUInt16(data, static_cast<qsizetype>(valueOffset), byteOrder);
}

bool isLikelyTiffRawImageData(const QByteArray &data)
{
    const std::optional<TiffByteOrder> byteOrder = tiffByteOrder(data);
    if (!byteOrder.has_value() || !byteRangeIsValid(data, 4, 4)) {
        return false;
    }

    if (data.contains(QByteArray::fromHex("06010300010000002380"))
        || data.contains(QByteArray::fromHex("01060003000000018023"))
        || data.contains(QByteArray::fromHex("12c6010004000000"))
        || data.contains(QByteArray::fromHex("c612000100000004"))) {
        return true;
    }

    const qsizetype ifdOffset = static_cast<qsizetype>(tiffUInt32(data, 4, *byteOrder));
    if (!byteRangeIsValid(data, ifdOffset, 2)) {
        return false;
    }

    const quint16 tagCount = tiffUInt16(data, ifdOffset, *byteOrder);
    const qsizetype tagEntriesOffset = ifdOffset + 2;
    if (!byteRangeIsValid(data, tagEntriesOffset, static_cast<qsizetype>(tagCount) * 12)) {
        return false;
    }

    for (quint16 tagIndex = 0; tagIndex < tagCount; ++tagIndex) {
        const qsizetype tagEntryOffset = tagEntriesOffset + static_cast<qsizetype>(tagIndex) * 12;
        const quint16 tag = tiffUInt16(data, tagEntryOffset, *byteOrder);
        if (tag == tiffDngVersionTag) {
            return true;
        }

        if (tag != tiffPhotometricInterpretationTag) {
            continue;
        }

        const quint16 type = tiffUInt16(data, tagEntryOffset + 2, *byteOrder);
        const quint32 valueCount = tiffUInt32(data, tagEntryOffset + 4, *byteOrder);
        if (type != tiffShortType) {
            continue;
        }

        const std::optional<quint16> value
            = tiffShortValue(data, tagEntryOffset, valueCount, *byteOrder);
        if (value == tiffCfaPhotometricInterpretation) {
            return true;
        }
    }

    return false;
}

bool shouldAttemptRawDecode(const QByteArray &data, const KiriView::ImageDecodeRequest &request)
{
    return KiriView::isSupportedRawImageFileName(request.imageUrl().fileName())
        || isLikelyTiffRawImageData(data);
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
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedImageSizeInvalid));
        return false;
    }

    if (KiriView::estimatedRgbaByteCost(size) > KiriView::imageFullDecodeFallbackByteLimit) {
        setRawDecodeError(errorString,
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawFullDecodeTooLarge));
        return false;
    }

    return true;
}

std::optional<QImage> qImageFromRawProcessedImage(
    const libraw_processed_image_t *processedImage, QString *errorString)
{
    if (processedImage == nullptr) {
        setRawDecodeError(errorString,
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedImageInvalid));
        return std::nullopt;
    }

    if (processedImage->type != LIBRAW_IMAGE_BITMAP || processedImage->bits != 8
        || (processedImage->colors != 3 && processedImage->colors != 4)) {
        setRawDecodeError(errorString,
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedPixelFormatUnsupported));
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
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedPixelDataInvalid));
        return std::nullopt;
    }

    QImage image(imageSize, QImage::Format_RGBA8888);
    if (image.isNull()) {
        setRawDecodeError(errorString,
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedImageAllocationFailed));
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
            rawDecodeErrorString(
                KiriView::imageErrorActionText(KiriView::ImageErrorActionTextId::ReadRawImage),
                errorCode));
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
            rawDecodeErrorString(
                KiriView::imageErrorActionText(KiriView::ImageErrorActionTextId::UnpackRawImage),
                errorCode));
        return std::nullopt;
    }

    errorCode = processor.dcraw_process();
    if (errorCode != LIBRAW_SUCCESS) {
        setRawDecodeError(errorString,
            rawDecodeErrorString(
                KiriView::imageErrorActionText(KiriView::ImageErrorActionTextId::ProcessRawImage),
                errorCode));
        return std::nullopt;
    }

    int memImageErrorCode = LIBRAW_SUCCESS;
    ProcessedRawImage processedImage(
        processor.dcraw_make_mem_image(&memImageErrorCode), &LibRaw::dcraw_clear_mem);
    if (memImageErrorCode != LIBRAW_SUCCESS) {
        setRawDecodeError(errorString,
            rawDecodeErrorString(KiriView::imageErrorActionText(
                                     KiriView::ImageErrorActionTextId::CreateDisplayImage),
                memImageErrorCode));
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
            errorString, KiriView::imageErrorText(KiriView::ImageErrorTextId::RenderRawTile));
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
    if (!shouldAttemptRawDecode(data, request)) {
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
