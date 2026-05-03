// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedecoder.h"

#include "imageviewtext.h"
#include "kiriview/src/avifcompat.cxx.h"

#include <libheif/heif.h>

#include <QBuffer>
#include <QColorSpace>
#include <QIODevice>
#include <QImageReader>
#include <QRectF>
#include <QSvgRenderer>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <mutex>
#include <optional>
#include <utility>

namespace {
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

KiriView::DecodedImageResult decodedImageFailure(const QString &errorString)
{
    return KiriView::DecodedImageFailure { errorString };
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
        "heic",
        "heim",
        "heis",
        "heix",
        "hevc",
        "hevm",
        "hevs",
        "hevx",
        "j2ki",
        "jpeg",
        "mif1",
        "mif2",
        "msf1",
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

std::optional<KiriView::DecodedImageResult> decodeHeifStillImageData(const QByteArray &data)
{
    if (!isLikelyHeifContainer(data)) {
        return std::nullopt;
    }

    if (std::optional<QString> initError = initializeHeifLibrary()) {
        return decodedImageFailure(*initError);
    }

    HeifContext context;
    if (context.get() == nullptr) {
        return decodedImageFailure(KiriView::imageViewText(
            "Could not decode the selected HEIF image: libheif could not allocate a context."));
    }

    heif_error error = heif_context_read_from_memory_without_copy(
        context.get(), data.constData(), static_cast<size_t>(data.size()), nullptr);
    if (error.code != heif_error_Ok) {
        return decodedImageFailure(heifErrorString("reading the HEIF container", error));
    }

    HeifImageHandle handle;
    error = heif_context_get_primary_image_handle(context.get(), handle.out());
    if (error.code != heif_error_Ok) {
        return decodedImageFailure(heifErrorString("reading the primary image", error));
    }

    HeifDecodingOptions options;
    HeifImage heifImage;
    error = heif_decode_image(handle.get(), heifImage.out(), heif_colorspace_RGB,
        heif_chroma_interleaved_RGBA, options.get());
    if (error.code != heif_error_Ok) {
        return decodedImageFailure(heifErrorString("decoding the primary image", error));
    }

    const int imageWidth = heif_image_get_width(heifImage.get(), heif_channel_interleaved);
    const int imageHeight = heif_image_get_height(heifImage.get(), heif_channel_interleaved);
    if (imageWidth <= 0 || imageHeight <= 0) {
        return decodedImageFailure(KiriView::imageViewText(
            "Could not decode the selected HEIF image: decoded image size is invalid."));
    }

    size_t sourceStride = 0;
    const uint8_t *source
        = heif_image_get_plane_readonly2(heifImage.get(), heif_channel_interleaved, &sourceStride);
    constexpr size_t bytesPerPixel = 4;
    const size_t rowBytes = static_cast<size_t>(imageWidth) * bytesPerPixel;
    if (source == nullptr || sourceStride < rowBytes) {
        return decodedImageFailure(KiriView::imageViewText(
            "Could not decode the selected HEIF image: decoded pixel data is invalid."));
    }

    QImage image(imageWidth, imageHeight, QImage::Format_RGBA8888);
    if (image.isNull()) {
        return decodedImageFailure(KiriView::imageViewText(
            "Could not decode the selected HEIF image: decoded image allocation failed."));
    }

    for (int y = 0; y < imageHeight; ++y) {
        std::memcpy(image.scanLine(y), source + (static_cast<size_t>(y) * sourceStride), rowBytes);
    }
    image.setColorSpace(QColorSpace(QColorSpace::SRgb));

    return KiriView::StaticDecodedImage { KiriView::displayReadyImage(image) };
}

std::optional<KiriView::DecodedImageResult> decodeSvgImageData(const QByteArray &data)
{
    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        return std::nullopt;
    }

    const QSize intrinsicSize = svgIntrinsicSize(renderer);
    if (intrinsicSize.isEmpty()) {
        return decodedImageFailure(
            KiriView::imageViewText("Could not determine the selected SVG image size."));
    }

    return KiriView::SvgDecodedImage { data, intrinsicSize };
}
}

namespace KiriView {
DecodedImageResult decodeImageData(const QByteArray &data)
{
    if (const std::optional<DecodedImageResult> svgResult = decodeSvgImageData(data)) {
        return *svgResult;
    }

    ApngDecodeResult apngResult = decodeApngAnimation(data);
    if (apngResult.status == ApngDecodeStatus::Success) {
        if (apngResult.animation.frames.empty()) {
            return decodedImageFailure(
                imageViewText("Could not decode the selected APNG animation."));
        }

        for (AnimationFrame &frame : apngResult.animation.frames) {
            frame.image = displayReadyImage(frame.image);
        }

        return DecodedAnimationImage {
            std::move(apngResult.animation.frames),
            apngResult.animation.loopCount,
        };
    }
    if (apngResult.status == ApngDecodeStatus::Error) {
        return decodedImageFailure(apngResult.errorString);
    }

    const QByteArray imageData = avifDataWithCompatibilityFixes(data);

    QBuffer buffer;
    buffer.setData(imageData);

    if (!buffer.open(QIODevice::ReadOnly)) {
        return decodedImageFailure(imageViewText("Could not read the selected image data."));
    }

    QImageReader reader(&buffer);
    reader.setAutoTransform(true);
    const bool supportsAnimation = reader.supportsAnimation();

    QImage image = reader.read();
    if (image.isNull()) {
        if (std::optional<DecodedImageResult> heifResult = decodeHeifStillImageData(imageData)) {
            return *heifResult;
        }
        return decodedImageFailure(reader.errorString());
    }

    const QByteArray format = reader.format();
    const int firstFrameDelay = reader.nextImageDelay();
    const int loopCount = reader.loopCount();
    const bool hasMoreFrames = reader.canRead();

    QImage firstFrame = displayReadyImage(image);
    if (supportsAnimation && hasMoreFrames) {
        return ReaderAnimationImage {
            std::move(firstFrame),
            imageData,
            format,
            loopCount,
            firstFrameDelay,
        };
    }
    return StaticDecodedImage { std::move(firstFrame) };
}

}
