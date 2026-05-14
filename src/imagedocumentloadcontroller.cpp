// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentloadcontroller.h"

#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
#include "imageopenworkflow.h"
#include "imagepresentationcontroller.h"
#include "imagesourceloadworkflow.h"
#include "imagespreadpresentationcontroller.h"

#include <QString>

namespace KiriView {
ImageDocumentLoadController::ImageDocumentLoadController(ImageDocumentState &state,
    ImageDocumentDeletionController &deletionController,
    ImageDocumentNavigationController &navigationController,
    ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
    ImagePresentationController &presentationController,
    ImageSpreadPresentationController &spreadController)
    : m_state(state)
    , m_deletionController(deletionController)
    , m_navigationController(navigationController)
    , m_predecodeController(predecodeController)
    , m_openController(openController)
    , m_presentationController(presentationController)
    , m_spreadController(spreadController)
{
}

void ImageDocumentLoadController::loadSource(const ImageDocumentSourceLoadRequest &request)
{
    m_deletionController.cancel();

    const ImageSourceLoadPlan plan
        = ImageSourceLoadWorkflow::plan(sourceLoadWorkflowRequest(request));
    applySourceLoadPlan(request, plan);
}

ImageSourceLoadRequest ImageDocumentLoadController::sourceLoadWorkflowRequest(
    const ImageDocumentSourceLoadRequest &request) const
{
    const bool sourceUrlChanged = m_state.sourceUrl() != request.sourceUrl;
    const bool resetRightToLeftReading = m_spreadController.shouldResetRightToLeftReadingForLoad(
        m_state.displayedArchiveDocument(), request.sourceUrl, request.containerNavigationUrl);

    ImageSourceLoadRequest sourceLoadRequest;
    sourceLoadRequest.sourceUrlChanged = sourceUrlChanged;
    sourceLoadRequest.preserveTwoPageSpreadTransition = request.preserveTwoPageSpreadTransition;
    sourceLoadRequest.resetRightToLeftReading = resetRightToLeftReading;
    sourceLoadRequest.rightToLeftReadingEnabled = m_spreadController.rightToLeftReadingEnabled();
    sourceLoadRequest.containerNavigationUrlEmpty = request.containerNavigationUrl.isEmpty();
    return sourceLoadRequest;
}

void ImageDocumentLoadController::applySourceLoadPlan(
    const ImageDocumentSourceLoadRequest &request, const ImageSourceLoadPlan &plan)
{
    for (ImageSourceLoadAction action : plan.actions) {
        applySourceLoadAction(request, action);
    }
}

void ImageDocumentLoadController::applySourceLoadAction(
    const ImageDocumentSourceLoadRequest &request, ImageSourceLoadAction action)
{
    switch (action) {
    case ImageSourceLoadAction::CancelNavigationAndPredecode:
        cancelNavigationAndPredecode();
        return;
    case ImageSourceLoadAction::FinishSpreadTransition:
        m_spreadController.finishTransition();
        return;
    case ImageSourceLoadAction::ResetRightToLeftReading:
        m_spreadController.resetRightToLeftReading();
        return;
    case ImageSourceLoadAction::NotifyRightToLeftReading:
        m_spreadController.notifyRightToLeftReadingChanged();
        return;
    case ImageSourceLoadAction::ClearSecondaryPage:
        m_spreadController.clearSecondaryPage();
        return;
    case ImageSourceLoadAction::ClearLoadingContainerNavigationUrl:
        m_state.clearLoadingContainerNavigationUrl();
        return;
    case ImageSourceLoadAction::UpdateContainerNavigationUrl:
        m_state.setContainerNavigationUrl(request.containerNavigationUrl);
        return;
    case ImageSourceLoadAction::SetLoadingContainerNavigationUrl:
        m_state.setLoadingContainerNavigationUrl(request.containerNavigationUrl);
        return;
    case ImageSourceLoadAction::SetSourceUrl:
        m_state.setSourceUrl(request.sourceUrl);
        return;
    case ImageSourceLoadAction::BeginOpen:
        m_openController.open();
        return;
    }
}

void ImageDocumentLoadController::clearImage()
{
    m_predecodeController.clear();
    m_spreadController.finishTransition();
    m_spreadController.clearSecondaryPage();
    m_navigationController.cancelPageNavigationUpdate();
    m_state.clearDisplayedImageLocation();
    m_presentationController.clearImage();
    m_navigationController.clearPageNavigation();
    m_spreadController.notifyRightToLeftReadingChanged();
}

ImageDocumentEffects ImageDocumentLoadController::clearAfterSuccessfulFileDeletion()
{
    cancelNavigationAndPredecode();
    m_openController.cancel();
    m_spreadController.finishTransition();
    m_spreadController.clearSecondaryPage();
    m_state.setSourceUrl(QUrl());
    m_state.setErrorString(QString());
    return ImageOpenWorkflow::finishEmptySourceLoad(m_state);
}

void ImageDocumentLoadController::cancelNavigationAndPredecode()
{
    m_navigationController.cancelNavigation();
    m_navigationController.cancelContainerNavigation();
    m_predecodeController.cancel();
}
}
