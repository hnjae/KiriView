// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentsourceloadruntimeplanexecutor.h"

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

ImageDocumentSourceLoadRuntimePlanExecutor::ImageDocumentSourceLoadRuntimePlanExecutor(
    ImageDocumentSourceLoadRuntimeOperations operations)
    : m_operations(std::move(operations))
{
}

bool ImageDocumentSourceLoadRuntimePlanExecutor::dispatchOperation(
    const ImageDocumentRuntimeOperation& operation)
{
    if (std::holds_alternative<ClearLoadingContainerNavigationUrlOperation>(operation)) {
        run(m_operations.clearLoadingContainerNavigationUrl);
        return true;
    }
    if (const auto* payload = std::get_if<SetLoadingContainerNavigationUrlOperation>(&operation)) {
        run(m_operations.setLoadingContainerNavigationUrl, payload->url);
        return true;
    }
    if (const auto* payload = std::get_if<SetContainerNavigationUrlOperation>(&operation)) {
        run(m_operations.setContainerNavigationUrl, payload->url);
        return true;
    }
    if (const auto* payload = std::get_if<PrepareSourceLoadOperation>(&operation)) {
        run(m_operations.prepareSourceLoad, payload->request);
        return true;
    }
    if (std::holds_alternative<BeginOpenOperation>(operation)) {
        run(m_operations.beginOpen);
        return true;
    }

    return false;
}
}
