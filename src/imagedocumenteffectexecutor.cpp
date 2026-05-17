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

void ImageDocumentEffectExecutor::dispatchGeneratedEffects(ImageDocumentEffects effects)
{
    for (ImageDocumentEffect &effect : effects) {
        dispatch(std::move(effect));
    }
}

void ImageDocumentEffectExecutor::clearImage()
{
    run(m_operations.clearArchiveSession);
    run(m_operations.clearPredecode);
    run(m_operations.finishSpreadTransition);
    run(m_operations.clearSecondaryPage);
    run(m_operations.cancelPageNavigationUpdate);
    run(m_operations.clearDisplayedImageLocation);
    run(m_operations.clearPresentationImage);
    run(m_operations.clearPageNavigation);
    run(m_operations.notifyRightToLeftReadingChanged);
}

ImageDocumentEffects ImageDocumentEffectExecutor::clearAfterSuccessfulFileDeletion()
{
    run(m_operations.clearArchiveSession);
    run(m_operations.cancelNavigation);
    run(m_operations.cancelContainerNavigation);
    run(m_operations.cancelPredecode);
    run(m_operations.cancelOpen);
    run(m_operations.finishSpreadTransition);
    run(m_operations.clearSecondaryPage);
    run(m_operations.setSourceUrl, QUrl());
    run(m_operations.setErrorString, QString());
    return generatedEffects(m_operations.finishEmptySourceLoad);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ClearImageEffect &) { clearImage(); }

void ImageDocumentEffectExecutor::dispatchPayload(const ClearDeletedImageEffect &)
{
    dispatchGeneratedEffects(clearAfterSuccessfulFileDeletion());
}

void ImageDocumentEffectExecutor::dispatchPayload(const ResetZoomEffect &)
{
    run(m_operations.resetZoom);
}

void ImageDocumentEffectExecutor::dispatchPayload(const UpdatePageNavigationEffect &)
{
    run(m_operations.updatePageNavigation);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ScheduleAdjacentImagePredecodeEffect &)
{
    run(m_operations.scheduleAdjacentImagePredecode);
}

void ImageDocumentEffectExecutor::dispatchPayload(const OpenUrlEffect &payload)
{
    run(m_operations.loadUrl, payload.url);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ContainerImageSelectedEffect &payload)
{
    run(m_operations.loadContainerImage, payload.imageUrl, payload.containerUrl);
}

void ImageDocumentEffectExecutor::dispatchPayload(const EmptyContainerSelectedEffect &payload)
{
    run(m_operations.finishEmptyContainerNavigation, payload.containerUrl);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ContainerNavigationFailedEffect &payload)
{
    run(m_operations.finishContainerNavigationLoadWithError, payload.containerUrl,
        payload.errorString);
}

void ImageDocumentEffectExecutor::dispatchPayload(const PageNavigationSelectedEffect &payload)
{
    run(m_operations.loadPageNavigationUrl, payload.url, payload.preserveTwoPageSpreadTransition);
}

void ImageDocumentEffectExecutor::dispatchPayload(const PrepareFailedContainerEffect &payload)
{
    run(m_operations.prepareFailedContainer, payload.containerUrl);
}
}
