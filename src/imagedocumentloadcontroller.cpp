// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentloadcontroller.h"

#include "imagedocumentsourceloadpolicy.h"
#include "imagedocumentsourceloadruntimeplan.h"
#include "imagedocumentstate.h"
#include "imagespreadpresentationcontroller.h"

#include <utility>

namespace {
KiriView::ImageDocumentSourceLoadSnapshot sourceLoadSnapshot(
    const KiriView::ImageDocumentState &state,
    const KiriView::ImageSpreadPresentationController &spreadController)
{
    return KiriView::ImageDocumentSourceLoadSnapshot {
        state.sourceUrl(),
        state.displayedArchiveDocument(),
        spreadController.rightToLeftReadingEnabled(),
    };
}

}

namespace KiriView {
ImageDocumentLoadController::ImageDocumentLoadController(ImageDocumentState &state,
    ImageSpreadPresentationController &spreadController, RuntimePlanDispatcher dispatchRuntimePlan)
    : m_state(state)
    , m_spreadController(spreadController)
    , m_dispatchRuntimePlan(std::move(dispatchRuntimePlan))
{
}

void ImageDocumentLoadController::loadSource(const ImageDocumentSourceLoadRequest &request)
{
    const ImageDocumentSourceLoadPlan plan
        = ImageDocumentSourceLoadPolicy::plan(imageDocumentSourceLoadPolicyInput(
            sourceLoadSnapshot(m_state, m_spreadController), request));
    if (m_dispatchRuntimePlan) {
        m_dispatchRuntimePlan(imageDocumentSourceLoadRuntimePlan(request, plan));
    }
}

}
