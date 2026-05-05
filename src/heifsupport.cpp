// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "heifsupport.h"

#include "imagerendering.h"
#include "imageviewtext.h"

#include <QColorSpace>
#include <algorithm>
#include <cstring>
#include <limits>
#include <mutex>
#include <utility>

namespace {
void setHeifSupportError(QString *errorString, const QString &message)
{
    if (errorString != nullptr) {
        *errorString = message;
    }
}
}

namespace KiriView {
QString heifErrorString(const char *action, const heif_error &error)
{
    QString message = imageViewText("Unknown libheif error.");
    if (error.message != nullptr) {
        message = QString::fromUtf8(error.message);
    }
    return imageViewText("Could not decode the selected HEIF image: ") + QString::fromUtf8(action)
        + QStringLiteral(": ") + message;
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

HeifContext::HeifContext()
    : m_context(heif_context_alloc())
{
}

HeifContext::~HeifContext()
{
    if (m_context != nullptr) {
        heif_context_free(m_context);
    }
}

HeifContext::HeifContext(HeifContext &&other) noexcept
    : m_context(std::exchange(other.m_context, nullptr))
{
}

HeifContext &HeifContext::operator=(HeifContext &&other) noexcept
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

heif_context *HeifContext::get() const { return m_context; }

HeifImageHandle::~HeifImageHandle()
{
    if (m_handle != nullptr) {
        heif_image_handle_release(m_handle);
    }
}

HeifImageHandle::HeifImageHandle(HeifImageHandle &&other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr))
{
}

HeifImageHandle &HeifImageHandle::operator=(HeifImageHandle &&other) noexcept
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

heif_image_handle **HeifImageHandle::out() { return &m_handle; }

const heif_image_handle *HeifImageHandle::get() const { return m_handle; }

HeifTrack::HeifTrack(heif_track *track)
    : m_track(track)
{
}

HeifTrack::~HeifTrack()
{
    if (m_track != nullptr) {
        heif_track_release(m_track);
    }
}

HeifTrack::HeifTrack(HeifTrack &&other) noexcept
    : m_track(std::exchange(other.m_track, nullptr))
{
}

HeifTrack &HeifTrack::operator=(HeifTrack &&other) noexcept
{
    if (this == &other) {
        return *this;
    }
    if (m_track != nullptr) {
        heif_track_release(m_track);
    }
    m_track = std::exchange(other.m_track, nullptr);
    return *this;
}

heif_track *HeifTrack::get() const { return m_track; }

HeifImage::~HeifImage()
{
    if (m_image != nullptr) {
        heif_image_release(m_image);
    }
}

HeifImage::HeifImage(HeifImage &&other) noexcept
    : m_image(std::exchange(other.m_image, nullptr))
{
}

HeifImage &HeifImage::operator=(HeifImage &&other) noexcept
{
    if (this == &other) {
        return *this;
    }
    if (m_image != nullptr) {
        heif_image_release(m_image);
    }
    m_image = std::exchange(other.m_image, nullptr);
    return *this;
}

heif_image **HeifImage::out() { return &m_image; }

const heif_image *HeifImage::get() const { return m_image; }

HeifDecodingOptions::HeifDecodingOptions()
    : m_options(heif_decoding_options_alloc())
{
    if (m_options != nullptr) {
        m_options->convert_hdr_to_8bit = 1;
    }
}

HeifDecodingOptions::~HeifDecodingOptions()
{
    if (m_options != nullptr) {
        heif_decoding_options_free(m_options);
    }
}

HeifDecodingOptions::HeifDecodingOptions(HeifDecodingOptions &&other) noexcept
    : m_options(std::exchange(other.m_options, nullptr))
{
}

HeifDecodingOptions &HeifDecodingOptions::operator=(HeifDecodingOptions &&other) noexcept
{
    if (this == &other) {
        return *this;
    }
    if (m_options != nullptr) {
        heif_decoding_options_free(m_options);
    }
    m_options = std::exchange(other.m_options, nullptr);
    return *this;
}

const heif_decoding_options *HeifDecodingOptions::get() const { return m_options; }

std::optional<HeifPrimaryImage> openHeifPrimaryImage(const QByteArray &data, QString *errorString)
{
    if (std::optional<QString> initError = initializeHeifLibrary()) {
        setHeifSupportError(errorString, *initError);
        return std::nullopt;
    }

    HeifContext context;
    if (context.get() == nullptr) {
        setHeifSupportError(errorString,
            imageViewText(
                "Could not decode the selected HEIF image: libheif could not allocate a context."));
        return std::nullopt;
    }

    heif_error error = heif_context_read_from_memory_without_copy(
        context.get(), data.constData(), static_cast<size_t>(data.size()), nullptr);
    if (error.code != heif_error_Ok) {
        setHeifSupportError(errorString, heifErrorString("reading the HEIF container", error));
        return std::nullopt;
    }

    HeifImageHandle handle;
    error = heif_context_get_primary_image_handle(context.get(), handle.out());
    if (error.code != heif_error_Ok) {
        setHeifSupportError(errorString, heifErrorString("reading the primary image", error));
        return std::nullopt;
    }

    return HeifPrimaryImage { std::move(context), std::move(handle) };
}

std::optional<QImage> qImageFromHeifImage(const heif_image *heifImage, QString *errorString)
{
    if (heifImage == nullptr) {
        if (errorString != nullptr) {
            *errorString = imageViewText(
                "Could not decode the selected HEIF image: decoded image is invalid.");
        }
        return std::nullopt;
    }

    const int imageWidth = heif_image_get_width(heifImage, heif_channel_interleaved);
    const int imageHeight = heif_image_get_height(heifImage, heif_channel_interleaved);
    if (imageWidth <= 0 || imageHeight <= 0) {
        if (errorString != nullptr) {
            *errorString = imageViewText(
                "Could not decode the selected HEIF image: decoded image size is invalid.");
        }
        return std::nullopt;
    }

    size_t sourceStride = 0;
    const uint8_t *source
        = heif_image_get_plane_readonly2(heifImage, heif_channel_interleaved, &sourceStride);
    constexpr size_t bytesPerPixel = 4;
    const size_t rowBytes = static_cast<size_t>(imageWidth) * bytesPerPixel;
    if (source == nullptr || sourceStride < rowBytes) {
        if (errorString != nullptr) {
            *errorString = imageViewText(
                "Could not decode the selected HEIF image: decoded pixel data is invalid.");
        }
        return std::nullopt;
    }

    QImage image(imageWidth, imageHeight, QImage::Format_RGBA8888);
    if (image.isNull()) {
        if (errorString != nullptr) {
            *errorString = imageViewText(
                "Could not decode the selected HEIF image: decoded image allocation failed.");
        }
        return std::nullopt;
    }

    for (int y = 0; y < imageHeight; ++y) {
        std::memcpy(image.scanLine(y), source + (static_cast<size_t>(y) * sourceStride), rowBytes);
    }
    image.setColorSpace(QColorSpace(QColorSpace::SRgb));

    return displayReadyImage(image);
}

int heifFrameDelay(std::uint32_t duration, std::uint32_t timescale)
{
    if (duration == 0 || timescale == 0) {
        return 0;
    }

    const std::uint64_t delay
        = (static_cast<std::uint64_t>(duration) * 1000 + static_cast<std::uint64_t>(timescale) - 1)
        / static_cast<std::uint64_t>(timescale);
    return static_cast<int>(std::min<std::uint64_t>(
        delay, static_cast<std::uint64_t>(std::numeric_limits<int>::max())));
}
}
