// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENWORKFLOW_H
#define KIRIVIEW_IMAGEOPENWORKFLOW_H

#include "imageopentransition.h"

namespace KiriView {
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
