// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENWORKFLOW_H
#define KIRIVIEW_IMAGEOPENWORKFLOW_H

#include "imageloadtypes.h"

namespace KiriView {
struct ImageOpenTransition;

namespace ImageOpenWorkflow {
    ImageOpenTransition beginSourceLoadTransition(
        bool hasImage, bool hasLoadingContainerNavigationTarget);
    ImageOpenTransition finishEmptySourceLoadTransition();
    ImageOpenTransition finishSuccessfulImageLoadTransition(const ImageLoadSession &session);
    ImageOpenTransition finishLoadWithErrorTransition(
        const ImageLoadSession &session, bool hasImage, bool hasDisplayedUrl);
    ImageOpenTransition finishContainerNavigationLoadWithErrorTransition();
    ImageOpenTransition finishAnimationLoadWithErrorTransition();
}
}

#endif
