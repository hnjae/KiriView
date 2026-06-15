// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodedependencies.h"

#include "imagedataloading.h"
#include "kiriimagedecoder.h"

#include <utility>

namespace {
kiriview::ImageDataLoader imageDataLoader(kiriview::ImageWorkerScheduler workerScheduler)
{
    return [workerScheduler = std::move(workerScheduler)](QObject *receiver,
               kiriview::ImageDecodeRequest request, kiriview::ImageDataCallback callback,
               kiriview::ErrorCallback errorCallback) {
        return kiriview::startStoredImageDataLoad(receiver, std::move(request), workerScheduler,
            std::move(callback), std::move(errorCallback));
    };
}

kiriview::DecodedImageResult decodeImageDataWithDefaults(
    const QByteArray &data, const kiriview::ImageDecodeRequest &request)
{
    return kiriview::decodeImageData(data, request);
}
}

namespace kiriview {
ImageDecodeDependencies defaultImageDecodeDependencies()
{
    ImageWorkerScheduler workerScheduler = defaultImageWorkerScheduler();
    return ImageDecodeDependencies {
        imageDataLoader(workerScheduler),
        decodeImageDataWithDefaults,
        defaultThumbnailCacheLookupProvider(workerScheduler),
        rawEmbeddedThumbnailPreviewResult,
        std::move(workerScheduler),
    };
}

ImageDecodeDependencies imageDecodeDependenciesWithDefaults(ImageDecodeDependencies dependencies)
{
    ImageDecodeDependencies defaults = defaultImageDecodeDependencies();
    if (!dependencies.workerScheduler.isValid()) {
        dependencies.workerScheduler = std::move(defaults.workerScheduler);
    }
    if (!dependencies.dataLoader) {
        dependencies.dataLoader = imageDataLoader(dependencies.workerScheduler);
    }
    if (!dependencies.dataDecoder) {
        dependencies.dataDecoder = std::move(defaults.dataDecoder);
    }
    if (!dependencies.thumbnailPreviewLookupProvider) {
        dependencies.thumbnailPreviewLookupProvider
            = defaultThumbnailCacheLookupProvider(dependencies.workerScheduler);
    }
    if (!dependencies.rawEmbeddedThumbnailPreviewExtractor) {
        dependencies.rawEmbeddedThumbnailPreviewExtractor
            = std::move(defaults.rawEmbeddedThumbnailPreviewExtractor);
    }

    return dependencies;
}
}
