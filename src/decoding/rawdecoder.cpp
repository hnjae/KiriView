// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rawdecoder.h"

#include "cache/imagebytecost.h"
#include "localization/imageerrortext.h"
#include "rendering/imagerendering.h"
#include "rendering/imagetilesourcehelpers_p.h"
#include "staticimagedecode.h"

#include <QColorSpace>
#include <QtGlobal>
#include <cstddef>
#include <libraw/libraw.h>
#include <memory>
#include <optional>
#include <utility>

namespace {
using ProcessedRawImage
    = std::unique_ptr<libraw_processed_image_t, decltype(&LibRaw::dcraw_clear_mem)>;

void setRawDecodeError(QString* errorString, QString message)
{
    if (errorString != nullptr) {
        *errorString = std::move(message);
    }
}

void setRawDecodeFailure(
    QString* errorString, QString* diagnosticDetail, QString message, QString detail)
{
    setRawDecodeError(errorString, std::move(message));
    setRawDecodeError(diagnosticDetail, std::move(detail));
}

QString rawBackendMessage(int errorCode)
{
    QString message = kiriview::imageErrorText(kiriview::ImageErrorTextId::UnknownLibrawError);
    if (const char* rawMessage = LibRaw::strerror(errorCode); rawMessage != nullptr) {
        message = QString::fromUtf8(rawMessage);
    }
    return message;
}

QString rawDecodeErrorString(const QString& action, int errorCode)
{
    const QString message = rawBackendMessage(errorCode);
    return kiriview::rawDecodeErrorText(action, message);
}

QString rawDecodeDiagnosticDetail(const QString& action, int errorCode)
{
    return QStringLiteral("LibRaw %1 failed with code %2: %3")
        .arg(action, QString::number(errorCode), rawBackendMessage(errorCode));
}

QString rawDecodeDiagnosticDetail(const QString& stage, const QString& message)
{
    return QStringLiteral("RAW %1 failed: %2").arg(stage, message);
}

QSize libRawImageSize(const LibRaw& processor)
{
    const libraw_image_sizes_t& sizes = processor.imgdata.sizes;
    if (sizes.iwidth > 0 && sizes.iheight > 0) {
        return QSize(sizes.iwidth, sizes.iheight);
    }
    return QSize(sizes.width, sizes.height);
}

bool validateRawImageSize(QSize size, QString* errorString, QString* diagnosticDetail)
{
    if (size.isEmpty()) {
        const QString message
            = kiriview::imageErrorText(kiriview::ImageErrorTextId::RawDecodedImageSizeInvalid);
        setRawDecodeFailure(errorString, diagnosticDetail, message,
            rawDecodeDiagnosticDetail(QStringLiteral("image size validation"), message));
        return false;
    }

    if (kiriview::estimatedRgbaByteCost(size) > kiriview::imageFullDecodeFallbackByteLimit) {
        const QString message
            = kiriview::imageErrorText(kiriview::ImageErrorTextId::RawFullDecodeTooLarge);
        setRawDecodeFailure(errorString, diagnosticDetail, message,
            rawDecodeDiagnosticDetail(QStringLiteral("full decode size guard"), message));
        return false;
    }

    return true;
}

std::optional<QImage> qImageFromRawProcessedImage(
    const libraw_processed_image_t* processedImage, QString* errorString, QString* diagnosticDetail)
{
    if (processedImage == nullptr) {
        const QString message
            = kiriview::imageErrorText(kiriview::ImageErrorTextId::RawDecodedImageInvalid);
        setRawDecodeFailure(errorString, diagnosticDetail, message,
            rawDecodeDiagnosticDetail(QStringLiteral("processed image validation"), message));
        return std::nullopt;
    }

    if (processedImage->type != LIBRAW_IMAGE_BITMAP || processedImage->bits != 8
        || (processedImage->colors != 3 && processedImage->colors != 4)) {
        const QString message = kiriview::imageErrorText(
            kiriview::ImageErrorTextId::RawDecodedPixelFormatUnsupported);
        setRawDecodeFailure(errorString, diagnosticDetail, message,
            rawDecodeDiagnosticDetail(
                QStringLiteral("processed pixel format validation"), message));
        return std::nullopt;
    }

    const QSize imageSize(processedImage->width, processedImage->height);
    if (!validateRawImageSize(imageSize, errorString, diagnosticDetail)) {
        return std::nullopt;
    }

    const std::size_t channelCount = processedImage->colors;
    const std::size_t minimumDataSize = static_cast<std::size_t>(processedImage->width)
        * static_cast<std::size_t>(processedImage->height) * channelCount;
    if (processedImage->data_size < minimumDataSize) {
        const QString message
            = kiriview::imageErrorText(kiriview::ImageErrorTextId::RawDecodedPixelDataInvalid);
        setRawDecodeFailure(errorString, diagnosticDetail, message,
            rawDecodeDiagnosticDetail(QStringLiteral("processed pixel data validation"), message));
        return std::nullopt;
    }

    QImage image(imageSize, QImage::Format_RGBA8888);
    if (image.isNull()) {
        const QString message
            = kiriview::imageErrorText(kiriview::ImageErrorTextId::RawDecodedImageAllocationFailed);
        setRawDecodeFailure(errorString, diagnosticDetail, message,
            rawDecodeDiagnosticDetail(QStringLiteral("display image allocation"), message));
        return std::nullopt;
    }

    const unsigned char* source = processedImage->data;
    for (int y = 0; y < image.height(); ++y) {
        unsigned char* target = image.scanLine(y);
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

    return kiriview::displayReadyImage(image);
}

std::optional<QImage> decodeRawImage(
    const QByteArray& data, QString* errorString, QString* diagnosticDetail)
{
    LibRaw processor;
    int errorCode = processor.open_buffer(data.constData(), static_cast<std::size_t>(data.size()));
    if (errorCode != LIBRAW_SUCCESS) {
        const QString action
            = kiriview::imageErrorActionText(kiriview::ImageErrorActionTextId::ReadRawImage);
        setRawDecodeFailure(errorString, diagnosticDetail, rawDecodeErrorString(action, errorCode),
            rawDecodeDiagnosticDetail(action, errorCode));
        return std::nullopt;
    }

    if (!validateRawImageSize(libRawImageSize(processor), errorString, diagnosticDetail)) {
        return std::nullopt;
    }

    processor.imgdata.params.use_camera_wb = 1;
    processor.imgdata.params.output_color = 1;
    processor.imgdata.params.output_bps = 8;

    errorCode = processor.unpack();
    if (errorCode != LIBRAW_SUCCESS) {
        const QString action
            = kiriview::imageErrorActionText(kiriview::ImageErrorActionTextId::UnpackRawImage);
        setRawDecodeFailure(errorString, diagnosticDetail, rawDecodeErrorString(action, errorCode),
            rawDecodeDiagnosticDetail(action, errorCode));
        return std::nullopt;
    }

    errorCode = processor.dcraw_process();
    if (errorCode != LIBRAW_SUCCESS) {
        const QString action
            = kiriview::imageErrorActionText(kiriview::ImageErrorActionTextId::ProcessRawImage);
        setRawDecodeFailure(errorString, diagnosticDetail, rawDecodeErrorString(action, errorCode),
            rawDecodeDiagnosticDetail(action, errorCode));
        return std::nullopt;
    }

    int memImageErrorCode = LIBRAW_SUCCESS;
    ProcessedRawImage processedImage(
        processor.dcraw_make_mem_image(&memImageErrorCode), &LibRaw::dcraw_clear_mem);
    if (memImageErrorCode != LIBRAW_SUCCESS) {
        const QString action
            = kiriview::imageErrorActionText(kiriview::ImageErrorActionTextId::CreateDisplayImage);
        setRawDecodeFailure(errorString, diagnosticDetail,
            rawDecodeErrorString(action, memImageErrorCode),
            rawDecodeDiagnosticDetail(action, memImageErrorCode));
        return std::nullopt;
    }

    return qImageFromRawProcessedImage(processedImage.get(), errorString, diagnosticDetail);
}

class RawImageTileSource final : public kiriview::ImageTileSource
{
public:
    explicit RawImageTileSource(QImage image)
        : m_image(std::move(image))
    {
    }

    QSize imageSize() const override { return m_image.size(); }
    qsizetype byteCost() const override { return kiriview::imageByteCost(m_image); }
    bool supportsRasterDisplayRefinement() const override { return true; }

    QImage decodeRasterDisplayImage(const QSize& rasterSize, QString*) const override
    {
        if (rasterSize.isEmpty()) {
            return {};
        }

        return kiriview::scaledTileImage(m_image, rasterSize);
    }

    QImage decodeBlockingDisplayImage(int maximumLongEdge, QString*) const override
    {
        return kiriview::scaledTileImage(
            m_image, kiriview::boundedPreviewSize(m_image.size(), maximumLongEdge));
    }

    std::optional<kiriview::DecodedTile> decodeTile(
        const kiriview::TileRequest& request, QString* errorString) const override
    {
        if (!kiriview::tileRequestCanDecode(request)) {
            return std::nullopt;
        }

        QImage levelImage = kiriview::scaledTileImage(m_image, request.levelSize);
        if (std::optional<kiriview::DecodedTile> tile
            = kiriview::decodedTileFromLevelImage(request, levelImage)) {
            return tile;
        }

        kiriview::setTileSourceError(
            errorString, kiriview::imageErrorText(kiriview::ImageErrorTextId::RenderRawTile));
        return std::nullopt;
    }

private:
    QImage m_image;
    Q_DISABLE_COPY(RawImageTileSource)
};
}

namespace kiriview {
DecodedImageResult decodeRawImageData(const QByteArray& data, const ImageDecodeRequest& request)
{
    QString errorString;
    QString diagnosticDetail;
    std::optional<QImage> image = decodeRawImage(data, &errorString, &diagnosticDetail);
    if (!image.has_value()) {
        return failedDecodedImageResult(DecodedImageFailure {
            std::move(errorString),
            DecodedImageFailureRoute::Raw,
            DecodedImageFailureOperation::DecodeRawImage,
            std::move(diagnosticDetail),
            DecodedImageFailureSeverity::Error,
            false,
        });
    }

    std::shared_ptr<ImageTileSource> source
        = std::make_shared<RawImageTileSource>(std::move(*image));
    return staticDecodedImageResult(std::move(source), request, &errorString);
}
}
