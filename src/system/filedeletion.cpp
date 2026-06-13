// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "system/filedeletion.h"

#include "async/imagecallback.h"

#include <KIO/AskUserActionInterface>
#include <KIO/DeleteOrTrashJob>
#include <KIO/Global>
#include <KJob>
#include <QObject>
#include <memory>
#include <utility>

namespace {
KIO::AskUserActionInterface::DeletionType kioDeletionType(kiriview::FileDeletionMode deletionMode)
{
    switch (deletionMode) {
    case kiriview::FileDeletionMode::MoveToTrash:
        return KIO::AskUserActionInterface::Trash;
    case kiriview::FileDeletionMode::DeletePermanently:
        return KIO::AskUserActionInterface::Delete;
    }

    return KIO::AskUserActionInterface::Trash;
}

bool isUserCanceledError(int error)
{
    return error == KJob::KilledJobError || error == KIO::ERR_USER_CANCELED;
}

void cancelKJob(QObject *object)
{
    auto *job = qobject_cast<KJob *>(object);
    if (job == nullptr) {
        return;
    }

    job->kill(KJob::Quietly);
}

kiriview::ImageIoJob startKioFileDeletion(QObject *receiver, kiriview::FileDeletionRequest request,
    kiriview::FileDeletionCallback callback)
{
    if (request.targetUrl.isEmpty()) {
        kiriview::invokeIfSet(callback, kiriview::FileDeletionResult::Failed, QString());
        return kiriview::ImageIoJob();
    }

    auto *job = new KIO::DeleteOrTrashJob(QList<QUrl> { request.targetUrl },
        kioDeletionType(request.mode), KIO::AskUserActionInterface::ForceConfirmation, receiver);
    kiriview::ImageIoJob ioJob(job, cancelKJob);
    const kiriview::ImageIoJobCompletion completion = ioJob.completion();
    QObject *context = receiver == nullptr ? job : receiver;

    QObject::connect(job, &KJob::result, context,
        [completion, callback = std::move(callback)](KJob *finishedJob) mutable {
            completion.claimAndRun([&]() {
                if (finishedJob->error() == KJob::NoError) {
                    kiriview::invokeIfSet(
                        callback, kiriview::FileDeletionResult::Succeeded, QString());
                    return;
                }

                if (isUserCanceledError(finishedJob->error())) {
                    kiriview::invokeIfSet(
                        callback, kiriview::FileDeletionResult::Canceled, QString());
                    return;
                }

                kiriview::invokeIfSet(
                    callback, kiriview::FileDeletionResult::Failed, finishedJob->errorString());
            });
        });
    job->start();
    return ioJob;
}
}

namespace kiriview {
FileDeletionCompletionAction fileDeletionCompletionAction(FileDeletionResult result)
{
    switch (result) {
    case FileDeletionResult::Succeeded:
        return FileDeletionCompletionAction::ClearDeletedTargetAndOpenFallback;
    case FileDeletionResult::Canceled:
        return FileDeletionCompletionAction::Ignore;
    case FileDeletionResult::Failed:
        return FileDeletionCompletionAction::ReportFailure;
    }

    return FileDeletionCompletionAction::ReportFailure;
}

FileDeletionProvider defaultFileDeletionProvider()
{
    return [](QObject *receiver, FileDeletionRequest request, FileDeletionCallback callback) {
        return startKioFileDeletion(receiver, std::move(request), std::move(callback));
    };
}

FileDeletionProvider fileDeletionProviderWithDefault(FileDeletionProvider provider)
{
    return provider ? std::move(provider) : defaultFileDeletionProvider();
}
}
