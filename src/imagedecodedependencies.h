// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDECODEDEPENDENCIES_H
#define KIRIVIEW_IMAGEDECODEDEPENDENCIES_H

#include "decodedimageresult.h"
#include "imageasynccallbacks.h"
#include "imagedecoderequest.h"
#include "imageiojob.h"

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
};

ImageDecodeDependencies defaultImageDecodeDependencies();
ImageDecodeDependencies imageDecodeDependenciesWithDefaults(ImageDecodeDependencies dependencies);
}

#endif
