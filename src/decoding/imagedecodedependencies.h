// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDECODEDEPENDENCIES_H
#define KIRIVIEW_IMAGEDECODEDEPENDENCIES_H

#include "async/imageasynccallbacks.h"
#include "async/imageiojob.h"
#include "decodedimageresult.h"
#include "imagedecoderequest.h"
#include "rawthumbnailpreview.h"
#include "session/thumbnailcachelookup.h"

#include <QByteArray>
#include <functional>

class QObject;

namespace KiriView {
using ImageDataLoader
    = std::function<ImageIoJob(QObject *, ImageDecodeRequest, ImageDataCallback, ErrorCallback)>;
using ImageDataDecoder
    = std::function<DecodedImageResult(const QByteArray &, const ImageDecodeRequest &)>;

struct ImageDecodeDependencies {
    ImageDataLoader dataLoader;
    ImageDataDecoder dataDecoder;
    ThumbnailCacheLookupProvider thumbnailPreviewLookupProvider;
    RawEmbeddedThumbnailPreviewExtractor rawEmbeddedThumbnailPreviewExtractor;
};

ImageDecodeDependencies defaultImageDecodeDependencies();
ImageDecodeDependencies imageDecodeDependenciesWithDefaults(ImageDecodeDependencies dependencies);
}

#endif
