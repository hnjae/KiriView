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

    const bool sourceUrlChanged = m_state.sourceUrl() != request.sourceUrl;
    const bool resetRightToLeftReading = m_spreadController.shouldResetRightToLeftReadingForLoad(
        m_state.displayedArchiveDocument(), request.sourceUrl, request.containerNavigationUrl);
    ImageOpenSourceLoadRequest openRequest;
    openRequest.sourceUrlChanged = sourceUrlChanged;
    openRequest.preserveTwoPageSpreadTransition = request.preserveTwoPageSpreadTransition;
    openRequest.resetRightToLeftReading = resetRightToLeftReading;
    openRequest.rightToLeftReadingEnabled = m_spreadController.rightToLeftReadingEnabled();
    openRequest.containerNavigationUrlEmpty = request.containerNavigationUrl.isEmpty();
    const ImageOpenSourceLoadPlan plan = ImageOpenWorkflow::sourceLoadPlan(openRequest);
    if (plan.cancelNavigationAndPredecode) {
        cancelNavigationAndPredecode();
    }
    if (plan.finishSpreadTransition) {
        m_spreadController.finishTransition();
    }

    if (plan.resetRightToLeftReading) {
        m_spreadController.resetRightToLeftReading();
    }
    if (plan.notifyRightToLeftReadingBeforeOpen) {
        m_spreadController.notifyRightToLeftReadingChanged();
    }
    if (plan.clearSecondaryPage) {
        m_spreadController.clearSecondaryPage();
    }
    if (plan.clearLoadingContainerNavigationUrl) {
        m_state.clearLoadingContainerNavigationUrl();
    }
    if (plan.updateContainerNavigationUrl) {
        m_state.setContainerNavigationUrl(request.containerNavigationUrl);
    }
    if (plan.setLoadingContainerNavigationUrl) {
        m_state.setLoadingContainerNavigationUrl(request.containerNavigationUrl);
    }
    if (plan.setSourceUrl) {
        m_state.setSourceUrl(request.sourceUrl);
    }
    if (plan.beginOpen) {
        m_openController.open();
    }
    if (plan.notifyRightToLeftReadingAfterOpen) {
        m_spreadController.notifyRightToLeftReadingChanged();
    }
}

void ImageDocumentLoadController::clearImage()
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
