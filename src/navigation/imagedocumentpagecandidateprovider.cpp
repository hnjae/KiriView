// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpagecandidateprovider.h"

#include "async/imageiojobs.h"
#include "imagedocumentpagecandidatestore.h"

#include <memory>
#include <utility>

namespace {
KiriView::ImageIoJob noOpImageDocumentPageCandidateChanges(
    QObject *, QUrl, KiriView::ImageDocumentPageCandidatesCallback, KiriView::ErrorCallback)
{
    return KiriView::ImageIoJob();
}
}

namespace KiriView {
ImageDocumentPageCandidateProvider defaultImageDocumentPageCandidateProvider(
    ImageWorkerScheduler workerScheduler)
{
    auto candidateStore = std::make_shared<ImageDocumentPageCandidateStore>();
    return ImageDocumentPageCandidateProvider {
        [candidateStore](QObject *receiver, QUrl directoryUrl,
            ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) {
            return candidateStore->loadDirectoryImages(
                receiver, std::move(directoryUrl), std::move(callback), std::move(errorCallback));
        },
        startDirectoryContainerCandidateList,
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
    ImageDocumentPageCandidateProvider provider, ImageWorkerScheduler workerScheduler)
{
    const bool providerIsEmpty = !provider.directoryImageDocumentPages
        && !provider.directoryContainers && !provider.openedCollectionCandidates
        && !provider.directoryImageDocumentPageChanges;
    ImageDocumentPageCandidateProvider defaults
        = defaultImageDocumentPageCandidateProvider(std::move(workerScheduler));
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
