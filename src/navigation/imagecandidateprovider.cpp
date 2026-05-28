// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidateprovider.h"

#include "async/imageiojobs.h"
#include "imagecandidatestore.h"

#include <memory>
#include <utility>

namespace {
KiriView::ImageIoJob noOpImageCandidateChanges(
    QObject *, QUrl, KiriView::ImageCandidatesCallback, KiriView::ErrorCallback)
{
    return KiriView::ImageIoJob();
}
}

namespace KiriView {
ImageNavigationCandidateProvider defaultImageNavigationCandidateProvider()
{
    auto candidateStore = std::make_shared<ImageCandidateStore>();
    return ImageNavigationCandidateProvider {
        [candidateStore](QObject *receiver, QUrl directoryUrl, ImageCandidatesCallback callback,
            ErrorCallback errorCallback) {
            return candidateStore->loadDirectoryImages(
                receiver, std::move(directoryUrl), std::move(callback), std::move(errorCallback));
        },
        startDirectoryContainerCandidateList,
        startOpenedCollectionCandidateList,
        [candidateStore](QObject *receiver, QUrl directoryUrl, ImageCandidatesCallback callback,
            ErrorCallback errorCallback) {
            return candidateStore->watchDirectoryImages(
                receiver, std::move(directoryUrl), std::move(callback), std::move(errorCallback));
        },
    };
}

ImageNavigationCandidateProvider imageNavigationCandidateProviderWithDefaults(
    ImageNavigationCandidateProvider provider)
{
    const bool providerIsEmpty = !provider.directoryImages && !provider.directoryContainers
        && !provider.openedCollectionCandidates && !provider.directoryImageChanges;
    ImageNavigationCandidateProvider defaults = defaultImageNavigationCandidateProvider();
    if (!provider.directoryImages) {
        provider.directoryImages = std::move(defaults.directoryImages);
    }
    if (!provider.directoryContainers) {
        provider.directoryContainers = std::move(defaults.directoryContainers);
    }
    if (!provider.openedCollectionCandidates) {
        provider.openedCollectionCandidates = std::move(defaults.openedCollectionCandidates);
    }
    if (!provider.directoryImageChanges) {
        provider.directoryImageChanges = providerIsEmpty ? std::move(defaults.directoryImageChanges)
                                                         : noOpImageCandidateChanges;
    }

    return provider;
}
}
