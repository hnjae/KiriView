// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/directorylistingjob.h"

#include "async/imagecallback.h"

#include <KCoreDirLister>
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

KCoreDirLister *createDirectoryItemLister(QObject *parent)
{
    auto *lister = new KCoreDirLister(parent);
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(false);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);
    return lister;
}

void finishDirectoryItemListWithError(KiriView::ImageIoJobCompletion completion,
    const QString &errorString, const KiriView::ErrorCallback &errorCallback)
{
    completion.claimAndDelete([&]() { KiriView::invokeIfSet(errorCallback, errorString); });
}
}

namespace KiriView {
ImageIoJob startDirectoryItemList(QObject *receiver, QUrl directoryUrl,
    DirectoryItemListCallback callback, ErrorCallback errorCallback)
{
    auto *lister = createDirectoryItemLister(receiver);
    ImageIoJob ioJob(lister, cancelDirLister);
    const ImageIoJobCompletion completion = ioJob.completion();

    QObject::connect(lister, &KCoreDirLister::completed, receiver,
        [completion, lister, callback = std::move(callback)]() mutable {
            completion.claimAndDelete([&]() {
                KiriView::invokeIfSet(callback, lister->items(KCoreDirLister::AllItems));
            });
        });
    QObject::connect(
        lister, &KCoreDirLister::jobError, receiver, [completion, errorCallback](KIO::Job *job) {
            const QString errorString = job == nullptr ? QString() : job->errorString();
            finishDirectoryItemListWithError(completion, errorString, errorCallback);
        });

    if (!lister->openUrl(directoryUrl, KCoreDirLister::Reload)) {
        finishDirectoryItemListWithError(completion, QString(), errorCallback);
    }

    return ioJob;
}
}
