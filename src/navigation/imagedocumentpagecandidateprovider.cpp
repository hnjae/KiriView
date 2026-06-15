// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpagecandidateprovider.h"

#include "async/imageiojobs.h"
#include "imagedocumentpagecandidateloading.h"
#include "imagedocumentpagecandidatestore.h"

#include <memory>
#include <utility>

namespace {
kiriview::ImageIoJob noOpImageDocumentPageCandidateChanges(
    QObject *, QUrl, kiriview::ImageDocumentPageCandidatesCallback, kiriview::ErrorCallback)
{
    return kiriview::ImageIoJob();
}
}

namespace kiriview {
ImageDocumentPageCandidateProvider defaultImageDocumentPageCandidateProvider(
    ImageWorkerScheduler workerScheduler, DirectoryItemListProvider directoryItemListProvider)
{
    auto candidateStore = std::make_shared<ImageDocumentPageCandidateStore>();
    return ImageDocumentPageCandidateProvider {
        [candidateStore](QObject *receiver, QUrl directoryUrl,
            ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) {
            return candidateStore->loadDirectoryImages(
                receiver, std::move(directoryUrl), std::move(callback), std::move(errorCallback));
        },
        [directoryItemListProvider = std::move(directoryItemListProvider)](QObject *receiver,
            QUrl directoryUrl, ContainerCandidatesCallback callback, ErrorCallback errorCallback) {
            return startDirectoryContainerCandidateList(receiver, std::move(directoryUrl),
                std::move(callback), std::move(errorCallback), directoryItemListProvider);
        },
        [workerScheduler = std::move(workerScheduler)](QObject *receiver,
            OpenedCollectionScopeLocation openedCollectionScope,
            ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) {
            return startOpenedCollectionCandidateList(receiver, std::move(openedCollectionScope),
                workerScheduler, std::move(callback), std::move(errorCallback));
        },
        [candidateStore](QObject *receiver, QUrl directoryUrl,
            ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) {
            return candidateStore->watchDirectoryImages(
                receiver, std::move(directoryUrl), std::move(callback), std::move(errorCallback));
        },
    };
}

ImageDocumentPageCandidateProvider imageDocumentPageNavigationCandidateProviderWithDefaults(
    ImageDocumentPageCandidateProvider provider, ImageWorkerScheduler workerScheduler,
    DirectoryItemListProvider directoryItemListProvider)
{
    const bool providerIsEmpty = !provider.directoryImageDocumentPages
        && !provider.directoryContainers && !provider.openedCollectionCandidates
        && !provider.directoryImageDocumentPageChanges;
    ImageDocumentPageCandidateProvider defaults = defaultImageDocumentPageCandidateProvider(
        std::move(workerScheduler), std::move(directoryItemListProvider));
    if (!provider.directoryImageDocumentPages) {
        provider.directoryImageDocumentPages = std::move(defaults.directoryImageDocumentPages);
    }
    if (!provider.directoryContainers) {
        provider.directoryContainers = std::move(defaults.directoryContainers);
    }
    if (!provider.openedCollectionCandidates) {
        provider.openedCollectionCandidates = std::move(defaults.openedCollectionCandidates);
    }
    if (!provider.directoryImageDocumentPageChanges) {
        provider.directoryImageDocumentPageChanges = providerIsEmpty
            ? std::move(defaults.directoryImageDocumentPageChanges)
            : noOpImageDocumentPageCandidateChanges;
    }

    return provider;
}
}
