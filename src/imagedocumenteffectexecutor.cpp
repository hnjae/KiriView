// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumenteffectexecutor.h"

#include <QString>
#include <QUrl>
#include <utility>
#include <variant>

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
    std::visit([this](const auto &payload) { dispatchPayload(payload); }, effect.payload);
}

void ImageDocumentEffectExecutor::shutdownRuntime()
{
    run(m_operations.lifecycle.cancelFileDeletion);
    run(m_operations.lifecycle.stopPresentationAnimation);
    run(m_operations.lifecycle.shutdownSpread);
    run(m_operations.predecode.cancelPredecode);
    run(m_operations.navigation.cancelPageNavigationUpdate);
    run(m_operations.navigation.cancelContainerNavigation);
    run(m_operations.navigation.cancelNavigation);
    run(m_operations.open.cancelOpen);
    run(m_operations.archive.clearSession);
}

void ImageDocumentEffectExecutor::dispatchGeneratedEffects(ImageDocumentEffects effects)
{
    for (ImageDocumentEffect &effect : effects) {
        dispatch(std::move(effect));
    }
}

void ImageDocumentEffectExecutor::clearImage()
{
    run(m_operations.archive.clearSession);
    run(m_operations.predecode.clearPredecode);
    run(m_operations.spread.finishSpreadTransition);
    run(m_operations.spread.clearSecondaryPage);
    run(m_operations.navigation.cancelPageNavigationUpdate);
    run(m_operations.open.clearDisplayedImageLocation);
    run(m_operations.open.clearPresentationImage);
    run(m_operations.navigation.clearPageNavigation);
    run(m_operations.spread.notifyRightToLeftReadingChanged);
}

ImageDocumentEffects ImageDocumentEffectExecutor::clearAfterSuccessfulFileDeletion()
{
    run(m_operations.archive.clearSession);
    run(m_operations.navigation.cancelNavigation);
    run(m_operations.navigation.cancelContainerNavigation);
    run(m_operations.predecode.cancelPredecode);
    run(m_operations.open.cancelOpen);
    run(m_operations.spread.finishSpreadTransition);
    run(m_operations.spread.clearSecondaryPage);
    run(m_operations.open.setSourceUrl, QUrl());
    run(m_operations.open.setErrorString, QString());
    return generatedEffects(m_operations.open.finishEmptySourceLoad);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ClearImageEffect &) { clearImage(); }

void ImageDocumentEffectExecutor::dispatchPayload(const ClearDeletedImageEffect &)
{
    dispatchGeneratedEffects(clearAfterSuccessfulFileDeletion());
}

void ImageDocumentEffectExecutor::dispatchPayload(const ResetZoomEffect &)
{
    run(m_operations.spread.resetZoom);
}

void ImageDocumentEffectExecutor::dispatchPayload(const UpdatePageNavigationEffect &)
{
    run(m_operations.navigation.updatePageNavigation);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ScheduleAdjacentImagePredecodeEffect &)
{
    run(m_operations.predecode.scheduleAdjacentImagePredecode);
}

void ImageDocumentEffectExecutor::dispatchPayload(const OpenUrlEffect &payload)
{
    run(m_operations.navigation.loadUrl, payload.url);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ContainerImageSelectedEffect &payload)
{
    run(m_operations.navigation.loadContainerImage, payload.imageUrl, payload.containerUrl);
}

void ImageDocumentEffectExecutor::dispatchPayload(const EmptyContainerSelectedEffect &payload)
{
    run(m_operations.navigation.finishEmptyContainerNavigation, payload.containerUrl);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ContainerNavigationFailedEffect &payload)
{
    run(m_operations.navigation.finishContainerNavigationLoadWithError, payload.containerUrl,
        payload.errorString);
}

void ImageDocumentEffectExecutor::dispatchPayload(const PageNavigationSelectedEffect &payload)
{
    run(m_operations.navigation.loadPageNavigationUrl, payload.url,
        payload.preserveTwoPageSpreadTransition);
}

void ImageDocumentEffectExecutor::dispatchPayload(const PrepareFailedContainerEffect &payload)
{
    run(m_operations.spread.prepareFailedContainer, payload.containerUrl);
}
}
