// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENWORKFLOWCONVERSION_H
#define KIRIVIEW_IMAGEOPENWORKFLOWCONVERSION_H

#include "document/imageopenworkflow.h"
#include "kiriview/src/policy/imageopenworkflow.cxx.h"

namespace KiriView {
RustImageDocumentSourceLoadPolicyInput rustImageDocumentSourceLoadPolicyInput(
    const ImageDocumentSourceLoadPolicyInput &input);
ImageDocumentRuntimePlan imageDocumentRuntimePlanFromBridge(
    const RustImageDocumentSourceLoadPlan &rustPlan, const ImageDocumentSourceLoadRequest &request);

RustImageOpenWorkflowEvent rustImageOpenWorkflowEvent(RustImageOpenWorkflowEventKind kind);
RustImageOpenWorkflowEvent rustBeginSourceLoadEvent(ImageOpenBeginSourceLoadSnapshot snapshot);
RustImageOpenWorkflowEvent rustSuccessfulImageLoadEvent(
    ImageOpenSuccessfulImageLoadSnapshot snapshot);
RustImageOpenWorkflowEvent rustSourceLoadErrorEvent(ImageOpenLoadErrorSnapshot snapshot);
ImageOpenTransition imageOpenTransitionFromBridge(const RustImageOpenTransition &rustTransition);
}

#endif
