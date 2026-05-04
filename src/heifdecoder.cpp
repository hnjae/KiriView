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

#include "heifcontainer.h"
#include "heifsupport.h"
#include "imagetilesource.h"
#include "imageviewtext.h"

#include <libheif/heif_sequences.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

namespace {
KiriView::DecodedImageResult decodedImageFailure(const QString &errorString)
{
    return KiriView::DecodedImageFailure { errorString };
}
}

namespace KiriView {
std::optional<DecodedImageResult> decodeHeifStillImageData(const QByteArray &data)
{
    if (!isLikelyHeifStillImageContainer(data)) {
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
