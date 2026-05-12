// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumenteffectexecutor.h"

#include "imagedocumentloadcontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
#include "imagepresentationcontroller.h"
#include "imagespreadpresentationcontroller.h"

#include <variant>

namespace KiriView {
ImageDocumentEffectExecutor::ImageDocumentEffectExecutor(ImageDocumentState &state,
    ImageDocumentNavigationController &navigationController,
    ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
    ImagePresentationController &presentationController,
    ImageSpreadPresentationController &spreadController,
    ImageDocumentLoadController &loadController)
    : m_state(state)
    , m_navigationController(navigationController)
    , m_predecodeController(predecodeController)
    , m_openController(openController)
    , m_presentationController(presentationController)
    , m_spreadController(spreadController)
    , m_loadController(loadController)
{
}

void ImageDocumentEffectExecutor::dispatch(ImageDocumentEffect effect)
{
    std::visit([this](const auto &payload) { dispatchPayload(payload); }, effect.payload);
}

void ImageDocumentEffectExecutor::dispatchAll(ImageDocumentEffects effects)
{
    for (ImageDocumentEffect &effect : effects) {
        dispatch(std::move(effect));
    }
}

void ImageDocumentEffectExecutor::dispatchPayload(const ClearImageEffect &)
{
    m_predecodeController.clear();
    m_spreadController.finishTransition();
    m_spreadController.clearSecondaryPage();
    m_navigationController.cancelPageNavigationUpdate();
    m_state.clearDisplayedImageUrls();
    m_presentationController.clearImage();
    m_navigationController.clearPageNavigation();
    m_spreadController.notifyRightToLeftReadingChanged();
}

void ImageDocumentEffectExecutor::dispatchPayload(const ResetZoomEffect &)
{
    m_spreadController.resetZoom();
}

void ImageDocumentEffectExecutor::dispatchPayload(const UpdatePageNavigationEffect &)
{
    m_navigationController.updatePageNavigation();
}

void ImageDocumentEffectExecutor::dispatchPayload(const ScheduleAdjacentImagePredecodeEffect &)
{
    m_predecodeController.scheduleAdjacentImagePredecode();
}

void ImageDocumentEffectExecutor::dispatchPayload(const OpenUrlEffect &payload)
{
    m_loadController.setSourceUrl(payload.url);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ContainerImageSelectedEffect &payload)
{
    m_loadController.setSourceUrl(payload.imageUrl, payload.containerUrl);
}

void ImageDocumentEffectExecutor::dispatchPayload(const EmptyContainerSelectedEffect &payload)
{
    m_openController.finishContainerNavigationWithEmptyContainer(payload.containerUrl);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ContainerNavigationFailedEffect &payload)
{
    m_openController.finishContainerNavigationLoadWithError(
        payload.containerUrl, payload.errorString);
}

void ImageDocumentEffectExecutor::dispatchPayload(const PrepareFailedContainerEffect &payload)
{
    m_presentationController.prepareFailedContainer(payload.containerUrl);
}
}
