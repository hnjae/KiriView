// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnavigationruntimeplanexecutor.h"

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

ImageDocumentNavigationRuntimePlanExecutor::ImageDocumentNavigationRuntimePlanExecutor(
    ImageDocumentNavigationRuntimeOperations operations)
    : m_operations(std::move(operations))
{
}

bool ImageDocumentNavigationRuntimePlanExecutor::dispatchOperation(
    const ImageDocumentRuntimeOperation& operation)
{
    if (std::holds_alternative<CancelPageNavigationUpdateOperation>(operation)) {
        run(m_operations.cancelPageNavigationUpdate);
        return true;
    }
    if (std::holds_alternative<CancelNavigationOperation>(operation)) {
        run(m_operations.cancelNavigation);
        return true;
    }
    if (std::holds_alternative<CancelContainerNavigationOperation>(operation)) {
        run(m_operations.cancelContainerNavigation);
        return true;
    }
    if (std::holds_alternative<CancelAllNavigationOperation>(operation)) {
        run(m_operations.cancelAllNavigation);
        return true;
    }
    if (std::holds_alternative<ClearPageNavigationOperation>(operation)) {
        run(m_operations.clearPageNavigation);
        return true;
    }
    if (std::holds_alternative<UpdatePageNavigationOperation>(operation)) {
        run(m_operations.updatePageNavigation);
        return true;
    }
    if (const auto* payload = std::get_if<LoadUrlOperation>(&operation)) {
        run(m_operations.loadUrl, payload->target);
        return true;
    }
    if (const auto* payload = std::get_if<LoadContainerImageOperation>(&operation)) {
        run(m_operations.loadContainerImage, payload->target, payload->containerUrl);
        return true;
    }
    if (const auto* payload = std::get_if<FinishEmptyContainerNavigationOperation>(&operation)) {
        run(m_operations.finishEmptyContainerNavigation, payload->containerUrl);
        return true;
    }
    if (const auto* payload
        = std::get_if<FinishContainerNavigationLoadWithErrorOperation>(&operation)) {
        run(m_operations.finishContainerNavigationLoadWithError, payload->containerUrl,
            payload->errorString);
        return true;
    }
    if (const auto* payload = std::get_if<ReportContainerNavigationBoundaryOperation>(&operation)) {
        run(m_operations.reportContainerNavigationBoundary, payload->direction);
        return true;
    }
    if (const auto* payload
        = std::get_if<ReportContainerNavigationListFailureOperation>(&operation)) {
        run(m_operations.reportContainerNavigationListFailure, payload->failure);
        return true;
    }
    if (const auto* payload = std::get_if<LoadPageNavigationUrlOperation>(&operation)) {
        run(m_operations.loadPageNavigationUrl, payload->target,
            payload->preserveTwoPageSpreadTransition);
        return true;
    }

    return false;
}
}
