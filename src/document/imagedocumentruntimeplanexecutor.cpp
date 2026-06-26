// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimeplanexecutor.h"

#include <type_traits>
#include <utility>

namespace kiriview {
namespace {
    template <typename> inline constexpr bool alwaysFalse = false;

    template <typename Operation>
    inline constexpr bool isLifecycleRuntimeOperation
        = std::is_same_v<Operation, CancelFileDeletionOperation>
        || std::is_same_v<Operation, StopPresentationAnimationOperation>
        || std::is_same_v<Operation, ShutdownSpreadOperation>;

    template <typename Operation>
    inline constexpr bool isMediaEntrySourceRuntimeOperation
        = std::is_same_v<Operation, ClearMediaEntrySourceOperation>;

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

    template <typename Operation>
    inline constexpr bool isNavigationRuntimeOperation
        = std::is_same_v<Operation, CancelPageNavigationUpdateOperation>
        || std::is_same_v<Operation, CancelNavigationOperation>
        || std::is_same_v<Operation, CancelContainerNavigationOperation>
        || std::is_same_v<Operation, CancelAllNavigationOperation>
        || std::is_same_v<Operation, ClearPageNavigationOperation>
        || std::is_same_v<Operation, UpdatePageNavigationOperation>
        || std::is_same_v<Operation, LoadUrlOperation>
        || std::is_same_v<Operation, LoadContainerImageOperation>
        || std::is_same_v<Operation, FinishEmptyContainerNavigationOperation>
        || std::is_same_v<Operation, FinishContainerNavigationLoadWithErrorOperation>
        || std::is_same_v<Operation, ReportContainerNavigationBoundaryOperation>
        || std::is_same_v<Operation, ReportContainerNavigationListFailureOperation>
        || std::is_same_v<Operation, LoadPageNavigationUrlOperation>;

    template <typename Operation>
    inline constexpr bool isSpreadRuntimeOperation
        = std::is_same_v<Operation, FinishSpreadTransitionOperation>
        || std::is_same_v<Operation, ResetRightToLeftReadingOperation>
        || std::is_same_v<Operation, ClearSecondaryPageOperation>
        || std::is_same_v<Operation, NotifyRightToLeftReadingChangedOperation>
        || std::is_same_v<Operation, ResetZoomOperation>
        || std::is_same_v<Operation, PrepareFailedContainerOperation>;

}

ImageDocumentRuntimePlanExecutor::ImageDocumentRuntimePlanExecutor(
    ImageDocumentRuntimeOperations operations)
    : m_operations(std::move(operations))
    , m_lifecycleExecutor(m_operations.lifecycle)
    , m_mediaEntrySourceExecutor(m_operations.mediaEntrySource)
    , m_sourceLoadExecutor(m_operations.sourceLoad)
    , m_openExecutor(m_operations.open)
    , m_predecodeExecutor(m_operations.predecode)
    , m_spreadExecutor(m_operations.spread)
    , m_navigationExecutor(m_operations.navigation)
{
}

void ImageDocumentRuntimePlanExecutor::shutdownRuntime()
{
    dispatchPlan(imageDocumentShutdownPlan());
}

void ImageDocumentRuntimePlanExecutor::dispatchPlan(const ImageDocumentRuntimePlan& plan)
{
    for (const ImageDocumentRuntimeOperation& operation : plan) {
        dispatchOperation(operation);
    }
}

void ImageDocumentRuntimePlanExecutor::dispatchOperation(
    const ImageDocumentRuntimeOperation& operation)
{
    if (m_lifecycleExecutor.dispatchOperation(operation)) {
        return;
    }
    if (m_mediaEntrySourceExecutor.dispatchOperation(operation)) {
        return;
    }
    if (m_sourceLoadExecutor.dispatchOperation(operation)) {
        return;
    }
    if (m_openExecutor.dispatchOperation(operation)) {
        return;
    }
    if (m_predecodeExecutor.dispatchOperation(operation)) {
        return;
    }
    if (m_spreadExecutor.dispatchOperation(operation)) {
        return;
    }
    if (m_navigationExecutor.dispatchOperation(operation)) {
        return;
    }

    std::visit(
        [](const auto& payload) {
            using Operation = std::decay_t<decltype(payload)>;
            if constexpr (isSourceLoadRuntimeOperation<Operation>
                || isOpenRuntimeOperation<Operation> || isPredecodeRuntimeOperation<Operation>
                || isNavigationRuntimeOperation<Operation> || isLifecycleRuntimeOperation<Operation>
                || isMediaEntrySourceRuntimeOperation<Operation>
                || isSpreadRuntimeOperation<Operation>) {
                // Delegated before the visitor; this branch keeps std::visit exhaustive.
            } else {
                static_assert(alwaysFalse<Operation>, "Unhandled image document runtime operation");
            }
        },
        operation);
}
}
