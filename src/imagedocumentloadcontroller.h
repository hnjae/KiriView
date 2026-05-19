// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTLOADCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTLOADCONTROLLER_H

#include "imagedocumenteffectplan.h"
#include "imagedocumentsourceloadrequest.h"

#include <functional>

namespace KiriView {
class ImageDocumentState;
class ImageSpreadPresentationController;

class ImageDocumentLoadController final
{
public:
    using RuntimePlanDispatcher = std::function<void(const ImageDocumentRuntimePlan &)>;

    ImageDocumentLoadController(ImageDocumentState &state,
        ImageSpreadPresentationController &spreadController,
        RuntimePlanDispatcher dispatchRuntimePlan);

    void loadSource(const ImageDocumentSourceLoadRequest &request);

private:
    ImageDocumentState &m_state;
    ImageSpreadPresentationController &m_spreadController;
    RuntimePlanDispatcher m_dispatchRuntimePlan;
};
}

#endif
