// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_FILEDELETIONWORKFLOW_H
#define KIRIVIEW_FILEDELETIONWORKFLOW_H

#include "filedeletion.h"

namespace KiriView {
class DisplayedImageLocation;

enum class FileDeletionTarget {
    DisplayedImage,
    ArchiveDocumentFile,
};

enum class FileDeletionCompletionAction {
    ClearDeletedImageAndOpenFallback,
    Ignore,
    ReportFailure,
};

FileDeletionTarget fileDeletionTargetForDisplayedLocation(const DisplayedImageLocation &location);
FileDeletionCompletionAction fileDeletionCompletionAction(FileDeletionResult result);
}

#endif
