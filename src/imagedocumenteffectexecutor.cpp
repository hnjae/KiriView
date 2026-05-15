// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumenteffectexecutor.h"

#include "archivedocumentsessionstore.h"
#include "imagedocumentloadcontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentsourceloadrequest.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
#include "imageopenworkflow.h"
#include "imagepresentationcontroller.h"
#include "imagespreadpresentationcontroller.h"

#include <QString>
#include <QUrl>
#include <utility>
#include <variant>

namespace KiriView {
ImageDocumentEffectExecutor::ImageDocumentEffectExecutor(ImageDocumentState &state,
    ImageDocumentNavigationController &navigationController,
    ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
    ImagePresentationController &presentationController,
    ImageSpreadPresentationController &spreadController,
    ImageDocumentLoadController &loadController, ArchiveDocumentSessionStore *archiveSessionStore)
    : m_state(state)
    , m_navigationController(navigationController)
    , m_predecodeController(predecodeController)
    , m_openController(openController)
    , m_presentationController(presentationController)
    , m_spreadController(spreadController)
    , m_loadController(loadController)
    , m_archiveSessionStore(archiveSessionStore)
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
    if (m_archiveSessionStore != nullptr) {
        m_archiveSessionStore->clear();
    }
    m_predecodeController.clear();
    m_spreadController.finishTransition();
    m_spreadController.clearSecondaryPage();
    m_navigationController.cancelPageNavigationUpdate();
    m_state.clearDisplayedImageLocation();
    m_presentationController.clearImage();
    m_navigationController.clearPageNavigation();
    m_spreadController.notifyRightToLeftReadingChanged();
}

ImageDocumentEffects ImageDocumentEffectExecutor::clearAfterSuccessfulFileDeletion()
{
    if (m_archiveSessionStore != nullptr) {
        m_archiveSessionStore->clear();
    }
    m_navigationController.cancelNavigation();
    m_navigationController.cancelContainerNavigation();
    m_predecodeController.cancel();
    m_openController.cancel();
    m_spreadController.finishTransition();
    m_spreadController.clearSecondaryPage();
    m_state.setSourceUrl(QUrl());
    m_state.setErrorString(QString());
    return ImageOpenWorkflow::finishEmptySourceLoad(m_state);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ClearImageEffect &) { clearImage(); }

void ImageDocumentEffectExecutor::dispatchPayload(const ClearDeletedImageEffect &)
{
    dispatchGeneratedEffects(clearAfterSuccessfulFileDeletion());
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
    m_loadController.loadSource(ImageDocumentSourceLoadRequest::fromUrl(payload.url));
}

void ImageDocumentEffectExecutor::dispatchPayload(const ContainerImageSelectedEffect &payload)
{
    m_loadController.loadSource(
        ImageDocumentSourceLoadRequest::fromContainerImage(payload.imageUrl, payload.containerUrl));
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
