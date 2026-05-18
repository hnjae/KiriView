// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "qimagereaderdecoder.h"

#include "bufferedimagereader.h"
#include "imagerendering.h"
#include "imagetilesource.h"
#include "imageviewtext.h"
#include "staticimagedecode.h"

#include <QImage>
#include <memory>
#include <utility>

namespace {
KiriView::DecodedImageResult openedStaticImageResult(
    const QByteArray &data, const KiriView::ImageDecodeRequest &request)
{
    QString errorString;
    std::shared_ptr<KiriView::ImageTileSource> source
        = KiriView::QImageReaderTileSource::open(data, &errorString);
    if (source == nullptr) {
        return KiriView::failedDecodedImageResult(errorString);
    }

    return KiriView::staticDecodedImageResult(
        std::move(source), request.firstDisplay(), &errorString);
}
}

namespace KiriView {
DecodedImageResult decodeQImageReaderImageData(
    const QByteArray &data, const ImageDecodeRequest &request)
{
    BufferedImageReader reader(data);
    if (!reader) {
        return failedDecodedImageResult(imageViewText("Could not read the selected image data."));
    }

    const bool supportsAnimation = reader.supportsAnimation();
    if (!supportsAnimation) {
        return openedStaticImageResult(data, request);
    }

    QImage image = reader.read();
    if (image.isNull()) {
        return openedStaticImageResult(data, request);
    }

    const QByteArray format = reader.format();
    const int firstFrameDelay = reader.nextImageDelay();
    const int loopCount = reader.loopCount();
    const bool hasMoreFrames = reader.canRead();

    QImage firstFrame = displayReadyImage(image);
    if (hasMoreFrames) {
        return successfulDecodedImageResult(ReaderAnimationImage {
            std::move(firstFrame),
            data,
            format,
            loopCount,
            firstFrameDelay,
        });
    }
    return openedStaticImageResult(data, request);
}
}
