// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rawthumbnailpreview.h"

#include "bufferedimagereader.h"
#include "cache/imagebytecost.h"
#include "imageinputclassification.h"
#include "localization/imageerrortext.h"
#include "location/sourcekey.h"
#include "rendering/imagerendering.h"

#include <QColorSpace>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <libraw/libraw.h>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

namespace {
using ProcessedRawImage
    = std::unique_ptr<libraw_processed_image_t, decltype(&LibRaw::dcraw_clear_mem)>;

bool validImageSize(const QSize &size)
{
    return size.isValid() && !size.isEmpty() && size.width() > 0 && size.height() > 0;
}

void setError(QString *errorString, QString message)
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

KiriView::RawEmbeddedThumbnailPreviewResult rawResult(
    KiriView::RawEmbeddedThumbnailPreviewStatus status, QImage image = {}, QSize originalSize = {},
    QString errorString = {})
{
    return KiriView::RawEmbeddedThumbnailPreviewResult {
        status,
        std::move(image),
        originalSize,
        std::move(errorString),
    };
}

bool rawThumbnailMissingError(int errorCode)
{
    return errorCode == LIBRAW_NO_THUMBNAIL || errorCode == LIBRAW_UNSUPPORTED_THUMBNAIL
        || errorCode == LIBRAW_REQUEST_FOR_NONEXISTENT_IMAGE;
}

bool rawInputIsEligibleForEmbeddedThumbnail(
    const QByteArray &data, const KiriView::ImageDecodeRequest &request)
{
    const KiriView::ImageInputClassification classification
        = KiriView::classifyImageInput(data, request.imageUrl().fileName());
    return classification.kind == KiriView::ImageInputKind::Raw;
}

QSize libRawImageSize(const LibRaw &processor)
{
    const libraw_image_sizes_t &sizes = processor.imgdata.sizes;
    if (sizes.iwidth > 0 && sizes.iheight > 0) {
        return QSize(sizes.iwidth, sizes.iheight);
    }
    return QSize(sizes.width, sizes.height);
}

bool rawPreviewByteCostFitsBudget(const QSize &size)
{
    const qsizetype byteCost = KiriView::estimatedRgbaByteCost(size);
    return byteCost > 0 && byteCost <= KiriView::imageFullDecodeFallbackByteLimit;
}

bool validateTrustedOriginalSize(const QSize &size)
{
    return validImageSize(size) && rawPreviewByteCostFitsBudget(size);
}

std::optional<QSize> trustedOriginalSizeFromRawData(const QByteArray &data, QString *errorString)
{
    LibRaw processor;
    const int errorCode
        = processor.open_buffer(data.constData(), static_cast<std::size_t>(data.size()));
    if (errorCode != LIBRAW_SUCCESS) {
        setError(errorString,
            rawDecodeErrorString(
                KiriView::imageErrorActionText(KiriView::ImageErrorActionTextId::ReadRawImage),
                errorCode));
        return std::nullopt;
    }

    const QSize originalSize = libRawImageSize(processor);
    if (!validateTrustedOriginalSize(originalSize)) {
        setError(errorString,
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedImageSizeInvalid));
        return std::nullopt;
    }
    return originalSize;
}

bool thumbnailFitsOriginal(const QSize &thumbnailSize, const QSize &originalSize)
{
    return thumbnailSize.width() <= originalSize.width()
        && thumbnailSize.height() <= originalSize.height();
}

bool aspectCompatible(const QSize &thumbnailSize, const QSize &originalSize)
{
    if (!validImageSize(thumbnailSize) || !validImageSize(originalSize)) {
        return false;
    }

    const long double left
        = static_cast<long double>(thumbnailSize.width()) * originalSize.height();
    const long double right
        = static_cast<long double>(thumbnailSize.height()) * originalSize.width();
    const long double maximum = std::max(std::fabsl(left), std::fabsl(right));
    if (maximum <= 0.0L) {
        return false;
    }

    return std::fabsl(left - right) / maximum <= 0.01L;
}

bool validateRawPreviewImage(const QImage &image, const QSize &originalSize, QString *errorString)
{
    if (image.isNull() || !validImageSize(image.size())) {
        setError(errorString,
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedImageInvalid));
        return false;
    }
    if (!validateTrustedOriginalSize(originalSize)) {
        setError(errorString,
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedImageSizeInvalid));
        return false;
    }
    if (!thumbnailFitsOriginal(image.size(), originalSize)) {
        setError(errorString, QStringLiteral("RAW embedded thumbnail is larger than source image"));
        return false;
    }
    if (!aspectCompatible(image.size(), originalSize)) {
        setError(errorString,
            QStringLiteral("RAW embedded thumbnail aspect ratio does not match source image"));
        return false;
    }
    const qsizetype byteCost = KiriView::imageByteCost(image);
    if (byteCost <= 0 || byteCost > KiriView::imageFullDecodeFallbackByteLimit) {
        setError(errorString,
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawFullDecodeTooLarge));
        return false;
    }

    return true;
}

std::optional<QImage> jpegThumbnailImage(
    const libraw_processed_image_t *processedImage, QString *errorString)
{
    if (processedImage->data_size <= 0
        || processedImage->data_size
            > static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
        setError(errorString,
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedPixelDataInvalid));
        return std::nullopt;
    }

    const QByteArray jpegData(reinterpret_cast<const char *>(processedImage->data),
        static_cast<qsizetype>(processedImage->data_size));
    KiriView::BufferedImageReader reader(jpegData, QByteArrayLiteral("jpeg"));
    if (!reader.canRead()) {
        setError(errorString,
            reader.errorString().isEmpty()
                ? KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedImageInvalid)
                : reader.errorString());
        return std::nullopt;
    }

    QImage image = reader.read();
    if (image.isNull()) {
        setError(errorString,
            reader.errorString().isEmpty()
                ? KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedImageInvalid)
                : reader.errorString());
        return std::nullopt;
    }
    return KiriView::displayReadyImage(image);
}

std::optional<QImage> bitmapThumbnailImage(
    const libraw_processed_image_t *processedImage, QString *errorString)
{
    if (processedImage->bits != 8 || (processedImage->colors != 3 && processedImage->colors != 4)) {
        setError(errorString,
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedPixelFormatUnsupported));
        return std::nullopt;
    }

    const QSize imageSize(processedImage->width, processedImage->height);
    if (!validImageSize(imageSize) || !rawPreviewByteCostFitsBudget(imageSize)) {
        setError(errorString,
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedImageSizeInvalid));
        return std::nullopt;
    }

    const std::size_t channelCount = processedImage->colors;
    const std::size_t minimumDataSize = static_cast<std::size_t>(processedImage->width)
        * static_cast<std::size_t>(processedImage->height) * channelCount;
    if (processedImage->data_size < minimumDataSize) {
        setError(errorString,
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedPixelDataInvalid));
        return std::nullopt;
    }

    QImage image(imageSize, QImage::Format_RGBA8888);
    if (image.isNull()) {
        setError(errorString,
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

std::optional<QImage> thumbnailImageFromProcessedRaw(
    const libraw_processed_image_t *processedImage, QString *errorString, bool *unsupported)
{
    if (unsupported != nullptr) {
        *unsupported = false;
    }
    if (processedImage == nullptr) {
        setError(errorString,
            KiriView::imageErrorText(KiriView::ImageErrorTextId::RawDecodedImageInvalid));
        return std::nullopt;
    }

    switch (processedImage->type) {
    case LIBRAW_IMAGE_JPEG:
        return jpegThumbnailImage(processedImage, errorString);
    case LIBRAW_IMAGE_BITMAP:
        return bitmapThumbnailImage(processedImage, errorString);
    case LIBRAW_IMAGE_JPEGXL:
    default:
        if (unsupported != nullptr) {
            *unsupported = true;
        }
        return std::nullopt;
    }
}
}

namespace KiriView {
std::optional<QSize> rawEmbeddedThumbnailPreviewTrustedOriginalSize(
    const QByteArray &data, const ImageDecodeRequest &request)
{
    if (!rawInputIsEligibleForEmbeddedThumbnail(data, request)) {
        return std::nullopt;
    }

    return trustedOriginalSizeFromRawData(data, nullptr);
}

RawEmbeddedThumbnailPreviewResult rawEmbeddedThumbnailPreviewResult(
    const QByteArray &data, const ImageDecodeRequest &request)
{
    if (!rawInputIsEligibleForEmbeddedThumbnail(data, request)) {
        return rawResult(RawEmbeddedThumbnailPreviewStatus::Missing);
    }

    QString errorString;
    LibRaw processor;
    int errorCode = processor.open_buffer(data.constData(), static_cast<std::size_t>(data.size()));
    if (errorCode != LIBRAW_SUCCESS) {
        return rawResult(RawEmbeddedThumbnailPreviewStatus::Failed, {}, {},
            rawDecodeErrorString(
                imageErrorActionText(ImageErrorActionTextId::ReadRawImage), errorCode));
    }

    const QSize originalSize = libRawImageSize(processor);
    if (!validateTrustedOriginalSize(originalSize)) {
        return rawResult(RawEmbeddedThumbnailPreviewStatus::Invalid, {}, originalSize,
            imageErrorText(ImageErrorTextId::RawDecodedImageSizeInvalid));
    }

    errorCode = processor.unpack_thumb();
    if (rawThumbnailMissingError(errorCode)) {
        return rawResult(RawEmbeddedThumbnailPreviewStatus::Missing, {}, originalSize);
    }
    if (errorCode != LIBRAW_SUCCESS) {
        return rawResult(RawEmbeddedThumbnailPreviewStatus::Failed, {}, originalSize,
            rawDecodeErrorString(
                imageErrorActionText(ImageErrorActionTextId::UnpackRawImage), errorCode));
    }

    int memImageErrorCode = LIBRAW_SUCCESS;
    ProcessedRawImage processedImage(
        processor.dcraw_make_mem_thumb(&memImageErrorCode), &LibRaw::dcraw_clear_mem);
    if (rawThumbnailMissingError(memImageErrorCode)) {
        return rawResult(RawEmbeddedThumbnailPreviewStatus::Missing, {}, originalSize);
    }
    if (memImageErrorCode != LIBRAW_SUCCESS) {
        return rawResult(RawEmbeddedThumbnailPreviewStatus::Failed, {}, originalSize,
            rawDecodeErrorString(imageErrorActionText(ImageErrorActionTextId::CreateDisplayImage),
                memImageErrorCode));
    }

    bool unsupportedThumbnail = false;
    std::optional<QImage> image
        = thumbnailImageFromProcessedRaw(processedImage.get(), &errorString, &unsupportedThumbnail);
    if (unsupportedThumbnail) {
        return rawResult(RawEmbeddedThumbnailPreviewStatus::Missing, {}, originalSize);
    }
    if (!image.has_value()) {
        return rawResult(
            RawEmbeddedThumbnailPreviewStatus::Invalid, {}, originalSize, std::move(errorString));
    }
    if (!validateRawPreviewImage(*image, originalSize, &errorString)) {
        return rawResult(RawEmbeddedThumbnailPreviewStatus::Invalid, std::move(*image),
            originalSize, std::move(errorString));
    }

    return rawResult(RawEmbeddedThumbnailPreviewStatus::Ready, std::move(*image), originalSize);
}

std::optional<StaticDisplayImagePayload> rawEmbeddedThumbnailPreviewDisplayPayload(
    const ImageDecodeRequest &request, RawEmbeddedThumbnailPreviewResult result)
{
    if (result.status != RawEmbeddedThumbnailPreviewStatus::Ready) {
        return std::nullopt;
    }

    QImage image = displayReadyImage(std::move(result.image));
    QString errorString;
    if (!validateRawPreviewImage(image, result.originalSize, &errorString)) {
        return std::nullopt;
    }

    const qreal pixelsPerSourcePixel = imagePixelsPerSourcePixel(result.originalSize, image.size());
    return StaticDisplayImagePayload {
        sourceKeyForUrl(request.imageUrl()).identity,
        {},
        result.originalSize,
        std::move(image),
        DisplayImageQuality::ThumbnailPreview,
        pixelsPerSourcePixel > 0.0 ? pixelsPerSourcePixel : 0.0,
        {},
        nullptr,
        DisplayImagePreviewOrigin::RawEmbeddedThumbnail,
    };
}
}
