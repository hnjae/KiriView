// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodedependencies.h"

#include "imageiojobs.h"
#include "kiriimagedecoder.h"

#include <utility>

namespace {
KiriView::ImageIoJob loadImageData(QObject *receiver, KiriView::ImageDecodeRequest request,
    KiriView::ImageDataCallback callback, KiriView::ErrorCallback errorCallback)
{
    return KiriView::startStoredImageDataLoad(
        receiver, std::move(request), std::move(callback), std::move(errorCallback));
}

KiriView::DecodedImageResult decodeImageDataWithDefaults(
    const QByteArray &data, const KiriView::ImageDecodeRequest &request)
{
    return KiriView::decodeImageData(data, request);
}
}

namespace KiriView {
ImageDecodeDependencies defaultImageDecodeDependencies()
{
    return ImageDecodeDependencies {
        loadImageData,
        decodeImageDataWithDefaults,
    };
}

ImageDecodeDependencies imageDecodeDependenciesWithDefaults(ImageDecodeDependencies dependencies)
{
    ImageDecodeDependencies defaults = defaultImageDecodeDependencies();
    if (!dependencies.dataLoader) {
        dependencies.dataLoader = std::move(defaults.dataLoader);
    }
    if (!dependencies.dataDecoder) {
        dependencies.dataDecoder = std::move(defaults.dataDecoder);
    }

    return dependencies;
}
}
