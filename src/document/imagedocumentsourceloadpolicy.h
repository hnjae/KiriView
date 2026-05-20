// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSOURCELOADPOLICY_H
#define KIRIVIEW_IMAGEDOCUMENTSOURCELOADPOLICY_H

#include "imagedocumentsourceloadrequest.h"
#include "location/imagelocation.h"

#include <QUrl>
#include <vector>

namespace KiriView {
enum class ImageDocumentSourceLoadKind {
    CurrentSource,
    ReplacementSource,
};

enum class ImageDocumentSourceLoadOperation {
    CancelFileDeletion,
    CancelNavigationAndPredecode,
    FinishSpreadTransition,
    ResetRightToLeftReading,
    NotifyRightToLeftReadingChanged,
    ClearSecondaryPage,
    ClearLoadingContainerNavigationUrl,
    SetLoadingContainerNavigationUrlToRequested,
    SetContainerNavigationUrlToRequested,
    PrepareSourceLoad,
    SetSourceUrlToRequested,
    BeginOpen,
};

struct ImageDocumentSourceLoadPolicyInput {
    ImageDocumentSourceLoadKind loadKind = ImageDocumentSourceLoadKind::CurrentSource;
    bool preserveTwoPageSpreadTransition = false;
    bool rightToLeftReadingEnabled = false;
    bool sourceWithinDisplayedComicBookArchive = false;
    bool hasRequestedContainerNavigationUrl = false;
};

struct ImageDocumentSourceLoadSnapshot {
    QUrl currentSourceUrl;
    ArchiveDocumentLocation displayedArchiveDocument;
    bool rightToLeftReadingEnabled = false;
};

struct ImageDocumentSourceLoadPlan {
    std::vector<ImageDocumentSourceLoadOperation> operations;
};

ImageDocumentSourceLoadPolicyInput imageDocumentSourceLoadPolicyInput(
    const ImageDocumentSourceLoadSnapshot &snapshot, const ImageDocumentSourceLoadRequest &request);

namespace ImageDocumentSourceLoadPolicy {
    ImageDocumentSourceLoadPlan plan(const ImageDocumentSourceLoadPolicyInput &input);
}
}

#endif
