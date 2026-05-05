// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_HEIFSUPPORT_H
#define KIRIVIEW_HEIFSUPPORT_H

#include <libheif/heif.h>
#include <libheif/heif_sequences.h>

#include <QByteArray>
#include <QImage>
#include <QString>
#include <cstdint>
#include <optional>
#include <utility>

namespace KiriView {
QString heifErrorString(const char *action, const heif_error &error);
std::optional<QString> initializeHeifLibrary();

namespace Detail {
    template <typename Resource, auto Release> class HeifResource final
    {
    public:
        HeifResource() = default;

        explicit HeifResource(Resource *resource)
            : m_resource(resource)
        {
        }

        ~HeifResource() { reset(); }

        HeifResource(const HeifResource &) = delete;
        HeifResource &operator=(const HeifResource &) = delete;

        HeifResource(HeifResource &&other) noexcept
            : m_resource(std::exchange(other.m_resource, nullptr))
        {
        }

        HeifResource &operator=(HeifResource &&other) noexcept
        {
            if (this == &other) {
                return *this;
            }

            reset(std::exchange(other.m_resource, nullptr));
            return *this;
        }

        Resource *get() const { return m_resource; }

        Resource **out()
        {
            reset();
            return &m_resource;
        }

        void reset(Resource *resource = nullptr)
        {
            if (m_resource != nullptr) {
                Release(m_resource);
            }
            m_resource = resource;
        }

    private:
        Resource *m_resource = nullptr;
    };
}

class HeifContext final
{
public:
    HeifContext();
    ~HeifContext();

    HeifContext(const HeifContext &) = delete;
    HeifContext &operator=(const HeifContext &) = delete;
    HeifContext(HeifContext &&other) noexcept;
    HeifContext &operator=(HeifContext &&other) noexcept;

    heif_context *get() const;

private:
    Detail::HeifResource<heif_context, heif_context_free> m_context;
};

class HeifImageHandle final
{
public:
    HeifImageHandle() = default;
    ~HeifImageHandle();

    HeifImageHandle(const HeifImageHandle &) = delete;
    HeifImageHandle &operator=(const HeifImageHandle &) = delete;
    HeifImageHandle(HeifImageHandle &&other) noexcept;
    HeifImageHandle &operator=(HeifImageHandle &&other) noexcept;

    heif_image_handle **out();
    const heif_image_handle *get() const;

private:
    Detail::HeifResource<heif_image_handle, heif_image_handle_release> m_handle;
};

class HeifTrack final
{
public:
    HeifTrack() = default;
    explicit HeifTrack(heif_track *track);
    ~HeifTrack();

    HeifTrack(const HeifTrack &) = delete;
    HeifTrack &operator=(const HeifTrack &) = delete;
    HeifTrack(HeifTrack &&other) noexcept;
    HeifTrack &operator=(HeifTrack &&other) noexcept;

    heif_track *get() const;

private:
    Detail::HeifResource<heif_track, heif_track_release> m_track;
};

class HeifImage final
{
public:
    HeifImage() = default;
    ~HeifImage();

    HeifImage(const HeifImage &) = delete;
    HeifImage &operator=(const HeifImage &) = delete;
    HeifImage(HeifImage &&other) noexcept;
    HeifImage &operator=(HeifImage &&other) noexcept;

    heif_image **out();
    const heif_image *get() const;

private:
    Detail::HeifResource<heif_image, heif_image_release> m_image;
};

class HeifDecodingOptions final
{
public:
    HeifDecodingOptions();
    ~HeifDecodingOptions();

    HeifDecodingOptions(const HeifDecodingOptions &) = delete;
    HeifDecodingOptions &operator=(const HeifDecodingOptions &) = delete;
    HeifDecodingOptions(HeifDecodingOptions &&other) noexcept;
    HeifDecodingOptions &operator=(HeifDecodingOptions &&other) noexcept;

    const heif_decoding_options *get() const;

private:
    Detail::HeifResource<heif_decoding_options, heif_decoding_options_free> m_options;
};

struct HeifPrimaryImage {
    HeifContext context;
    HeifImageHandle handle;
};

// The input data must outlive the returned context because libheif reads it without copying.
std::optional<HeifPrimaryImage> openHeifPrimaryImage(const QByteArray &data, QString *errorString);
std::optional<QImage> qImageFromHeifImage(const heif_image *heifImage, QString *errorString);
int heifFrameDelay(std::uint32_t duration, std::uint32_t timescale);
}

#endif
