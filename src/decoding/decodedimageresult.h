// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DECODEDIMAGERESULT_H
#define KIRIVIEW_DECODEDIMAGERESULT_H

#include "metadata/embeddedmetadata.h"
#include "rendering/staticimage.h"

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QtGlobal>
#include <optional>
#include <utility>
#include <variant>

namespace kiriview {
enum class DecodedImageFailureRoute {
    Unknown,
    Svg,
    Apng,
    HeifFamily,
    Raw,
    QtRaster,
};

enum class DecodedImageFailureOperation {
    Unknown,
    OpenStaticImageSource,
    DecodeFirstDisplayImage,
    DecodeBlockingDisplayImage,
    DecodeAnimationOpen,
    DecodeRawImage,
    DecodeHeifSequenceOpen,
    DecodeHeifSequenceFrame,
};

enum class DecodedImageFailureSeverity {
    Error,
};

struct DecodedImageFailure {
    QString errorString;
    DecodedImageFailureRoute route = DecodedImageFailureRoute::Unknown;
    DecodedImageFailureOperation operation = DecodedImageFailureOperation::Unknown;
    QString diagnosticDetail;
    DecodedImageFailureSeverity severity = DecodedImageFailureSeverity::Error;
    bool retryable = false;
};

struct StaticDecodedImage {
    StaticDisplayImagePayload displayImage;
    EmbeddedMetadata embeddedMetadata;
};

struct ApngAnimationImage {
    QImage firstFrame;
    QByteArray data;
    EmbeddedMetadata embeddedMetadata;
    QString sourceIdentity;
};

struct ReaderAnimationImage {
    QImage firstFrame;
    QByteArray data;
    QByteArray format;
    EmbeddedMetadata embeddedMetadata;
    QString sourceIdentity;
};

struct WebPAnimationImage {
    QImage firstFrame;
    QByteArray data;
    EmbeddedMetadata embeddedMetadata;
    QString sourceIdentity;
};

struct JxlAnimationImage {
    QImage firstFrame;
    QByteArray data;
    EmbeddedMetadata embeddedMetadata;
    QString sourceIdentity;
};

struct HeifSequenceAnimationImage {
    QImage firstFrame;
    QByteArray data;
    EmbeddedMetadata embeddedMetadata;
    QString sourceIdentity;
};

using DecodedImage = std::variant<StaticDecodedImage, ApngAnimationImage, ReaderAnimationImage,
    WebPAnimationImage, JxlAnimationImage, HeifSequenceAnimationImage>;

class DecodedImageResult
{
public:
    explicit DecodedImageResult(DecodedImageFailure failure);
    explicit DecodedImageResult(DecodedImage image);

    const DecodedImageFailure *failure() const;
    DecodedImageFailure *failure();
    const DecodedImage *image() const;
    DecodedImage *image();
    std::optional<DecodedImage> takeImage() &&;

private:
    using Payload = std::variant<DecodedImageFailure, DecodedImage>;

    Payload m_payload;
};

DecodedImageResult failedDecodedImageResult(QString errorString);
DecodedImageResult failedDecodedImageResult(DecodedImageFailure failure);
DecodedImageResult successfulDecodedImageResult(DecodedImage image);
template <typename Image> DecodedImageResult successfulDecodedImageResult(Image image)
{
    return successfulDecodedImageResult(DecodedImage { std::move(image) });
}
const DecodedImageFailure *decodedImageResultFailure(const DecodedImageResult &result);
const DecodedImage *decodedImageResultImage(const DecodedImageResult &result);
DecodedImage *decodedImageResultImage(DecodedImageResult &result);
DecodedImageFailure *decodedImageResultFailure(DecodedImageResult &result);
const EmbeddedMetadata &decodedImageEmbeddedMetadata(const DecodedImage &image);
void setDecodedImageEmbeddedMetadata(DecodedImage &image, EmbeddedMetadata metadata);
template <typename Image> const Image *decodedImageResultImageAs(const DecodedImageResult &result)
{
    const DecodedImage *image = decodedImageResultImage(result);
    return image == nullptr ? nullptr : std::get_if<Image>(image);
}
}

#endif
