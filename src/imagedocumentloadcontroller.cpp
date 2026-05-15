// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentloadcontroller.h"

#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentloadpolicy.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
#include "imageopenworkflow.h"
#include "imagepresentationcontroller.h"
#include "imagespreadpresentationcontroller.h"

#include <QString>

namespace {
KiriView::ImageDocumentSourceLoadKind sourceLoadKind(const KiriView::ImageDocumentState &state,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    if (state.sourceUrl() == request.sourceUrl) {
        return KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    }

    return KiriView::ImageDocumentSourceLoadKind::ReplacementSource;
}

KiriView::ImageDocumentSourceLoadPolicyInput sourceLoadPolicyInput(
    const KiriView::ImageDocumentState &state,
    const KiriView::ImageSpreadPresentationController &spreadController,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    KiriView::ImageDocumentSourceLoadPolicyInput input;
    input.loadKind = sourceLoadKind(state, request);
    input.preserveTwoPageSpreadTransition = request.preserveTwoPageSpreadTransition;
    input.resetRightToLeftReading = spreadController.shouldResetRightToLeftReadingForLoad(
        state.displayedArchiveDocument(), request.sourceUrl, request.containerNavigationUrl);
    input.rightToLeftReadingWasEnabled = spreadController.rightToLeftReadingEnabled();
    input.hasRequestedContainerNavigationUrl = !request.containerNavigationUrl.isEmpty();
    return input;
}

void cancelNavigationAndPredecode(KiriView::ImageDocumentNavigationController &navigationController,
    KiriView::ImageDocumentPredecodeController &predecodeController)
{
    navigationController.cancelNavigation();
    navigationController.cancelContainerNavigation();
    predecodeController.cancel();
}
}

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

    const ImageDocumentSourceLoadPlan plan = ImageDocumentLoadPolicy::sourceLoadPlan(
        ::sourceLoadPolicyInput(m_state, m_spreadController, request));
    applySourceLoadPlan(request, plan);
}

void ImageDocumentLoadController::applySourceLoadPlan(
    const ImageDocumentSourceLoadRequest &request, const ImageDocumentSourceLoadPlan &plan)
{
    for (ImageDocumentSourceLoadAction action : plan.actions) {
        applySourceLoadAction(request, action);
    }
}

void ImageDocumentLoadController::applySourceLoadAction(
    const ImageDocumentSourceLoadRequest &request, ImageDocumentSourceLoadAction action)
{
    switch (action) {
    case ImageDocumentSourceLoadAction::CancelNavigationAndPredecode:
        ::cancelNavigationAndPredecode(m_navigationController, m_predecodeController);
        return;
    case ImageDocumentSourceLoadAction::FinishSpreadTransition:
        m_spreadController.finishTransition();
        return;
    case ImageDocumentSourceLoadAction::ResetRightToLeftReading:
        m_spreadController.resetRightToLeftReading();
        return;
    case ImageDocumentSourceLoadAction::NotifyRightToLeftReading:
        m_spreadController.notifyRightToLeftReadingChanged();
        return;
    case ImageDocumentSourceLoadAction::ClearSecondaryPage:
        m_spreadController.clearSecondaryPage();
        return;
    case ImageDocumentSourceLoadAction::ClearLoadingContainerNavigationUrl:
        m_state.clearLoadingContainerNavigationUrl();
        return;
    case ImageDocumentSourceLoadAction::UpdateContainerNavigationUrl:
        m_state.setContainerNavigationUrl(request.containerNavigationUrl);
        return;
    case ImageDocumentSourceLoadAction::SetLoadingContainerNavigationUrl:
        m_state.setLoadingContainerNavigationUrl(request.containerNavigationUrl);
        return;
    case ImageDocumentSourceLoadAction::SetSourceUrl:
        m_state.setSourceUrl(request.sourceUrl);
        return;
    case ImageDocumentSourceLoadAction::BeginOpen:
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
    ::cancelNavigationAndPredecode(m_navigationController, m_predecodeController);
    m_openController.cancel();
    m_spreadController.finishTransition();
    m_spreadController.clearSecondaryPage();
    m_state.setSourceUrl(QUrl());
    m_state.setErrorString(QString());
    return ImageOpenWorkflow::finishEmptySourceLoad(m_state);
}

}
