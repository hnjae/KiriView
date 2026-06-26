// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentlifecycleruntimeplanexecutor.h"

#include <utility>
#include <variant>

namespace kiriview {
namespace {
    template <typename Operation, typename... Args>
    void run(const Operation& operation, Args&&... args)
    {
        if (operation) {
            operation(std::forward<Args>(args)...);
        }
    }
}

ImageDocumentLifecycleRuntimePlanExecutor::ImageDocumentLifecycleRuntimePlanExecutor(
    ImageDocumentLifecycleRuntimeOperations operations)
    : m_operations(std::move(operations))
{
}

bool ImageDocumentLifecycleRuntimePlanExecutor::dispatchOperation(
    const ImageDocumentRuntimeOperation& operation)
{
    if (std::holds_alternative<CancelFileDeletionOperation>(operation)) {
        run(m_operations.cancelFileDeletion);
        return true;
    }
    if (std::holds_alternative<StopPresentationAnimationOperation>(operation)) {
        run(m_operations.stopPresentationAnimation);
        return true;
    }
    if (std::holds_alternative<ShutdownSpreadOperation>(operation)) {
        run(m_operations.shutdownSpread);
        return true;
    }

    return false;
}
}
