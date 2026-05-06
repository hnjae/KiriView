// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEASYNCDEPENDENCIES_H
#define KIRIVIEW_IMAGEASYNCDEPENDENCIES_H

#include "decodedimageresult.h"
#include "imagecandidaterepository.h"
#include "imagedecoderequest.h"
#include "imageiojob.h"
#include "imageiojobs.h"

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

struct ImageAsyncDependencies {
    ImageNavigationCandidateProvider candidateProvider;
    ImageDecodeDependencies imageDecode;
};

ImageDecodeDependencies defaultImageDecodeDependencies();
ImageAsyncDependencies defaultImageAsyncDependencies();
}

#endif
