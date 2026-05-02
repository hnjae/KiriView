// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageiojobs.h"

#include "asyncobjectslot.h"
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

void finishDirectoryCandidateListWithError(KiriView::AsyncObjectSlot *slot, KCoreDirLister *lister,
    quint64 token, const QString &errorString, const ErrorCallback &errorCallback)
{
    if (!slot->claim(token, lister)) {
        return;
    }

    lister->deleteLater();
    if (errorCallback) {
        errorCallback(errorString);
    }
}

template <typename Candidates, typename CandidateCallback, typename CandidateFactory>
void startDirectoryCandidateList(QObject *receiver, KiriView::AsyncObjectSlot *slot,
    const QUrl &directoryUrl, CandidateCallback callback, ErrorCallback errorCallback,
    CandidateFactory candidateFactory)
{
    auto *lister = createImageCandidateLister(receiver);
    const quint64 token = slot->start(lister, cancelDirLister);

    QObject::connect(lister, &KCoreDirLister::completed, receiver,
        [slot, lister, token, callback = std::move(callback),
            candidateFactory = std::move(candidateFactory)]() mutable {
            if (!slot->claim(token, lister)) {
                return;
            }

            Candidates candidates = candidateFactory(lister);
            lister->deleteLater();
            callback(std::move(candidates));
        });
    QObject::connect(lister, &KCoreDirLister::jobError, receiver,
        [slot, lister, token, errorCallback](KIO::Job *job) {
            const QString errorString = job == nullptr ? QString() : job->errorString();
            finishDirectoryCandidateListWithError(slot, lister, token, errorString, errorCallback);
        });
    QObject::connect(
        lister, &QObject::destroyed, receiver, [slot, lister]() { slot->clear(lister); });

    if (!lister->openUrl(directoryUrl, KCoreDirLister::Reload)) {
        finishDirectoryCandidateListWithError(slot, lister, token, QString(), errorCallback);
    }
}
}

namespace KiriView {
void startDirectoryImageCandidateList(QObject *receiver, AsyncObjectSlot *slot, QUrl directoryUrl,
    ImageCandidatesCallback callback, ErrorCallback errorCallback)
{
    startDirectoryCandidateList<std::vector<ImageNavigationCandidate>>(receiver, slot, directoryUrl,
        std::move(callback), std::move(errorCallback), imageCandidatesFromLister);
}

void startDirectoryContainerCandidateList(QObject *receiver, AsyncObjectSlot *slot,
    QUrl directoryUrl, ContainerCandidatesCallback callback, ErrorCallback errorCallback)
{
    startDirectoryCandidateList<std::vector<ContainerNavigationCandidate>>(receiver, slot,
        directoryUrl, std::move(callback), std::move(errorCallback), containerCandidatesFromLister);
}

void startArchiveImageCandidateList(QObject *receiver, AsyncObjectSlot *slot, QUrl archiveRootUrl,
    ImageCandidatesCallback callback, ErrorCallback errorCallback)
{
    auto *job
        = KIO::listRecursive(archiveRootUrl, KIO::HideProgressInfo, recursiveImageListFlags());
    auto candidates = collectArchiveImageCandidates(job, receiver, archiveRootUrl);
    const quint64 token = slot->start(job, cancelKJob);

    QObject::connect(job, &KJob::result, receiver,
        [slot, job, token, candidates, callback = std::move(callback),
            errorCallback = std::move(errorCallback)](KJob *finishedJob) mutable {
            if (!slot->claim(token, job)) {
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

    QObject::connect(job, &QObject::destroyed, receiver, [slot, job]() { slot->clear(job); });
}

void startStoredImageDataLoad(QObject *receiver, AsyncObjectSlot *slot, QUrl imageUrl,
    ImageDataCallback callback, ErrorCallback errorCallback)
{
    auto *job = KIO::storedGet(imageUrl, KIO::NoReload, KIO::HideProgressInfo);
    const quint64 token = slot->start(job, cancelKJob);

    QObject::connect(job, &KJob::result, receiver,
        [slot, job, token, callback = std::move(callback),
            errorCallback = std::move(errorCallback)](KJob *finishedJob) mutable {
            if (!slot->claim(token, job)) {
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

    QObject::connect(job, &QObject::destroyed, receiver, [slot, job]() { slot->clear(job); });
}
}
