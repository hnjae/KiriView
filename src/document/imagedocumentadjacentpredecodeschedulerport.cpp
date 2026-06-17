// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentadjacentpredecodeschedulerport.h"

#include <utility>

namespace kiriview {
ImageDocumentAdjacentPredecodeSchedulerPort::ImageDocumentAdjacentPredecodeSchedulerPort(
    RuntimePlanDispatcher dispatchPlan)
    : m_dispatchPlan(std::move(dispatchPlan))
{
}

void ImageDocumentAdjacentPredecodeSchedulerPort::scheduleAdjacentImagePredecode() const
{
    if (m_dispatchPlan) {
        m_dispatchPlan(ImageDocumentRuntimePlan { ScheduleAdjacentImagePredecodeOperation {} });
    }
}
}
