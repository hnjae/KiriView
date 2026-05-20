// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DECODEDIMAGERESULT_H
#define KIRIVIEW_DECODEDIMAGERESULT_H

#include "rendering/staticimage.h"

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QtGlobal>
#include <optional>
#include <utility>
#include <variant>

namespace KiriView {
struct DecodedImageFailure {
    QString errorString;
};

struct StaticDecodedImage {
    StaticImagePayload staticImage;
};

struct ApngAnimationImage {
    QImage firstFrame;
    QByteArray data;
};

struct ReaderAnimationImage {
    QImage firstFrame;
    QByteArray data;
    QByteArray format;
};

struct HeifSequenceAnimationImage {
    QImage firstFrame;
    QByteArray data;
};

using DecodedImage = std::variant<StaticDecodedImage, ApngAnimationImage, ReaderAnimationImage,
    HeifSequenceAnimationImage>;

class DecodedImageResult
{
public:
    explicit DecodedImageResult(DecodedImageFailure failure);
    explicit DecodedImageResult(DecodedImage image);

    const DecodedImageFailure *failure() const;
    const DecodedImage *image() const;
    std::optional<DecodedImage> takeImage() &&;

private:
    using Payload = std::variant<DecodedImageFailure, DecodedImage>;

    Payload m_payload;
};

DecodedImageResult failedDecodedImageResult(QString errorString);
DecodedImageResult successfulDecodedImageResult(DecodedImage image);
template <typename Image> DecodedImageResult successfulDecodedImageResult(Image image)
{
    return successfulDecodedImageResult(DecodedImage { std::move(image) });
}
const DecodedImageFailure *decodedImageResultFailure(const DecodedImageResult &result);
const DecodedImage *decodedImageResultImage(const DecodedImageResult &result);
template <typename Image> const Image *decodedImageResultImageAs(const DecodedImageResult &result)
{
    const DecodedImage *image = decodedImageResultImage(result);
    return image == nullptr ? nullptr : std::get_if<Image>(image);
}
}

#endif
