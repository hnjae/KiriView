// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "filedeletionworkflow.h"

#include "imagecontainer.h"
#include "kiriview/src/filedeletionworkflow.cxx.h"

namespace {
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

KiriView::FileDeletionTarget fileDeletionTarget(KiriView::RustFileDeletionTarget target)
{
    switch (target) {
    case KiriView::RustFileDeletionTarget::DisplayedImage:
        return KiriView::FileDeletionTarget::DisplayedImage;
    case KiriView::RustFileDeletionTarget::ArchiveDocumentFile:
        return KiriView::FileDeletionTarget::ArchiveDocumentFile;
    }

    return KiriView::FileDeletionTarget::DisplayedImage;
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
FileDeletionTarget fileDeletionTargetForDisplayedLocation(const DisplayedImageLocation &location)
{
    return ::fileDeletionTarget(
        rustFileDeletionTarget(displayedLocationIsInsideArchiveDocument(location)));
}

FileDeletionCompletionAction fileDeletionCompletionAction(FileDeletionResult result)
{
    return ::fileDeletionCompletionAction(
        rustFileDeletionCompletionAction(rustFileDeletionResult(result)));
}
}
