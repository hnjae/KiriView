// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/imageiojobs.h"

#include "archive/archivebackend.h"
#include "async/directorylistingjob.h"
#include "async/imagecallback.h"
#include "async/imageioworkerjob.h"
#include "location/imagedocumentlocation.h"
#include "navigation/imagecandidateitems.h"

#include <KIO/Job>
#include <KIO/StoredTransferJob>
#include <KJob>
#include <QObject>
#include <memory>
#include <utility>
#include <variant>

namespace {
using KiriView::ArchiveError;
using KiriView::ArchiveImageCandidates;
using KiriView::ArchiveImageCandidatesResult;
using KiriView::ArchiveImageData;
using KiriView::ArchiveImageDataResult;
using KiriView::containerNavigationCandidates;
using KiriView::ErrorCallback;
using KiriView::imageNavigationCandidates;

template <typename... Handlers> struct ArchiveResultHandler : Handlers... {
    using Handlers::operator()...;
};

template <typename... Handlers>
ArchiveResultHandler(Handlers...) -> ArchiveResultHandler<Handlers...>;

template <typename Success, typename Result, typename SuccessCallback>
void finishArchiveWorkerResult(
    Result result, ErrorCallback errorCallback, SuccessCallback successCallback)
{
    auto resultHandler = ArchiveResultHandler {
        [&errorCallback](
            const ArchiveError &error) { KiriView::invokeIfSet(errorCallback, error.errorString); },
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
    CandidateCallback callback, ErrorCallback errorCallback, CandidateFactory candidateFactory)
{
    return KiriView::startDirectoryItemList(
        receiver, directoryUrl,
        [callback = std::move(callback), candidateFactory = std::move(candidateFactory)](
            KFileItemList items) mutable {
            KiriView::invokeIfSet(callback, candidateFactory(items));
        },
        std::move(errorCallback));
}

template <typename Work, typename Finish>
KiriView::ImageIoJob startMediaEntrySourceWorkerJob(QObject *receiver, Work work, Finish finish)
{
    return KiriView::startImageIoWorkerJob(receiver, std::move(work), std::move(finish));
}
}

namespace KiriView {
ImageIoJob startDirectoryImageCandidateList(QObject *receiver, QUrl directoryUrl,
    ImageCandidatesCallback callback, ErrorCallback errorCallback)
{
    return startDirectoryCandidateList(receiver, directoryUrl, std::move(callback),
        std::move(errorCallback), imageNavigationCandidates);
}

ImageIoJob startDirectoryContainerCandidateList(QObject *receiver, QUrl directoryUrl,
    ContainerCandidatesCallback callback, ErrorCallback errorCallback)
{
    return startDirectoryCandidateList(receiver, directoryUrl, std::move(callback),
        std::move(errorCallback), containerNavigationCandidates);
}

ImageIoJob startOpenedCollectionCandidateList(QObject *receiver,
    OpenedCollectionScopeLocation archiveCollection, ImageCandidatesCallback callback,
    ErrorCallback errorCallback)
{
    return startMediaEntrySourceWorkerJob(
        receiver,
        [archiveCollection = std::move(archiveCollection)]() {
            return loadMediaEntrySourceCandidates(archiveCollection);
        },
        [callback = std::move(callback), errorCallback = std::move(errorCallback)](
            ArchiveImageCandidatesResult result) mutable {
            finishArchiveWorkerResult<ArchiveImageCandidates>(std::move(result),
                std::move(errorCallback),
                [callback = std::move(callback)](ArchiveImageCandidates candidates) mutable {
                    KiriView::invokeIfSet(callback, std::move(candidates.candidates));
                });
        });
}

ImageIoJob startStoredImageDataLoad(QObject *receiver, ImageDecodeRequest request,
    ImageDataCallback callback, ErrorCallback errorCallback)
{
    if (openedCollectionScopeContainsUrl(request.openedCollectionScope(), request.imageUrl())) {
        return startMediaEntrySourceWorkerJob(
            receiver,
            [request = std::move(request)]() {
                return loadMediaEntrySourceImageData(
                    request.openedCollectionScope(), request.imageUrl());
            },
            [callback = std::move(callback), errorCallback = std::move(errorCallback)](
                ArchiveImageDataResult result) mutable {
                finishArchiveWorkerResult<ArchiveImageData>(std::move(result),
                    std::move(errorCallback),
                    [callback = std::move(callback)](ArchiveImageData data) mutable {
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
