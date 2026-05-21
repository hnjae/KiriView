// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediacandidateprovider.h"

#include "async/imagecallback.h"
#include "mediaformatregistry.h"

#include <KCoreDirLister>
#include <KFileItem>
#include <KIO/Job>
#include <KJob>
#include <QObject>
#include <utility>

namespace {
void cancelDirLister(QObject *object)
{
    auto *lister = qobject_cast<KCoreDirLister *>(object);
    if (lister == nullptr) {
        return;
    }

    lister->stop();
    lister->deleteLater();
}

KCoreDirLister *createMediaCandidateLister(QObject *parent)
{
    auto *lister = new KCoreDirLister(parent);
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(false);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);
    return lister;
}

std::vector<KiriView::MediaNavigationCandidate> mediaCandidatesFromLister(KCoreDirLister *lister)
{
    const KFileItemList items = lister->items(KCoreDirLister::AllItems);
    std::vector<KiriView::MediaNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(items.size()));

    for (const KFileItem &item : items) {
        const QString name = item.name();
        if (!item.isFile() || !KiriView::isSupportedOrdinaryMediaFileName(name)) {
            continue;
        }

        candidates.push_back(KiriView::MediaNavigationCandidate { item.url(), name });
    }

    KiriView::sortMediaNavigationCandidates(&candidates);
    return candidates;
}

void finishDirectoryCandidateListWithError(KiriView::ImageIoJobCompletion completion,
    const QString &errorString, const KiriView::ErrorCallback &errorCallback)
{
    completion.claimAndDelete([&]() { KiriView::invokeIfSet(errorCallback, errorString); });
}

KiriView::ImageIoJob startDirectoryMediaCandidateList(QObject *receiver, const QUrl &directoryUrl,
    KiriView::MediaCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
{
    auto *lister = createMediaCandidateLister(receiver);
    KiriView::ImageIoJob ioJob(lister, cancelDirLister);
    const KiriView::ImageIoJobCompletion completion = ioJob.completion();

    QObject::connect(lister, &KCoreDirLister::completed, receiver,
        [completion, lister, callback = std::move(callback)]() mutable {
            completion.claimAndDelete(
                [&]() { KiriView::invokeIfSet(callback, mediaCandidatesFromLister(lister)); });
        });
    QObject::connect(
        lister, &KCoreDirLister::jobError, receiver, [completion, errorCallback](KIO::Job *job) {
            const QString errorString = job == nullptr ? QString() : job->errorString();
            finishDirectoryCandidateListWithError(completion, errorString, errorCallback);
        });

    if (!lister->openUrl(directoryUrl, KCoreDirLister::Reload)) {
        finishDirectoryCandidateListWithError(completion, QString(), errorCallback);
    }

    return ioJob;
}
}

namespace KiriView {
MediaNavigationCandidateProvider defaultMediaNavigationCandidateProvider()
{
    return MediaNavigationCandidateProvider {
        [](QObject *receiver, QUrl directoryUrl, MediaCandidatesCallback callback,
            ErrorCallback errorCallback) {
            return startDirectoryMediaCandidateList(
                receiver, directoryUrl, std::move(callback), std::move(errorCallback));
        },
    };
}

MediaNavigationCandidateProvider mediaNavigationCandidateProviderWithDefault(
    MediaNavigationCandidateProvider provider)
{
    if (!provider.directoryMedia) {
        provider = defaultMediaNavigationCandidateProvider();
    }
    return provider;
}
}
