// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageiojobs.h"

#include "archivebackend.h"
#include "imageasyncworker.h"
#include "imagecontainer.h"
#include "imagenavigationmodel.h"

#include <KCoreDirLister>
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
using KiriView::ContainerNavigationCandidate;
using KiriView::containerNavigationCandidates;
using KiriView::ErrorCallback;
using KiriView::ImageNavigationCandidate;
using KiriView::imageNavigationCandidates;

template <typename... Handlers> struct ArchiveResultHandler : Handlers... {
    using Handlers::operator()...;
};

template <typename... Handlers>
ArchiveResultHandler(Handlers...) -> ArchiveResultHandler<Handlers...>;

void cancelKJob(QObject *object)
{
    auto *job = qobject_cast<KJob *>(object);
    if (job == nullptr) {
        return;
    }

    job->kill(KJob::Quietly);
}

void cancelDirLister(QObject *object)
{
    auto *lister = qobject_cast<KCoreDirLister *>(object);
    if (lister == nullptr) {
        return;
    }

    lister->stop();
    lister->deleteLater();
}

KCoreDirLister *createImageCandidateLister(QObject *parent)
{
    auto *lister = new KCoreDirLister(parent);
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(false);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);
    return lister;
}

std::vector<ImageNavigationCandidate> imageCandidatesFromLister(KCoreDirLister *lister)
{
    return imageNavigationCandidates(lister->items(KCoreDirLister::AllItems));
}

std::vector<ContainerNavigationCandidate> containerCandidatesFromLister(KCoreDirLister *lister)
{
    return containerNavigationCandidates(lister->items(KCoreDirLister::AllItems));
}

void finishDirectoryCandidateListWithError(std::shared_ptr<KiriView::ImageIoJobState> jobState,
    KCoreDirLister *lister, const QString &errorString, const ErrorCallback &errorCallback)
{
    if (!jobState->claim(lister)) {
        return;
    }

    lister->deleteLater();
    if (errorCallback) {
        errorCallback(errorString);
    }
}

template <typename Candidates, typename CandidateCallback, typename CandidateFactory>
KiriView::ImageIoJob startDirectoryCandidateList(QObject *receiver, const QUrl &directoryUrl,
    CandidateCallback callback, ErrorCallback errorCallback, CandidateFactory candidateFactory)
{
    auto *lister = createImageCandidateLister(receiver);
    KiriView::ImageIoJob ioJob(lister, cancelDirLister);
    auto jobState = ioJob.state();

    QObject::connect(lister, &KCoreDirLister::completed, receiver,
        [jobState, lister, callback = std::move(callback),
            candidateFactory = std::move(candidateFactory)]() mutable {
            if (!jobState->claim(lister)) {
                return;
            }

            Candidates candidates = candidateFactory(lister);
            lister->deleteLater();
            callback(std::move(candidates));
        });
    QObject::connect(lister, &KCoreDirLister::jobError, receiver,
        [jobState, lister, errorCallback](KIO::Job *job) {
            const QString errorString = job == nullptr ? QString() : job->errorString();
            finishDirectoryCandidateListWithError(jobState, lister, errorString, errorCallback);
        });
    QObject::connect(
        lister, &QObject::destroyed, receiver, [jobState, lister]() { jobState->clear(lister); });

    if (!lister->openUrl(directoryUrl, KCoreDirLister::Reload)) {
        finishDirectoryCandidateListWithError(jobState, lister, QString(), errorCallback);
    }

    return ioJob;
}

void cancelArchiveWorkerToken(QObject *object)
{
    if (object != nullptr) {
        object->deleteLater();
    }
}

template <typename Work, typename Finish>
KiriView::ImageIoJob startArchiveWorkerJob(QObject *receiver, Work work, Finish finish)
{
    if (receiver == nullptr) {
        finish(work());
        return KiriView::ImageIoJob();
    }

    auto *token = new QObject(receiver);
    KiriView::ImageIoJob ioJob(token, cancelArchiveWorkerToken);
    auto jobState = ioJob.state();
    QObject::connect(
        token, &QObject::destroyed, receiver, [jobState, token]() { jobState->clear(token); });

    KiriView::runAsyncWorker(receiver, std::move(work),
        [token, jobState, finish = std::move(finish)](auto result) mutable {
            if (!jobState->claim(token)) {
                return;
            }

            token->deleteLater();
            finish(std::move(result));
        });
    return ioJob;
}
}

namespace KiriView {
ImageIoJob startDirectoryImageCandidateList(QObject *receiver, QUrl directoryUrl,
    ImageCandidatesCallback callback, ErrorCallback errorCallback)
{
    return startDirectoryCandidateList<std::vector<ImageNavigationCandidate>>(receiver,
        directoryUrl, std::move(callback), std::move(errorCallback), imageCandidatesFromLister);
}

ImageIoJob startDirectoryContainerCandidateList(QObject *receiver, QUrl directoryUrl,
    ContainerCandidatesCallback callback, ErrorCallback errorCallback)
{
    return startDirectoryCandidateList<std::vector<ContainerNavigationCandidate>>(receiver,
        directoryUrl, std::move(callback), std::move(errorCallback), containerCandidatesFromLister);
}

ImageIoJob startArchiveImageCandidateList(QObject *receiver,
    ArchiveDocumentLocation archiveDocument, ImageCandidatesCallback callback,
    ErrorCallback errorCallback)
{
    return startArchiveWorkerJob(
        receiver,
        [archiveDocument = std::move(archiveDocument)]() {
            return loadArchiveDocumentImageCandidates(archiveDocument);
        },
        [callback = std::move(callback), errorCallback = std::move(errorCallback)](
            ArchiveImageCandidatesResult result) mutable {
            auto resultHandler = ArchiveResultHandler {
                [&errorCallback](const ArchiveError &error) {
                    if (errorCallback) {
                        errorCallback(error.errorString);
                    }
                },
                [&callback](ArchiveImageCandidates &candidates) {
                    if (callback) {
                        callback(std::move(candidates.candidates));
                    }
                },
            };
            std::visit(resultHandler, result);
        });
}

ImageIoJob startStoredImageDataLoad(QObject *receiver, ImageDecodeRequest request,
    ImageDataCallback callback, ErrorCallback errorCallback)
{
    if (archiveDocumentContainsUrl(request.archiveDocument, request.imageUrl)) {
        return startArchiveWorkerJob(
            receiver,
            [request = std::move(request)]() {
                return loadArchiveDocumentImageData(request.archiveDocument, request.imageUrl);
            },
            [callback = std::move(callback), errorCallback = std::move(errorCallback)](
                ArchiveImageDataResult result) mutable {
                auto resultHandler = ArchiveResultHandler {
                    [&errorCallback](const ArchiveError &error) {
                        if (errorCallback) {
                            errorCallback(error.errorString);
                        }
                    },
                    [&callback](ArchiveImageData &data) {
                        if (callback) {
                            callback(std::move(data.data));
                        }
                    },
                };
                std::visit(resultHandler, result);
            });
    }

    auto *job = KIO::storedGet(request.imageUrl, KIO::NoReload, KIO::HideProgressInfo);
    ImageIoJob ioJob(job, cancelKJob);
    auto jobState = ioJob.state();

    QObject::connect(job, &KJob::result, receiver,
        [jobState, job, callback = std::move(callback), errorCallback = std::move(errorCallback)](
            KJob *finishedJob) mutable {
            if (!jobState->claim(job)) {
                return;
            }

            if (finishedJob->error() != KJob::NoError) {
                if (errorCallback) {
                    errorCallback(finishedJob->errorString());
                }
                return;
            }

            callback(job->data());
        });

    QObject::connect(
        job, &QObject::destroyed, receiver, [jobState, job]() { jobState->clear(job); });
    return ioJob;
}
}
