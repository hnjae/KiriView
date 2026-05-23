// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENWORKFLOW_H
#define KIRIVIEW_IMAGEOPENWORKFLOW_H

#include "imagedocumentruntimeplan.h"
#include "imagedocumentsourceloadrequest.h"
#include "imageopentransition.h"
#include "location/imagelocation.h"

namespace KiriView {
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
    ImageDocumentRuntimePlan sourceLoadPlan(const ImageDocumentSourceLoadSnapshot &snapshot,
        const ImageDocumentSourceLoadRequest &request);
    ImageOpenTransition beginSourceLoadTransition(ImageOpenBeginSourceLoadSnapshot snapshot);
    ImageOpenTransition finishEmptySourceLoadTransition();
    ImageOpenTransition resolveSourceImageTransition();
    ImageOpenTransition finishSuccessfulImageLoadTransition(
        ImageOpenSuccessfulImageLoadSnapshot snapshot);
    ImageOpenTransition finishLoadWithErrorTransition(ImageOpenLoadErrorSnapshot snapshot);
    ImageOpenTransition finishContainerNavigationLoadWithErrorTransition();
    ImageOpenTransition finishAnimationLoadWithErrorTransition();
}
}

#endif
