// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "filedeletionworkflow.h"

#include "imagecontainer.h"

namespace KiriView {
FileDeletionTarget fileDeletionTargetForDisplayedLocation(const DisplayedImageLocation &location)
{
    if (displayedLocationIsInsideArchiveDocument(location)) {
        return FileDeletionTarget::ArchiveDocumentFile;
    }

    return FileDeletionTarget::DisplayedImage;
}

FileDeletionCompletionAction fileDeletionCompletionAction(FileDeletionResult result)
{
    switch (result) {
    case FileDeletionResult::Succeeded:
        return FileDeletionCompletionAction::ClearDeletedImageAndOpenFallback;
    case FileDeletionResult::Canceled:
        return FileDeletionCompletionAction::Ignore;
    case FileDeletionResult::Failed:
        return FileDeletionCompletionAction::ReportFailure;
    }

    return FileDeletionCompletionAction::ReportFailure;
}
}
