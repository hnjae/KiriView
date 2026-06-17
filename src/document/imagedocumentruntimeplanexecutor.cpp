// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimeplanexecutor.h"

#include <type_traits>
#include <utility>

namespace kiriview {
namespace {
    template <typename> inline constexpr bool alwaysFalse = false;

    template <typename Operation, typename... Args>
    void run(const Operation &operation, Args &&...args)
    {
        if (operation) {
            operation(std::forward<Args>(args)...);
        }
    }

    template <typename Operation>
    inline constexpr bool isSourceLoadRuntimeOperation
        = std::is_same_v<Operation, ClearLoadingContainerNavigationUrlOperation>
        || std::is_same_v<Operation, SetLoadingContainerNavigationUrlOperation>
        || std::is_same_v<Operation, SetContainerNavigationUrlOperation>
        || std::is_same_v<Operation, PrepareSourceLoadOperation>
        || std::is_same_v<Operation, BeginOpenOperation>;

    template <typename Operation>
    inline constexpr bool isOpenRuntimeOperation = std::is_same_v<Operation, CancelOpenOperation>
        || std::is_same_v<Operation, ClearDisplayedImageLocationOperation>
        || std::is_same_v<Operation, ClearPresentationImageOperation>
        || std::is_same_v<Operation, SetSourceUrlOperation>
        || std::is_same_v<Operation, SetErrorStringOperation>
        || std::is_same_v<Operation, FinishEmptySourceLoadOperation>;

    template <typename Operation>
    inline constexpr bool isPredecodeRuntimeOperation
        = std::is_same_v<Operation, ClearPredecodeOperation>
        || std::is_same_v<Operation, CancelPredecodeOperation>
        || std::is_same_v<Operation, ScheduleAdjacentImagePredecodeOperation>;

}

ImageDocumentRuntimePlanExecutor::ImageDocumentRuntimePlanExecutor(
    ImageDocumentRuntimeOperations operations)
    : m_operations(std::move(operations))
    , m_sourceLoadExecutor(m_operations.sourceLoad)
    , m_openExecutor(m_operations.open)
    , m_predecodeExecutor(m_operations.predecode)
{
}

void ImageDocumentRuntimePlanExecutor::shutdownRuntime()
{
    dispatchPlan(imageDocumentShutdownPlan());
}

void ImageDocumentRuntimePlanExecutor::dispatchPlan(const ImageDocumentRuntimePlan &plan)
{
    for (const ImageDocumentRuntimeOperation &operation : plan) {
        dispatchOperation(operation);
    }
}

void ImageDocumentRuntimePlanExecutor::dispatchOperation(
    const ImageDocumentRuntimeOperation &operation)
{
    if (m_sourceLoadExecutor.dispatchOperation(operation)) {
        return;
    }
    if (m_openExecutor.dispatchOperation(operation)) {
        return;
    }
    if (m_predecodeExecutor.dispatchOperation(operation)) {
        return;
    }

    std::visit(
        [this](const auto &payload) {
            using Operation = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<Operation, CancelFileDeletionOperation>) {
                run(m_operations.lifecycle.cancelFileDeletion);
            } else if constexpr (std::is_same_v<Operation, StopPresentationAnimationOperation>) {
                run(m_operations.lifecycle.stopPresentationAnimation);
            } else if constexpr (std::is_same_v<Operation, ShutdownSpreadOperation>) {
                run(m_operations.lifecycle.shutdownSpread);
            } else if constexpr (std::is_same_v<Operation, ClearMediaEntrySourceOperation>) {
                run(m_operations.mediaEntrySource.clear);
            } else if constexpr (std::is_same_v<Operation, FinishSpreadTransitionOperation>) {
                run(m_operations.spread.finishSpreadTransition);
            } else if constexpr (std::is_same_v<Operation, ResetRightToLeftReadingOperation>) {
                run(m_operations.spread.resetRightToLeftReading);
            } else if constexpr (std::is_same_v<Operation, ClearSecondaryPageOperation>) {
                run(m_operations.spread.clearSecondaryPage);
            } else if constexpr (std::is_same_v<Operation,
                                     NotifyRightToLeftReadingChangedOperation>) {
                run(m_operations.spread.notifyRightToLeftReadingChanged);
            } else if constexpr (std::is_same_v<Operation, ResetZoomOperation>) {
                run(m_operations.spread.resetZoom);
            } else if constexpr (std::is_same_v<Operation, PrepareFailedContainerOperation>) {
                run(m_operations.spread.prepareFailedContainer, payload.containerUrl);
            } else if constexpr (std::is_same_v<Operation, CancelPageNavigationUpdateOperation>) {
                run(m_operations.navigation.cancelPageNavigationUpdate);
            } else if constexpr (std::is_same_v<Operation, CancelNavigationOperation>) {
                run(m_operations.navigation.cancelNavigation);
            } else if constexpr (std::is_same_v<Operation, CancelContainerNavigationOperation>) {
                run(m_operations.navigation.cancelContainerNavigation);
            } else if constexpr (std::is_same_v<Operation, CancelAllNavigationOperation>) {
                run(m_operations.navigation.cancelAllNavigation);
            } else if constexpr (std::is_same_v<Operation, ClearPageNavigationOperation>) {
                run(m_operations.navigation.clearPageNavigation);
            } else if constexpr (std::is_same_v<Operation, UpdatePageNavigationOperation>) {
                run(m_operations.navigation.updatePageNavigation);
            } else if constexpr (std::is_same_v<Operation, LoadUrlOperation>) {
                run(m_operations.navigation.loadUrl, payload.target);
            } else if constexpr (std::is_same_v<Operation, LoadContainerImageOperation>) {
                run(m_operations.navigation.loadContainerImage, payload.target,
                    payload.containerUrl);
            } else if constexpr (std::is_same_v<Operation,
                                     FinishEmptyContainerNavigationOperation>) {
                run(m_operations.navigation.finishEmptyContainerNavigation, payload.containerUrl);
            } else if constexpr (std::is_same_v<Operation,
                                     FinishContainerNavigationLoadWithErrorOperation>) {
                run(m_operations.navigation.finishContainerNavigationLoadWithError,
                    payload.containerUrl, payload.errorString);
            } else if constexpr (std::is_same_v<Operation,
                                     ReportContainerNavigationBoundaryOperation>) {
                run(m_operations.navigation.reportContainerNavigationBoundary, payload.direction);
            } else if constexpr (std::is_same_v<Operation,
                                     ReportContainerNavigationListFailureOperation>) {
                run(m_operations.navigation.reportContainerNavigationListFailure, payload.failure);
            } else if constexpr (std::is_same_v<Operation, LoadPageNavigationUrlOperation>) {
                run(m_operations.navigation.loadPageNavigationUrl, payload.target,
                    payload.preserveTwoPageSpreadTransition);
            } else if constexpr (isSourceLoadRuntimeOperation<Operation>
                || isOpenRuntimeOperation<Operation> || isPredecodeRuntimeOperation<Operation>) {
                // Delegated before the visitor; this branch keeps std::visit exhaustive.
            } else {
                static_assert(alwaysFalse<Operation>, "Unhandled image document runtime operation");
            }
        },
        operation);
}
}
