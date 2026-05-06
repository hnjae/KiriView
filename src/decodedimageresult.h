// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DECODEDIMAGERESULT_H
#define KIRIVIEW_DECODEDIMAGERESULT_H

#include "animationframe.h"
#include "staticimage.h"

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QtGlobal>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace KiriView {
struct DecodedImageFailure {
    QString errorString;
};

struct StaticDecodedImage {
    StaticImagePayload staticImage;
};

struct DecodedAnimationImage {
    std::vector<AnimationFrame> frames;
    int loopCount = 0;
};

struct ReaderAnimationImage {
    QImage firstFrame;
    QByteArray data;
    QByteArray format;
    int loopCount = 0;
    int firstFrameDelay = 0;
};

struct HeifSequenceAnimationImage {
    QImage firstFrame;
    QByteArray data;
    int firstFrameDelay = 0;
};

using DecodedImage = std::variant<StaticDecodedImage, DecodedAnimationImage, ReaderAnimationImage,
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
DecodedImageResult staticDecodedImageResult(std::shared_ptr<ImageTileSource> source,
    const ImageFirstDisplayDecodeContext &firstDisplay, QString *errorString);
const DecodedImageFailure *decodedImageResultFailure(const DecodedImageResult &result);
const DecodedImage *decodedImageResultImage(const DecodedImageResult &result);
template <typename Image> const Image *decodedImageResultImageAs(const DecodedImageResult &result)
{
    const DecodedImage *image = decodedImageResultImage(result);
    return image == nullptr ? nullptr : std::get_if<Image>(image);
}
}

#endif
