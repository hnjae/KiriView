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

std::optional<KiriView::DecodedImageResult> decodeHeifStillImageDataForInfo(
    const QByteArray &data, const KiriView::HeifContainerInfo &info)
{
    if (!info.stillImage) {
        return std::nullopt;
    }

    QString errorString;
    std::shared_ptr<KiriView::ImageTileSource> source
        = KiriView::openHeifTileSource(data, &errorString);
    if (source == nullptr) {
        return decodedImageFailure(errorString);
    }

    QImage preview = source->decodeBlockingDisplayImage(
        KiriView::imageBlockingDisplayLongEdgeMax, &errorString);
    if (preview.isNull()) {
        return decodedImageFailure(errorString);
    }

    return KiriView::StaticDecodedImage { std::move(source), std::move(preview), {} };
}

std::optional<KiriView::DecodedImageResult> decodeHeifSequenceImageDataForInfo(
    const QByteArray &data, const KiriView::HeifContainerInfo &info)
{
    if (!info.imageSequence) {
        return std::nullopt;
    }

    KiriView::HeifSequenceReader reader;
    const KiriView::HeifSequenceOpenResult openResult = reader.open(data);
    if (openResult.status == KiriView::HeifSequenceOpenStatus::NotHeif
        || openResult.status == KiriView::HeifSequenceOpenStatus::NotSequence) {
        return std::nullopt;
    }
    if (openResult.status == KiriView::HeifSequenceOpenStatus::Error) {
        return decodedImageFailure(openResult.errorString);
    }

    QString errorString;
    std::optional<KiriView::AnimationFrame> firstFrame = reader.readNextFrame(&errorString);
    if (!firstFrame.has_value()) {
        if (errorString.isEmpty()) {
            errorString
                = KiriView::imageViewText("Could not decode the selected HEIF image sequence.");
        }
        return decodedImageFailure(errorString);
    }

    return KiriView::HeifSequenceAnimationImage {
        std::move(firstFrame->image),
        data,
        firstFrame->delay,
    };
}
}

namespace KiriView {
std::optional<DecodedImageResult> decodeHeifStillImageData(const QByteArray &data)
{
    return decodeHeifStillImageDataForInfo(data, heifContainerInfo(data));
}

std::optional<DecodedImageResult> decodeHeifSequenceImageData(const QByteArray &data)
{
    return decodeHeifSequenceImageDataForInfo(data, heifContainerInfo(data));
}

std::optional<DecodedImageResult> decodeHeifImageData(const QByteArray &data)
{
    const HeifContainerInfo info = heifContainerInfo(data);
    if (std::optional<DecodedImageResult> sequenceResult
        = decodeHeifSequenceImageDataForInfo(data, info)) {
        return sequenceResult;
    }

    return decodeHeifStillImageDataForInfo(data, info);
}

class HeifSequenceReader::Private
{
public:
    Private() = default;
    ~Private() = default;

    Private(const Private &) = delete;
    Private &operator=(const Private &) = delete;

    void reset()
    {
        options.reset();
        track = HeifTrack();
        context.reset();
        data.clear();
        timescale = 0;
    }

    QByteArray data;
    std::optional<HeifContext> context;
    HeifTrack track;
    std::optional<HeifDecodingOptions> options;
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

    d->context.emplace();
    if (d->context->get() == nullptr) {
        const QString message = imageViewText(
            "Could not decode the selected HEIF image: libheif could not allocate a context.");
        return { HeifSequenceOpenStatus::Error, message };
    }

    d->data = std::move(data);
    heif_error error = heif_context_read_from_memory_without_copy(
        d->context->get(), d->data.constData(), static_cast<size_t>(d->data.size()), nullptr);
    if (error.code != heif_error_Ok) {
        const QString message = heifErrorString("reading the HEIF container", error);
        close();
        return { HeifSequenceOpenStatus::Error, message };
    }

    if (!heif_context_has_sequence(d->context->get())) {
        close();
        return { HeifSequenceOpenStatus::NotSequence, {} };
    }

    d->track = HeifTrack(heif_context_get_track(d->context->get(), 0));
    if (d->track.get() == nullptr) {
        close();
        return { HeifSequenceOpenStatus::Error,
            imageViewText("Could not decode the selected HEIF image: sequence track is missing.") };
    }

    if (heif_track_get_track_handler_type(d->track.get()) != heif_track_type_image_sequence) {
        close();
        return { HeifSequenceOpenStatus::NotSequence, {} };
    }

    d->options.emplace();
    if (d->options->get() == nullptr) {
        close();
        constexpr const char messageStr[] = "Could not decode the selected HEIF image: libheif "
                                            "could not allocate decoding options.";
        return { HeifSequenceOpenStatus::Error, imageViewText(messageStr) };
    }
    d->timescale = heif_track_get_timescale(d->track.get());

    return { HeifSequenceOpenStatus::Success, {} };
}

std::optional<AnimationFrame> HeifSequenceReader::readNextFrame(QString *errorString)
{
    if (errorString != nullptr) {
        errorString->clear();
    }

    if (d->track.get() == nullptr) {
        if (errorString != nullptr) {
            *errorString
                = imageViewText("Could not decode the selected HEIF image: sequence track is "
                                "missing.");
        }
        return std::nullopt;
    }

    HeifImage heifImage;
    const heif_error error = heif_track_decode_next_image(d->track.get(), heifImage.out(),
        heif_colorspace_RGB, heif_chroma_interleaved_RGBA, d->options->get());
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
