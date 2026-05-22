// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENWORKFLOW_H
#define KIRIVIEW_IMAGEOPENWORKFLOW_H

#include "imagedocumentruntimeplan.h"
#include "imagedocumentsourceloadrequest.h"
#include "imageopentransition.h"
#include "location/imagelocation.h"

namespace KiriView {
enum class ImageDocumentSourceLoadKind {
    CurrentSource,
    ReplacementSource,
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

struct ImageOpenBeginSourceLoadSnapshot {
    bool hasImage = false;
    bool hasLoadingContainerNavigationTarget = false;
};

struct ImageOpenSuccessfulImageLoadSnapshot {
    bool hasRequestContainerNavigationTarget = false;
};

struct ImageOpenLoadErrorSnapshot {
    bool hasContainerNavigationTarget = false;
    bool hasImage = false;
    bool hasDisplayedUrl = false;
};

namespace ImageOpenWorkflow {
    ImageDocumentSourceLoadPolicyInput sourceLoadPolicyInput(
        const ImageDocumentSourceLoadSnapshot &snapshot,
        const ImageDocumentSourceLoadRequest &request);
    ImageDocumentRuntimePlan sourceLoadPlan(const ImageDocumentSourceLoadPolicyInput &input,
        const ImageDocumentSourceLoadRequest &request);
    ImageOpenTransition beginSourceLoadTransition(ImageOpenBeginSourceLoadSnapshot snapshot);
    ImageOpenTransition finishEmptySourceLoadTransition();
    ImageOpenTransition finishSuccessfulImageLoadTransition(
        ImageOpenSuccessfulImageLoadSnapshot snapshot);
    ImageOpenTransition finishLoadWithErrorTransition(ImageOpenLoadErrorSnapshot snapshot);
    ImageOpenTransition finishContainerNavigationLoadWithErrorTransition();
    ImageOpenTransition finishAnimationLoadWithErrorTransition();
}
}

#endif
