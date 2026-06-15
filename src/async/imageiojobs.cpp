// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/imageiojobs.h"

#include "archive/mediaentrysourcebackend.h"
#include "async/directorylistingjob.h"
#include "async/imagecallback.h"
#include "async/imageioworkerjob.h"
#include "location/imagedocumentlocation.h"
#include "navigation/imagedocumentpagecandidateitems.h"

#include <utility>
#include <variant>

namespace {
using kiriview::containerNavigationCandidates;
using kiriview::ErrorCallback;
using kiriview::imageDocumentPageNavigationCandidates;
using kiriview::MediaEntrySourceCandidates;
using kiriview::MediaEntrySourceCandidatesResult;
using kiriview::MediaEntrySourceError;

template <typename... Handlers> struct MediaEntrySourceResultHandler : Handlers... {
    using Handlers::operator()...;
};

template <typename... Handlers>
MediaEntrySourceResultHandler(Handlers...) -> MediaEntrySourceResultHandler<Handlers...>;

template <typename Success, typename Result, typename SuccessCallback>
void finishMediaEntrySourceWorkerResult(
    Result result, ErrorCallback errorCallback, SuccessCallback successCallback)
{
    auto resultHandler = MediaEntrySourceResultHandler {
        [&errorCallback](const MediaEntrySourceError &error) {
            kiriview::invokeIfSet(errorCallback, error.errorString);
        },
        [&successCallback](Success &success) mutable { successCallback(std::move(success)); },
    };
    std::visit(resultHandler, result);
}

template <typename CandidateCallback, typename CandidateFactory>
kiriview::ImageIoJob startDirectoryCandidateList(QObject *receiver, const QUrl &directoryUrl,
    CandidateCallback callback, ErrorCallback errorCallback,
    kiriview::DirectoryItemListProvider directoryItemListProvider,
    CandidateFactory candidateFactory)
{
    return kiriview::startDirectoryItemList(
        receiver, directoryUrl,
        [callback = std::move(callback), candidateFactory = std::move(candidateFactory)](
            KFileItemList items) mutable {
            kiriview::invokeIfSet(callback, candidateFactory(items));
        },
        std::move(errorCallback), std::move(directoryItemListProvider));
}

template <typename Work, typename Finish>
kiriview::ImageIoJob startMediaEntrySourceWorkerJob(QObject *receiver,
    const kiriview::ImageWorkerScheduler &workerScheduler, Work work, Finish finish)
{
    return kiriview::startImageIoWorkerJob(
        receiver, workerScheduler, std::move(work), std::move(finish));
}
}

namespace kiriview {
ImageIoJob startDirectoryImageDocumentPageCandidateList(QObject *receiver, QUrl directoryUrl,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback)
{
    return startDirectoryCandidateList(receiver, directoryUrl, std::move(callback),
        std::move(errorCallback), {}, imageDocumentPageNavigationCandidates);
}

ImageIoJob startDirectoryImageDocumentPageCandidateList(QObject *receiver, QUrl directoryUrl,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback,
    DirectoryItemListProvider directoryItemListProvider)
{
    return startDirectoryCandidateList(receiver, directoryUrl, std::move(callback),
        std::move(errorCallback), std::move(directoryItemListProvider),
        imageDocumentPageNavigationCandidates);
}

ImageIoJob startDirectoryContainerCandidateList(QObject *receiver, QUrl directoryUrl,
    ContainerCandidatesCallback callback, ErrorCallback errorCallback)
{
    return startDirectoryCandidateList(receiver, directoryUrl, std::move(callback),
        std::move(errorCallback), {}, containerNavigationCandidates);
}

ImageIoJob startDirectoryContainerCandidateList(QObject *receiver, QUrl directoryUrl,
    ContainerCandidatesCallback callback, ErrorCallback errorCallback,
    DirectoryItemListProvider directoryItemListProvider)
{
    return startDirectoryCandidateList(receiver, directoryUrl, std::move(callback),
        std::move(errorCallback), std::move(directoryItemListProvider),
        containerNavigationCandidates);
}

ImageIoJob startOpenedCollectionCandidateList(QObject *receiver,
    OpenedCollectionScopeLocation openedCollectionScope,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback)
{
    return startOpenedCollectionCandidateList(receiver, std::move(openedCollectionScope),
        ImageWorkerScheduler(), std::move(callback), std::move(errorCallback));
}

ImageIoJob startOpenedCollectionCandidateList(QObject *receiver,
    OpenedCollectionScopeLocation openedCollectionScope,
    const ImageWorkerScheduler &workerScheduler, ImageDocumentPageCandidatesCallback callback,
    ErrorCallback errorCallback)
{
    return startMediaEntrySourceWorkerJob(
        receiver, workerScheduler,
        [openedCollectionScope = std::move(openedCollectionScope)]() {
            return loadMediaEntrySourceCandidates(openedCollectionScope);
        },
        [callback = std::move(callback), errorCallback = std::move(errorCallback)](
            MediaEntrySourceCandidatesResult result) mutable {
            finishMediaEntrySourceWorkerResult<MediaEntrySourceCandidates>(std::move(result),
                std::move(errorCallback),
                [callback = std::move(callback)](MediaEntrySourceCandidates candidates) mutable {
                    kiriview::invokeIfSet(callback, std::move(candidates.candidates));
                });
        });
}
}
