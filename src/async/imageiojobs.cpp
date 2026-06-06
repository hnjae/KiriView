// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/imageiojobs.h"

#include "archive/mediaentrysourcebackend.h"
#include "async/directorylistingjob.h"
#include "async/imagecallback.h"
#include "async/imageioworkerjob.h"
#include "location/imagedocumentlocation.h"
#include "navigation/imagedocumentpagecandidateitems.h"

#include <KIO/Job>
#include <KIO/StoredTransferJob>
#include <KJob>
#include <QObject>
#include <memory>
#include <utility>
#include <variant>

namespace {
using KiriView::containerNavigationCandidates;
using KiriView::ErrorCallback;
using KiriView::imageDocumentPageNavigationCandidates;
using KiriView::MediaEntrySourceCandidates;
using KiriView::MediaEntrySourceCandidatesResult;
using KiriView::MediaEntrySourceError;
using KiriView::MediaEntrySourceImageData;
using KiriView::MediaEntrySourceImageDataResult;

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
            KiriView::invokeIfSet(errorCallback, error.errorString);
        },
        [&successCallback](Success &success) mutable { successCallback(std::move(success)); },
    };
    std::visit(resultHandler, result);
}

void cancelKJob(QObject *object)
{
    auto *job = qobject_cast<KJob *>(object);
    if (job == nullptr) {
        return;
    }

    job->kill(KJob::Quietly);
}

template <typename CandidateCallback, typename CandidateFactory>
KiriView::ImageIoJob startDirectoryCandidateList(QObject *receiver, const QUrl &directoryUrl,
    CandidateCallback callback, ErrorCallback errorCallback,
    KiriView::DirectoryItemListProvider directoryItemListProvider,
    CandidateFactory candidateFactory)
{
    return KiriView::startDirectoryItemList(
        receiver, directoryUrl,
        [callback = std::move(callback), candidateFactory = std::move(candidateFactory)](
            KFileItemList items) mutable {
            KiriView::invokeIfSet(callback, candidateFactory(items));
        },
        std::move(errorCallback), std::move(directoryItemListProvider));
}

template <typename Work, typename Finish>
KiriView::ImageIoJob startMediaEntrySourceWorkerJob(QObject *receiver,
    const KiriView::ImageWorkerScheduler &workerScheduler, Work work, Finish finish)
{
    return KiriView::startImageIoWorkerJob(
        receiver, workerScheduler, std::move(work), std::move(finish));
}
}

namespace KiriView {
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
                    KiriView::invokeIfSet(callback, std::move(candidates.candidates));
                });
        });
}

ImageIoJob startStoredImageDataLoad(QObject *receiver, ImageDecodeRequest request,
    ImageDataCallback callback, ErrorCallback errorCallback)
{
    return startStoredImageDataLoad(receiver, std::move(request), ImageWorkerScheduler(),
        std::move(callback), std::move(errorCallback));
}

ImageIoJob startStoredImageDataLoad(QObject *receiver, ImageDecodeRequest request,
    const ImageWorkerScheduler &workerScheduler, ImageDataCallback callback,
    ErrorCallback errorCallback)
{
    if (openedCollectionScopeContainsUrl(request.openedCollectionScope(), request.imageUrl())) {
        return startMediaEntrySourceWorkerJob(
            receiver, workerScheduler,
            [request = std::move(request)]() {
                return loadMediaEntrySourceImageData(
                    request.openedCollectionScope(), request.imageUrl());
            },
            [callback = std::move(callback), errorCallback = std::move(errorCallback)](
                MediaEntrySourceImageDataResult result) mutable {
                finishMediaEntrySourceWorkerResult<MediaEntrySourceImageData>(std::move(result),
                    std::move(errorCallback),
                    [callback = std::move(callback)](MediaEntrySourceImageData data) mutable {
                        KiriView::invokeIfSet(callback, std::move(data.data));
                    });
            });
    }

    auto *job = KIO::storedGet(request.imageUrl(), KIO::NoReload, KIO::HideProgressInfo);
    ImageIoJob ioJob(job, cancelKJob);
    const ImageIoJobCompletion completion = ioJob.completion();

    QObject::connect(job, &KJob::result, receiver,
        [completion, job, callback = std::move(callback), errorCallback = std::move(errorCallback)](
            KJob *finishedJob) mutable {
            completion.claimAndRun([&]() {
                if (finishedJob->error() != KJob::NoError) {
                    KiriView::invokeIfSet(errorCallback, finishedJob->errorString());
                    return;
                }

                KiriView::invokeIfSet(callback, job->data());
            });
        });
    return ioJob;
}
}
