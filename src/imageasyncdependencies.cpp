// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageasyncdependencies.h"

#include "filedeletion.h"
#include "imagecandidaterepository.h"
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
ImageNavigationCandidateProvider imageNavigationCandidateProviderWithDefaults(
    ImageNavigationCandidateProvider provider)
{
    ImageNavigationCandidateProvider defaults = defaultImageNavigationCandidateProvider();
    if (!provider.directoryImages) {
        provider.directoryImages = std::move(defaults.directoryImages);
    }
    if (!provider.directoryContainers) {
        provider.directoryContainers = std::move(defaults.directoryContainers);
    }
    if (!provider.archiveImages) {
        provider.archiveImages = std::move(defaults.archiveImages);
    }

    return provider;
}

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

FileOperationProvider fileOperationProviderWithDefault(FileOperationProvider provider)
{
    return provider ? std::move(provider) : defaultFileOperationProvider();
}

ImageAsyncDependencies defaultImageAsyncDependencies()
{
    return imageAsyncDependenciesWithDefaults({});
}

ImageAsyncDependencies imageAsyncDependenciesWithDefaults(ImageAsyncDependencies dependencies)
{
    dependencies.candidateProvider
        = imageNavigationCandidateProviderWithDefaults(std::move(dependencies.candidateProvider));
    dependencies.imageDecode
        = imageDecodeDependenciesWithDefaults(std::move(dependencies.imageDecode));
    dependencies.fileOperations
        = fileOperationProviderWithDefault(std::move(dependencies.fileOperations));
    return dependencies;
}
}
