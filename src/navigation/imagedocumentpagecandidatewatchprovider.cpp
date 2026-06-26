// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpagecandidatewatchprovider.h"

#include "async/imagecallback.h"
#include "imagedocumentpagecandidateitems.h"

#include <KCoreDirLister>
#include <KIO/Job>
#include <QPointer>
#include <QTimer>
#include <utility>

namespace {
constexpr const char* CanceledProperty = "_kiriviewCanceledImageDocumentPageCandidateWatch";

void cancelLiveImageDocumentPageCandidateLister(QObject* object)
{
    auto* lister = qobject_cast<KCoreDirLister*>(object);
    if (lister == nullptr) {
        return;
    }

    lister->setProperty(CanceledProperty, true);
    QObject::disconnect(lister, nullptr, nullptr, nullptr);
    lister->stop();
    lister->deleteLater();
}

KCoreDirLister* createLiveImageDocumentPageCandidateLister(QObject* parent)
{
    auto* lister = new KCoreDirLister(parent);
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(true);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);
    return lister;
}

std::vector<kiriview::ImageDocumentPageCandidate> imageDocumentPageCandidatesForLister(
    KCoreDirLister* lister, const QUrl& directoryUrl)
{
    return kiriview::imageDocumentPageNavigationCandidates(
        lister->itemsForDir(directoryUrl, KCoreDirLister::AllItems));
}

QList<QUrl> urlsForItems(const KFileItemList& items)
{
    QList<QUrl> urls;
    urls.reserve(items.size());
    for (const KFileItem& item : items) {
        urls.push_back(item.url());
    }
    return urls;
}

bool watchCanceled(const KCoreDirLister* lister)
{
    return lister == nullptr || lister->property(CanceledProperty).toBool();
}

void notifyChanged(KCoreDirLister* lister, const QUrl& directoryUrl,
    kiriview::ImageDocumentPageCandidateWatchSnapshotCallback changedSnapshot)
{
    QPointer<KCoreDirLister> guardedLister(lister);
    QTimer::singleShot(0, lister,
        [guardedLister, directoryUrl, changedSnapshot = std::move(changedSnapshot)]() mutable {
            if (guardedLister.isNull() || watchCanceled(guardedLister.data())) {
                return;
            }

            kiriview::invokeIfSet(changedSnapshot,
                imageDocumentPageCandidatesForLister(guardedLister.data(), directoryUrl));
        });
}

kiriview::ImageIoJob startKCoreImageDocumentPageCandidateWatch(QObject* receiver, QUrl directoryUrl,
    kiriview::ImageDocumentPageCandidateWatchSnapshotCallback initialSnapshot,
    kiriview::ImageDocumentPageCandidateWatchSnapshotCallback changedSnapshot,
    kiriview::ImageDocumentPageCandidateWatchDeletedCallback deletedUrls,
    kiriview::ErrorCallback errorCallback)
{
    auto* lister = createLiveImageDocumentPageCandidateLister(receiver);
    kiriview::ImageIoJob ioJob(lister, cancelLiveImageDocumentPageCandidateLister);
    QObject* context = receiver == nullptr ? lister : receiver;

    QObject::connect(lister, &KCoreDirLister::completed, context,
        [lister, directoryUrl, initialSnapshot = std::move(initialSnapshot)]() mutable {
            if (watchCanceled(lister)) {
                return;
            }

            kiriview::invokeIfSet(
                initialSnapshot, imageDocumentPageCandidatesForLister(lister, directoryUrl));
        });
    QObject::connect(lister, &KCoreDirLister::itemsAdded, context,
        [lister, directoryUrl, changedSnapshot](const QUrl&, const KFileItemList&) mutable {
            notifyChanged(lister, directoryUrl, changedSnapshot);
        });
    QObject::connect(lister, &KCoreDirLister::itemsDeleted, context,
        [lister, deletedUrls = std::move(deletedUrls)](const KFileItemList& items) {
            if (watchCanceled(lister)) {
                return;
            }

            kiriview::invokeIfSet(deletedUrls, urlsForItems(items));
        });
    QObject::connect(lister, &KCoreDirLister::refreshItems, context,
        [lister, directoryUrl, changedSnapshot](const QList<QPair<KFileItem, KFileItem>>&) mutable {
            notifyChanged(lister, directoryUrl, changedSnapshot);
        });
    QObject::connect(
        lister, &KCoreDirLister::clear, context, [lister, directoryUrl, changedSnapshot]() mutable {
            notifyChanged(lister, directoryUrl, changedSnapshot);
        });
    QObject::connect(lister, &KCoreDirLister::clearDir, context,
        [lister, directoryUrl, changedSnapshot](
            const QUrl&) mutable { notifyChanged(lister, directoryUrl, changedSnapshot); });
    QObject::connect(lister, &KCoreDirLister::jobError, context,
        [lister, errorCallback = std::move(errorCallback)](KIO::Job* job) {
            if (watchCanceled(lister)) {
                return;
            }

            kiriview::invokeIfSet(errorCallback, job == nullptr ? QString() : job->errorString());
        });

    if (!lister->openUrl(directoryUrl, KCoreDirLister::Reload)) {
        ioJob.cancel();
        return kiriview::ImageIoJob();
    }

    return ioJob;
}
}

namespace kiriview {
ImageDocumentPageCandidateWatchProvider defaultImageDocumentPageCandidateWatchProvider()
{
    return [](QObject* receiver, QUrl directoryUrl,
               ImageDocumentPageCandidateWatchSnapshotCallback initialSnapshot,
               ImageDocumentPageCandidateWatchSnapshotCallback changedSnapshot,
               ImageDocumentPageCandidateWatchDeletedCallback deletedUrls,
               ErrorCallback errorCallback) {
        return startKCoreImageDocumentPageCandidateWatch(receiver, std::move(directoryUrl),
            std::move(initialSnapshot), std::move(changedSnapshot), std::move(deletedUrls),
            std::move(errorCallback));
    };
}
}
