// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

// Compatibility shim: KiriView decodes selected HEIF data through libheif
// because current KImageFormats/QImageReader support does not reliably cover
// every HEIF variant KiriView supports. HEIF image sequences are read here so
// they play as authored animations with frames, delays, alpha, and supported
// JPEG/JPEG 2000/AVC/HEVC/VVC sequence codecs. Remove the sequence reader and
// route HEIF sequences through QImageReader once KImageFormats reliably exposes
// those semantics. Re-evaluate the still-image fallback separately once
// KImageFormats covers the still HEIF variants KiriView supports.

#include "heifdecoder.h"

#include "imagerendering.h"
#include "imagetilesource.h"
#include "imageviewtext.h"

#include <libheif/heif.h>
#include <libheif/heif_sequences.h>

#include <QColorSpace>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <mutex>
#include <utility>

namespace {
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

std::optional<QImage> qImageFromHeifImage(const heif_image *heifImage, QString *errorString)
{
    if (heifImage == nullptr) {
        *errorString = KiriView::imageViewText(
            "Could not decode the selected HEIF image: decoded image is invalid.");
        return std::nullopt;
    }

    const int imageWidth = heif_image_get_width(heifImage, heif_channel_interleaved);
    const int imageHeight = heif_image_get_height(heifImage, heif_channel_interleaved);
    if (imageWidth <= 0 || imageHeight <= 0) {
        *errorString = KiriView::imageViewText(
            "Could not decode the selected HEIF image: decoded image size is invalid.");
        return std::nullopt;
    }

    size_t sourceStride = 0;
    const uint8_t *source
        = heif_image_get_plane_readonly2(heifImage, heif_channel_interleaved, &sourceStride);
    constexpr size_t bytesPerPixel = 4;
    const size_t rowBytes = static_cast<size_t>(imageWidth) * bytesPerPixel;
    if (source == nullptr || sourceStride < rowBytes) {
        *errorString = KiriView::imageViewText(
            "Could not decode the selected HEIF image: decoded pixel data is invalid.");
        return std::nullopt;
    }

    QImage image(imageWidth, imageHeight, QImage::Format_RGBA8888);
    if (image.isNull()) {
        *errorString = KiriView::imageViewText(
            "Could not decode the selected HEIF image: decoded image allocation failed.");
        return std::nullopt;
    }

    for (int y = 0; y < imageHeight; ++y) {
        std::memcpy(image.scanLine(y), source + (static_cast<size_t>(y) * sourceStride), rowBytes);
    }
    image.setColorSpace(QColorSpace(QColorSpace::SRgb));

    return KiriView::displayReadyImage(image);
}

int heifFrameDelay(uint32_t duration, uint32_t timescale)
{
    if (duration == 0 || timescale == 0) {
        return 0;
    }

    const uint64_t delay
        = (static_cast<uint64_t>(duration) * 1000 + static_cast<uint64_t>(timescale) - 1)
        / static_cast<uint64_t>(timescale);
    return static_cast<int>(
        std::min<uint64_t>(delay, static_cast<uint64_t>(std::numeric_limits<int>::max())));
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

bool isHeifSequenceBrand(const char *brand)
{
    static constexpr const char *brands[] = {
        "avcs",
        "avis",
        "hevc",
        "hevm",
        "hevs",
        "hevx",
        "j2is",
        "jpgs",
        "msf1",
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

bool isLikelyHeifSequenceContainer(const QByteArray &data)
{
    if (data.size() < 16 || std::memcmp(data.constData() + 4, "ftyp", 4) != 0) {
        return false;
    }

    const quint32 boxSize = readBigEndianUint32(data.constData());
    if (boxSize < 16 || boxSize > static_cast<quint32>(data.size())) {
        return false;
    }

    if (isHeifSequenceBrand(data.constData() + 8)) {
        return true;
    }

    for (quint32 offset = 16; offset + 4 <= boxSize; offset += 4) {
        if (isHeifSequenceBrand(data.constData() + offset)) {
            return true;
        }
    }

    return false;
}
}

namespace KiriView {
std::optional<DecodedImageResult> decodeHeifStillImageData(const QByteArray &data)
{
    if (!isLikelyHeifContainer(data)) {
        return std::nullopt;
    }

    QString errorString;
    std::shared_ptr<ImageTileSource> source = openHeifTileSource(data, &errorString);
    if (source == nullptr) {
        return decodedImageFailure(errorString);
    }

    QImage preview = source->decodePreview(imagePreviewLongEdgeMax, &errorString);
    if (preview.isNull()) {
        return decodedImageFailure(errorString);
    }

    return StaticDecodedImage { std::move(source), std::move(preview) };
}

std::optional<DecodedImageResult> decodeHeifSequenceImageData(const QByteArray &data)
{
    if (!isLikelyHeifSequenceContainer(data)) {
        return std::nullopt;
    }

    HeifSequenceReader reader;
    const HeifSequenceOpenResult openResult = reader.open(data);
    if (openResult.status == HeifSequenceOpenStatus::NotHeif
        || openResult.status == HeifSequenceOpenStatus::NotSequence) {
        return std::nullopt;
    }
    if (openResult.status == HeifSequenceOpenStatus::Error) {
        return decodedImageFailure(openResult.errorString);
    }

    QString errorString;
    std::optional<AnimationFrame> firstFrame = reader.readNextFrame(&errorString);
    if (!firstFrame.has_value()) {
        if (errorString.isEmpty()) {
            errorString = imageViewText("Could not decode the selected HEIF image sequence.");
        }
        return decodedImageFailure(errorString);
    }

    return HeifSequenceAnimationImage {
        std::move(firstFrame->image),
        data,
        firstFrame->delay,
    };
}

class HeifSequenceReader::Private
{
public:
    Private() = default;
    ~Private() { reset(); }

    Private(const Private &) = delete;
    Private &operator=(const Private &) = delete;

    void reset()
    {
        if (track != nullptr) {
            heif_track_release(track);
            track = nullptr;
        }
        if (context != nullptr) {
            heif_context_free(context);
            context = nullptr;
        }
        if (options != nullptr) {
            heif_decoding_options_free(options);
            options = nullptr;
        }

        data.clear();
        timescale = 0;
    }

    QByteArray data;
    heif_context *context = nullptr;
    heif_track *track = nullptr;
    heif_decoding_options *options = nullptr;
    uint32_t timescale = 0;
};

HeifSequenceReader::HeifSequenceReader()
    : d(std::make_unique<Private>())
{
}

HeifSequenceReader::~HeifSequenceReader() = default;

HeifSequenceReader::HeifSequenceReader(HeifSequenceReader &&) noexcept = default;

HeifSequenceReader &HeifSequenceReader::operator=(HeifSequenceReader &&) noexcept = default;

HeifSequenceOpenResult HeifSequenceReader::open(QByteArray data)
{
    close();

    if (!isLikelyHeifContainer(data)) {
        return { HeifSequenceOpenStatus::NotHeif, {} };
    }

    if (std::optional<QString> initError = initializeHeifLibrary()) {
        return { HeifSequenceOpenStatus::Error, *initError };
    }

    d->context = heif_context_alloc();
    if (d->context == nullptr) {
        const QString message = imageViewText(
            "Could not decode the selected HEIF image: libheif could not allocate a context.");
        return { HeifSequenceOpenStatus::Error, message };
    }

    d->data = std::move(data);
    heif_error error = heif_context_read_from_memory_without_copy(
        d->context, d->data.constData(), static_cast<size_t>(d->data.size()), nullptr);
    if (error.code != heif_error_Ok) {
        const QString message = heifErrorString("reading the HEIF container", error);
        close();
        return { HeifSequenceOpenStatus::Error, message };
    }

    if (!heif_context_has_sequence(d->context)) {
        close();
        return { HeifSequenceOpenStatus::NotSequence, {} };
    }

    d->track = heif_context_get_track(d->context, 0);
    if (d->track == nullptr) {
        close();
        return { HeifSequenceOpenStatus::Error,
            imageViewText("Could not decode the selected HEIF image: sequence track is missing.") };
    }

    if (heif_track_get_track_handler_type(d->track) != heif_track_type_image_sequence) {
        close();
        return { HeifSequenceOpenStatus::NotSequence, {} };
    }

    d->options = heif_decoding_options_alloc();
    if (d->options == nullptr) {
        close();
        constexpr const char messageStr[] = "Could not decode the selected HEIF image: libheif "
                                            "could not allocate decoding options.";
        return { HeifSequenceOpenStatus::Error, imageViewText(messageStr) };
    }
    d->options->convert_hdr_to_8bit = 1;
    d->timescale = heif_track_get_timescale(d->track);

    return { HeifSequenceOpenStatus::Success, {} };
}

std::optional<AnimationFrame> HeifSequenceReader::readNextFrame(QString *errorString)
{
    if (errorString != nullptr) {
        errorString->clear();
    }

    if (d->track == nullptr) {
        if (errorString != nullptr) {
            *errorString
                = imageViewText("Could not decode the selected HEIF image: sequence track is "
                                "missing.");
        }
        return std::nullopt;
    }

    HeifImage heifImage;
    const heif_error error = heif_track_decode_next_image(
        d->track, heifImage.out(), heif_colorspace_RGB, heif_chroma_interleaved_RGBA, d->options);
    if (error.code == heif_error_End_of_sequence) {
        return std::nullopt;
    }
    if (error.code != heif_error_Ok) {
        if (errorString != nullptr) {
            *errorString = heifErrorString("decoding the HEIF image sequence", error);
        }
        return std::nullopt;
    }

    QString conversionError;
    std::optional<QImage> image = qImageFromHeifImage(heifImage.get(), &conversionError);
    if (!image.has_value()) {
        if (errorString != nullptr) {
            *errorString = conversionError;
        }
        return std::nullopt;
    }

    return AnimationFrame {
        std::move(*image),
        heifFrameDelay(heif_image_get_duration(heifImage.get()), d->timescale),
    };
}

void HeifSequenceReader::close()
{
    if (d != nullptr) {
        d->reset();
    }
}
}
