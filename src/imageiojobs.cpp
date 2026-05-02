// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageiojobs.h"

#include "imagecontainer.h"
#include "imagenavigationmodel.h"

#include <KCoreDirLister>
#include <KIO/Job>
#include <KIO/ListJob>
#include <KIO/StoredTransferJob>
#include <KIO/UDSEntry>
#include <KJob>
#include <QObject>
#include <memory>
#include <utility>

namespace {
using KiriView::appendArchiveImageNavigationCandidates;
using KiriView::ContainerNavigationCandidate;
using KiriView::containerNavigationCandidates;
using KiriView::ErrorCallback;
using KiriView::ImageNavigationCandidate;
using KiriView::imageNavigationCandidates;
using KiriView::sortImageNavigationCandidates;

KIO::ListJob::ListFlags recursiveImageListFlags()
{
    return KIO::ListJob::ListFlags(KIO::ListJob::ListFlag::IncludeHidden);
}

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

std::shared_ptr<std::vector<ImageNavigationCandidate>> collectArchiveImageCandidates(
    KIO::ListJob *job, QObject *receiver, const QUrl &archiveRootUrl)
{
    auto candidates = std::make_shared<std::vector<ImageNavigationCandidate>>();
    QObject::connect(job, &KIO::ListJob::entries, receiver,
        [archiveRootUrl, candidates](KIO::Job *entriesJob, const KIO::UDSEntryList &entries) {
            auto *listJob = qobject_cast<KIO::ListJob *>(entriesJob);
            const QUrl directoryUrl = listJob == nullptr ? archiveRootUrl : listJob->url();
            appendArchiveImageNavigationCandidates(
                candidates.get(), entries, directoryUrl, archiveRootUrl);
        });
    return candidates;
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

ImageIoJob startArchiveImageCandidateList(QObject *receiver, QUrl archiveRootUrl,
    ImageCandidatesCallback callback, ErrorCallback errorCallback)
{
    auto *job
        = KIO::listRecursive(archiveRootUrl, KIO::HideProgressInfo, recursiveImageListFlags());
    auto candidates = collectArchiveImageCandidates(job, receiver, archiveRootUrl);
    ImageIoJob ioJob(job, cancelKJob);
    auto jobState = ioJob.state();

    QObject::connect(job, &KJob::result, receiver,
        [jobState, job, candidates, callback = std::move(callback),
            errorCallback = std::move(errorCallback)](KJob *finishedJob) mutable {
            if (!jobState->claim(job)) {
                return;
            }

            if (finishedJob->error() != KJob::NoError) {
                if (errorCallback) {
                    errorCallback(finishedJob->errorString());
                }
                return;
            }

            sortImageNavigationCandidates(candidates.get());
            callback(std::move(*candidates));
        });

    QObject::connect(
        job, &QObject::destroyed, receiver, [jobState, job]() { jobState->clear(job); });
    return ioJob;
}

ImageIoJob startStoredImageDataLoad(
    QObject *receiver, QUrl imageUrl, ImageDataCallback callback, ErrorCallback errorCallback)
{
    auto *job = KIO::storedGet(imageUrl, KIO::NoReload, KIO::HideProgressInfo);
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
