// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "heifsequencereader.h"

#include "animationtiming.h"
#include "heifcontainer.h"
#include "heifsupport.h"
#include "localization/imageerrortext.h"

#include <libheif/heif_sequences.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

namespace kiriview {
QString heifSequenceDecodeErrorString()
{
    return imageErrorText(ImageErrorTextId::DecodeHeifSequence);
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

    d->data = std::move(data);
    QString errorString;
    d->context = openHeifContext(d->data, &errorString);
    if (!d->context.has_value()) {
        close();
        return { HeifSequenceOpenStatus::Error, errorString };
    }

    if (!heif_context_has_sequence(d->context->get())) {
        close();
        return { HeifSequenceOpenStatus::NotSequence, {} };
    }

    d->track = HeifTrack(heif_context_get_track(d->context->get(), 0));
    if (d->track.get() == nullptr) {
        close();
        return { HeifSequenceOpenStatus::Error,
            imageErrorText(ImageErrorTextId::HeifSequenceTrackMissing) };
    }

    if (heif_track_get_track_handler_type(d->track.get()) != heif_track_type_image_sequence) {
        close();
        return { HeifSequenceOpenStatus::NotSequence, {} };
    }

    d->options.emplace();
    if (d->options->get() == nullptr) {
        close();
        return { HeifSequenceOpenStatus::Error,
            imageErrorText(ImageErrorTextId::HeifDecodeOptionsAllocationFailed) };
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
            *errorString = imageErrorText(ImageErrorTextId::HeifSequenceTrackMissing);
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
            *errorString = heifErrorString(
                imageErrorActionText(ImageErrorActionTextId::DecodeHeifSequence), error);
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
