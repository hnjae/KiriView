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
KiriView::DecodedImageResult openedStaticImageResult(
    const QByteArray &data, const KiriView::ImageDecodeRequest &request, const QByteArray &format)
{
    QString errorString;
    std::shared_ptr<KiriView::ImageTileSource> source
        = KiriView::QImageReaderTileSource::open(data, format, &errorString);
    if (source == nullptr) {
        return KiriView::failedDecodedImageResult(errorString);
    }

    return KiriView::staticDecodedImageResult(std::move(source), request, &errorString);
}

QString sourceIdentityForRequest(const KiriView::ImageDecodeRequest &request)
{
    return KiriView::sourceKeyForUrl(request.imageUrl()).identity;
}
}

namespace KiriView {
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
