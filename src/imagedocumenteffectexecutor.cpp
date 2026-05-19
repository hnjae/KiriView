// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumenteffectexecutor.h"

#include <type_traits>
#include <utility>

namespace KiriView {
namespace {
    template <typename Operation, typename... Args>
    void run(const Operation &operation, Args &&...args)
    {
        if (operation) {
            operation(std::forward<Args>(args)...);
        }
    }

    ImageDocumentEffects generatedEffects(const std::function<ImageDocumentEffects()> &operation)
    {
        return operation ? operation() : ImageDocumentEffects {};
    }
}

ImageDocumentEffectExecutor::ImageDocumentEffectExecutor(ImageDocumentEffectOperations operations)
    : m_operations(std::move(operations))
{
}

void ImageDocumentEffectExecutor::dispatch(ImageDocumentEffect effect)
{
    dispatchPlan(imageDocumentEffectPlan(effect));
}

void ImageDocumentEffectExecutor::shutdownRuntime() { dispatchPlan(imageDocumentShutdownPlan()); }

void ImageDocumentEffectExecutor::dispatchPlan(const ImageDocumentRuntimePlan &plan)
{
    for (const ImageDocumentRuntimeOperation &operation : plan) {
        dispatchOperation(operation);
    }
}

void ImageDocumentEffectExecutor::dispatchGeneratedEffects(ImageDocumentEffects effects)
{
    for (ImageDocumentEffect &effect : effects) {
        dispatch(std::move(effect));
    }
}

void ImageDocumentEffectExecutor::dispatchOperation(const ImageDocumentRuntimeOperation &operation)
{
    std::visit(
        [this](const auto &payload) {
            using Operation = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<Operation, CancelFileDeletionOperation>) {
                run(m_operations.lifecycle.cancelFileDeletion);
            } else if constexpr (std::is_same_v<Operation, StopPresentationAnimationOperation>) {
                run(m_operations.lifecycle.stopPresentationAnimation);
            } else if constexpr (std::is_same_v<Operation, ShutdownSpreadOperation>) {
                run(m_operations.lifecycle.shutdownSpread);
            } else if constexpr (std::is_same_v<Operation, ClearArchiveSessionOperation>) {
                run(m_operations.archive.clearSession);
            } else if constexpr (std::is_same_v<Operation, ClearPredecodeOperation>) {
                run(m_operations.predecode.clearPredecode);
            } else if constexpr (std::is_same_v<Operation, CancelPredecodeOperation>) {
                run(m_operations.predecode.cancelPredecode);
            } else if constexpr (std::is_same_v<Operation,
                                     ScheduleAdjacentImagePredecodeOperation>) {
                run(m_operations.predecode.scheduleAdjacentImagePredecode);
            } else if constexpr (std::is_same_v<Operation, FinishSpreadTransitionOperation>) {
                run(m_operations.spread.finishSpreadTransition);
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
            } else if constexpr (std::is_same_v<Operation, ClearPageNavigationOperation>) {
                run(m_operations.navigation.clearPageNavigation);
            } else if constexpr (std::is_same_v<Operation, UpdatePageNavigationOperation>) {
                run(m_operations.navigation.updatePageNavigation);
            } else if constexpr (std::is_same_v<Operation, LoadUrlOperation>) {
                run(m_operations.navigation.loadUrl, payload.url);
            } else if constexpr (std::is_same_v<Operation, LoadContainerImageOperation>) {
                run(m_operations.navigation.loadContainerImage, payload.imageUrl,
                    payload.containerUrl);
            } else if constexpr (std::is_same_v<Operation,
                                     FinishEmptyContainerNavigationOperation>) {
                run(m_operations.navigation.finishEmptyContainerNavigation, payload.containerUrl);
            } else if constexpr (std::is_same_v<Operation,
                                     FinishContainerNavigationLoadWithErrorOperation>) {
                run(m_operations.navigation.finishContainerNavigationLoadWithError,
                    payload.containerUrl, payload.errorString);
            } else if constexpr (std::is_same_v<Operation, LoadPageNavigationUrlOperation>) {
                run(m_operations.navigation.loadPageNavigationUrl, payload.url,
                    payload.preserveTwoPageSpreadTransition);
            } else if constexpr (std::is_same_v<Operation, CancelOpenOperation>) {
                run(m_operations.open.cancelOpen);
            } else if constexpr (std::is_same_v<Operation, ClearDisplayedImageLocationOperation>) {
                run(m_operations.open.clearDisplayedImageLocation);
            } else if constexpr (std::is_same_v<Operation, ClearPresentationImageOperation>) {
                run(m_operations.open.clearPresentationImage);
            } else if constexpr (std::is_same_v<Operation, SetSourceUrlOperation>) {
                run(m_operations.open.setSourceUrl, payload.url);
            } else if constexpr (std::is_same_v<Operation, SetErrorStringOperation>) {
                run(m_operations.open.setErrorString, payload.errorString);
            } else {
                dispatchGeneratedEffects(generatedEffects(m_operations.open.finishEmptySourceLoad));
            }
        },
        operation);
}
}
