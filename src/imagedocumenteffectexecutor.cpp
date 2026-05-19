// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumenteffectexecutor.h"

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
    switch (operation.kind) {
    case ImageDocumentRuntimeOperationKind::CancelFileDeletion:
        run(m_operations.lifecycle.cancelFileDeletion);
        return;
    case ImageDocumentRuntimeOperationKind::StopPresentationAnimation:
        run(m_operations.lifecycle.stopPresentationAnimation);
        return;
    case ImageDocumentRuntimeOperationKind::ShutdownSpread:
        run(m_operations.lifecycle.shutdownSpread);
        return;
    case ImageDocumentRuntimeOperationKind::ClearArchiveSession:
        run(m_operations.archive.clearSession);
        return;
    case ImageDocumentRuntimeOperationKind::ClearPredecode:
        run(m_operations.predecode.clearPredecode);
        return;
    case ImageDocumentRuntimeOperationKind::CancelPredecode:
        run(m_operations.predecode.cancelPredecode);
        return;
    case ImageDocumentRuntimeOperationKind::ScheduleAdjacentImagePredecode:
        run(m_operations.predecode.scheduleAdjacentImagePredecode);
        return;
    case ImageDocumentRuntimeOperationKind::FinishSpreadTransition:
        run(m_operations.spread.finishSpreadTransition);
        return;
    case ImageDocumentRuntimeOperationKind::ClearSecondaryPage:
        run(m_operations.spread.clearSecondaryPage);
        return;
    case ImageDocumentRuntimeOperationKind::NotifyRightToLeftReadingChanged:
        run(m_operations.spread.notifyRightToLeftReadingChanged);
        return;
    case ImageDocumentRuntimeOperationKind::ResetZoom:
        run(m_operations.spread.resetZoom);
        return;
    case ImageDocumentRuntimeOperationKind::PrepareFailedContainer:
        run(m_operations.spread.prepareFailedContainer, operation.url);
        return;
    case ImageDocumentRuntimeOperationKind::CancelPageNavigationUpdate:
        run(m_operations.navigation.cancelPageNavigationUpdate);
        return;
    case ImageDocumentRuntimeOperationKind::CancelNavigation:
        run(m_operations.navigation.cancelNavigation);
        return;
    case ImageDocumentRuntimeOperationKind::CancelContainerNavigation:
        run(m_operations.navigation.cancelContainerNavigation);
        return;
    case ImageDocumentRuntimeOperationKind::ClearPageNavigation:
        run(m_operations.navigation.clearPageNavigation);
        return;
    case ImageDocumentRuntimeOperationKind::UpdatePageNavigation:
        run(m_operations.navigation.updatePageNavigation);
        return;
    case ImageDocumentRuntimeOperationKind::LoadUrl:
        run(m_operations.navigation.loadUrl, operation.url);
        return;
    case ImageDocumentRuntimeOperationKind::LoadContainerImage:
        run(m_operations.navigation.loadContainerImage, operation.url, operation.secondaryUrl);
        return;
    case ImageDocumentRuntimeOperationKind::FinishEmptyContainerNavigation:
        run(m_operations.navigation.finishEmptyContainerNavigation, operation.url);
        return;
    case ImageDocumentRuntimeOperationKind::FinishContainerNavigationLoadWithError:
        run(m_operations.navigation.finishContainerNavigationLoadWithError, operation.url,
            operation.errorString);
        return;
    case ImageDocumentRuntimeOperationKind::LoadPageNavigationUrl:
        run(m_operations.navigation.loadPageNavigationUrl, operation.url,
            operation.preserveTwoPageSpreadTransition);
        return;
    case ImageDocumentRuntimeOperationKind::CancelOpen:
        run(m_operations.open.cancelOpen);
        return;
    case ImageDocumentRuntimeOperationKind::ClearDisplayedImageLocation:
        run(m_operations.open.clearDisplayedImageLocation);
        return;
    case ImageDocumentRuntimeOperationKind::ClearPresentationImage:
        run(m_operations.open.clearPresentationImage);
        return;
    case ImageDocumentRuntimeOperationKind::SetSourceUrl:
        run(m_operations.open.setSourceUrl, operation.url);
        return;
    case ImageDocumentRuntimeOperationKind::SetErrorString:
        run(m_operations.open.setErrorString, operation.errorString);
        return;
    case ImageDocumentRuntimeOperationKind::FinishEmptySourceLoad:
        dispatchGeneratedEffects(generatedEffects(m_operations.open.finishEmptySourceLoad));
        return;
    }
}
}
