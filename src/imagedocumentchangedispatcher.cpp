// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentchangedispatcher.h"

#include "imagecallback.h"
#include "imagedocumentstate.h"
#include "imagespreadpresentationcontroller.h"

#include <utility>

namespace KiriView {
ImageDocumentChangeDispatcher::ImageDocumentChangeDispatcher(ImageDocumentState &state,
    ImageSpreadPresentationController &spreadController,
    ImageDocumentChangeDispatcher::ChangeCallback changeCallback)
    : m_state(state)
    , m_spreadController(spreadController)
    , m_changeCallback(std::move(changeCallback))
{
}

void ImageDocumentChangeDispatcher::dispatch(ImageDocumentChange change)
{
    const ImageDocumentChangeDispatchPlan plan
        = imageDocumentChangeDispatchPlan(change, m_state.errorString().isEmpty());

    if (plan.finishSpreadTransition) {
        m_spreadController.finishTransition();
    }

    if (plan.refreshSecondaryPage) {
        m_spreadController.refreshSecondaryPage();
    }
    if (plan.notifyRightToLeftReading) {
        m_spreadController.notifyRightToLeftReadingChanged();
    }

    invokeIfSet(m_changeCallback, change);
}
}
