// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpredecoderuntimeplanexecutor.h"

#include <utility>
#include <variant>

namespace kiriview {
namespace {
    template <typename Operation, typename... Args>
    void run(const Operation &operation, Args &&...args)
    {
        if (operation) {
            operation(std::forward<Args>(args)...);
        }
    }
}

ImageDocumentPredecodeRuntimePlanExecutor::ImageDocumentPredecodeRuntimePlanExecutor(
    ImageDocumentPredecodeRuntimeOperations operations)
    : m_operations(std::move(operations))
{
}

bool ImageDocumentPredecodeRuntimePlanExecutor::dispatchOperation(
    const ImageDocumentRuntimeOperation &operation)
{
    if (std::holds_alternative<ClearPredecodeOperation>(operation)) {
        run(m_operations.clearPredecode);
        return true;
    }
    if (std::holds_alternative<CancelPredecodeOperation>(operation)) {
        run(m_operations.cancelPredecode);
        return true;
    }
    if (std::holds_alternative<ScheduleAdjacentImagePredecodeOperation>(operation)) {
        run(m_operations.scheduleAdjacentImagePredecode);
        return true;
    }

    return false;
}
}
