// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENWORKFLOWCONVERSION_H
#define KIRIVIEW_IMAGEOPENWORKFLOWCONVERSION_H

#include "document/imageopenworkflow.h"
#include "kiriview/src/policy/imageopenworkflow.cxx.h"

namespace kiriview {
namespace Bridge {
    enum class ImageDocumentSourceLoadKind {
        CurrentSource,
        ReplacementSource,
    };

    struct ImageDocumentSourceLoadPolicyInput
    {
        ImageDocumentSourceLoadKind loadKind = ImageDocumentSourceLoadKind::CurrentSource;
        bool preserveTwoPageSpreadTransition = false;
        bool rightToLeftReadingEnabled = false;
        bool sourceWithinDisplayedComicBookArchive = false;
        bool hasRequestedContainerNavigationUrl = false;
    };
}

RustImageDocumentSourceLoadPolicyInput rustImageDocumentSourceLoadPolicyInput(
    const Bridge::ImageDocumentSourceLoadPolicyInput& input);
ImageDocumentSourceLoadPlan imageDocumentSourceLoadPlanFromBridge(
    const RustImageDocumentSourceLoadPlan& rustPlan);

RustImageOpenWorkflowEvent rustImageOpenWorkflowEvent(RustImageOpenWorkflowEventKind kind);
RustImageOpenWorkflowEvent rustBeginSourceLoadEvent(ImageOpenBeginSourceLoadSnapshot snapshot);
RustImageOpenWorkflowEvent rustSuccessfulImageLoadEvent(
    ImageOpenSuccessfulImageLoadSnapshot snapshot);
RustImageOpenWorkflowEvent rustSourceLoadErrorEvent(ImageOpenLoadErrorSnapshot snapshot);
ImageOpenTransition imageOpenTransitionFromBridge(const RustImageOpenTransition& rustTransition);
}

#endif
