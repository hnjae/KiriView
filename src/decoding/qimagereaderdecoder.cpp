// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "qimagereaderdecoder.h"

#include "bufferedimagereader.h"
#include "localization/imageerrortext.h"
#include "location/sourcekey.h"
#include "rendering/imagerendering.h"
#include "rendering/qimagereadertilesource.h"
#include "staticimagedecode.h"

#include <QImage>
#include <memory>
#include <utility>

namespace {
kiriview::DecodedImageResult openedStaticImageResult(
    const QByteArray &data, const kiriview::ImageDecodeRequest &request, const QByteArray &format)
{
    QString errorString;
    std::shared_ptr<kiriview::ImageTileSource> source
        = kiriview::QImageReaderTileSource::open(data, format, &errorString);
    if (source == nullptr) {
        return kiriview::failedDecodedImageResult(errorString);
    }

    return kiriview::staticDecodedImageResult(std::move(source), request, &errorString);
}

QString sourceIdentityForRequest(const kiriview::ImageDecodeRequest &request)
{
    return kiriview::sourceKeyForUrl(request.imageUrl()).identity;
}
}

namespace kiriview {
DecodedImageResult decodeQImageReaderImageData(
    const QByteArray &data, const ImageDecodeRequest &request, QtRasterFormat format)
{
    const QByteArray readerFormat = qtImageReaderFormat(format);
    BufferedImageReader reader(data, readerFormat);
    if (!reader) {
        return failedDecodedImageResult(imageErrorText(ImageErrorTextId::ReadImageData));
    }

    const bool supportsAnimation = reader.supportsAnimation();
    if (!supportsAnimation) {
        return openedStaticImageResult(data, request, readerFormat);
    }

    QImage image = reader.read();
    if (image.isNull()) {
        return openedStaticImageResult(data, request, readerFormat);
    }

    const QByteArray animationFormat = reader.format();
    const bool hasMoreFrames = reader.canRead();

    QImage firstFrame = displayReadyImage(image);
    if (hasMoreFrames) {
        return successfulDecodedImageResult(ReaderAnimationImage {
            std::move(firstFrame),
            data,
            animationFormat,
            {},
            sourceIdentityForRequest(request),
        });
    }
    return openedStaticImageResult(data, request, readerFormat);
}
}
