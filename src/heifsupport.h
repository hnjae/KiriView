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

namespace KiriView {
QString heifErrorString(const char *action, const heif_error &error);
std::optional<QString> initializeHeifLibrary();

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
    heif_context *m_context = nullptr;
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
    heif_image_handle *m_handle = nullptr;
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
    heif_track *m_track = nullptr;
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
    heif_image *m_image = nullptr;
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
    heif_decoding_options *m_options = nullptr;
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
