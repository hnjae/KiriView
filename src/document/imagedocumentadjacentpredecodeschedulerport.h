// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTADJACENTPREDECODESCHEDULERPORT_H
#define KIRIVIEW_IMAGEDOCUMENTADJACENTPREDECODESCHEDULERPORT_H

#include "imagedocumentruntimeplan.h"

#include <functional>

namespace kiriview {
class ImageDocumentAdjacentPredecodeSchedulerPort final
{
public:
    using RuntimePlanDispatcher = std::function<void(const ImageDocumentRuntimePlan&)>;

    explicit ImageDocumentAdjacentPredecodeSchedulerPort(RuntimePlanDispatcher dispatchPlan = {});

    void scheduleAdjacentImagePredecode() const;

private:
    RuntimePlanDispatcher m_dispatchPlan;
};
}

#endif
