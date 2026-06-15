// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/imageiojobs.h"

#include "archive/mediaentrysourcebackend.h"
#include "async/imagecallback.h"
#include "async/imageioworkerjob.h"

#include <utility>
#include <variant>

namespace {
using kiriview::ErrorCallback;
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

template <typename Work, typename Finish>
kiriview::ImageIoJob startMediaEntrySourceWorkerJob(QObject *receiver,
    const kiriview::ImageWorkerScheduler &workerScheduler, Work work, Finish finish)
{
    return kiriview::startImageIoWorkerJob(
        receiver, workerScheduler, std::move(work), std::move(finish));
}
}

namespace kiriview {
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
