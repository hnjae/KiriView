// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/imagedataloading.h"

#include "archive/mediaentrysourcebackend.h"
#include "async/imagecallback.h"
#include "async/imageioworkerjob.h"
#include "location/imagedocumentlocation.h"

#include <KIO/Job>
#include <KIO/StoredTransferJob>
#include <KJob>
#include <QObject>
#include <utility>
#include <variant>

namespace {
using kiriview::ErrorCallback;
using kiriview::MediaEntrySourceError;
using kiriview::MediaEntrySourceImageData;
using kiriview::MediaEntrySourceImageDataResult;

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

void cancelKJob(QObject *object)
{
    auto *job = qobject_cast<KJob *>(object);
    if (job == nullptr) {
        return;
    }

    job->kill(KJob::Quietly);
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
                        kiriview::invokeIfSet(callback, std::move(data.data));
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
                    kiriview::invokeIfSet(errorCallback, finishedJob->errorString());
                    return;
                }

                kiriview::invokeIfSet(callback, job->data());
            });
        });
    return ioJob;
}
}
