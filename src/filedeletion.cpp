// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "filedeletion.h"

#include "imagecallback.h"
#include "imagecontainer.h"
#include "kiriview/src/imagedeletionpolicy.cxx.h"

#include <KIO/AskUserActionInterface>
#include <KIO/DeleteOrTrashJob>
#include <KIO/Global>
#include <KJob>
#include <QObject>
#include <memory>
#include <utility>

namespace {
KIO::AskUserActionInterface::DeletionType kioDeletionType(KiriView::FileDeletionMode deletionMode)
{
    switch (deletionMode) {
    case KiriView::FileDeletionMode::MoveToTrash:
        return KIO::AskUserActionInterface::Trash;
    case KiriView::FileDeletionMode::DeletePermanently:
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

KiriView::ImageIoJob startKioFileDeletion(QObject *receiver, KiriView::FileDeletionRequest request,
    KiriView::FileDeletionCallback callback)
{
    if (request.targetUrl.isEmpty()) {
        KiriView::invokeIfSet(callback, KiriView::FileDeletionResult::Failed, QString());
        return KiriView::ImageIoJob();
    }

    auto *job = new KIO::DeleteOrTrashJob(QList<QUrl> { request.targetUrl },
        kioDeletionType(request.mode), KIO::AskUserActionInterface::ForceConfirmation, receiver);
    KiriView::ImageIoJob ioJob(job, cancelKJob);
    std::shared_ptr<KiriView::ImageIoJobState> jobState = ioJob.state();
    QObject *context = receiver == nullptr ? job : receiver;

    QObject::connect(job, &KJob::result, context,
        [jobState, job, callback = std::move(callback)](KJob *finishedJob) mutable {
            jobState->claimAndRun(job, [&]() {
                if (finishedJob->error() == KJob::NoError) {
                    KiriView::invokeIfSet(
                        callback, KiriView::FileDeletionResult::Succeeded, QString());
                    return;
                }

                if (isUserCanceledError(finishedJob->error())) {
                    KiriView::invokeIfSet(
                        callback, KiriView::FileDeletionResult::Canceled, QString());
                    return;
                }

                KiriView::invokeIfSet(
                    callback, KiriView::FileDeletionResult::Failed, finishedJob->errorString());
            });
        });
    job->start();
    return ioJob;
}

KiriView::RustImageDeletionTargetInput rustImageDeletionTargetInput(
    const KiriView::DisplayedImageLocation &location)
{
    return KiriView::RustImageDeletionTargetInput { location.imageUrl().isEmpty(),
        location.archiveDocument().isEmpty(),
        KiriView::displayedLocationIsInsideArchiveDocument(location) };
}

KiriView::RustFileDeletionResult rustFileDeletionResult(KiriView::FileDeletionResult result)
{
    switch (result) {
    case KiriView::FileDeletionResult::Succeeded:
        return KiriView::RustFileDeletionResult::Succeeded;
    case KiriView::FileDeletionResult::Canceled:
        return KiriView::RustFileDeletionResult::Canceled;
    case KiriView::FileDeletionResult::Failed:
        return KiriView::RustFileDeletionResult::Failed;
    }

    return KiriView::RustFileDeletionResult::Failed;
}

KiriView::FileDeletionCompletionAction fileDeletionCompletionAction(
    KiriView::RustFileDeletionCompletionAction action)
{
    switch (action) {
    case KiriView::RustFileDeletionCompletionAction::ClearDeletedImageAndOpenFallback:
        return KiriView::FileDeletionCompletionAction::ClearDeletedImageAndOpenFallback;
    case KiriView::RustFileDeletionCompletionAction::Ignore:
        return KiriView::FileDeletionCompletionAction::Ignore;
    case KiriView::RustFileDeletionCompletionAction::ReportFailure:
        return KiriView::FileDeletionCompletionAction::ReportFailure;
    }

    return KiriView::FileDeletionCompletionAction::ReportFailure;
}
}

namespace KiriView {
QUrl deletionTargetUrlForDisplayedLocation(const DisplayedImageLocation &location)
{
    switch (rustImageDeletionTarget(rustImageDeletionTargetInput(location))) {
    case RustImageDeletionTarget::ArchiveDocument:
        return location.archiveDocumentFileUrl();
    case RustImageDeletionTarget::DisplayedImage:
        return location.imageUrl();
    case RustImageDeletionTarget::NoDeletionTarget:
        return {};
    }

    return {};
}

FileDeletionCompletionAction fileDeletionCompletionAction(FileDeletionResult result)
{
    return ::fileDeletionCompletionAction(
        rustFileDeletionCompletionAction(rustFileDeletionResult(result)));
}

FileOperationProvider defaultFileOperationProvider()
{
    return [](QObject *receiver, FileDeletionRequest request, FileDeletionCallback callback) {
        return startKioFileDeletion(receiver, std::move(request), std::move(callback));
    };
}
}
