// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentloadcontroller.h"

#include "archivedocumentsessionstore.h"
#include "imagecontainer.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentsourceloadexecutor.h"
#include "imagedocumentsourceloadpolicy.h"
#include "imagedocumentstate.h"
#include "imagenavigationservice.h"
#include "imageopencontroller.h"
#include "imagespreadpresentationcontroller.h"

namespace {
KiriView::ImageDocumentSourceLoadKind sourceLoadKind(const KiriView::ImageDocumentState &state,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    if (state.sourceUrl() == request.sourceUrl) {
        return KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    }

    return KiriView::ImageDocumentSourceLoadKind::ReplacementSource;
}

bool sourceWithinDisplayedComicBookArchive(const KiriView::ImageDocumentState &state,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    const KiriView::ArchiveDocumentLocation displayedArchiveDocument
        = state.displayedArchiveDocument();
    return displayedArchiveDocument.isComicBook()
        && KiriView::archiveDocumentContainsUrl(displayedArchiveDocument, request.sourceUrl);
}

KiriView::ImageDocumentSourceLoadPolicyInput sourceLoadPolicyInput(
    const KiriView::ImageDocumentState &state,
    const KiriView::ImageSpreadPresentationController &spreadController,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    KiriView::ImageDocumentSourceLoadPolicyInput input;
    input.loadKind = sourceLoadKind(state, request);
    input.preserveTwoPageSpreadTransition = request.preserveTwoPageSpreadTransition;
    input.rightToLeftReadingEnabled = spreadController.rightToLeftReadingEnabled();
    input.sourceWithinDisplayedComicBookArchive
        = sourceWithinDisplayedComicBookArchive(state, request);
    input.hasRequestedContainerNavigationUrl = !request.containerNavigationUrl.isEmpty();
    return input;
}

}

namespace KiriView {
ImageDocumentLoadController::ImageDocumentLoadController(ImageDocumentState &state,
    ImageDocumentDeletionController &deletionController, ImageNavigationService &navigationService,
    ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
    ImageSpreadPresentationController &spreadController,
    ArchiveDocumentSessionStore *archiveSessionStore)
    : m_state(state)
    , m_deletionController(deletionController)
    , m_navigationService(navigationService)
    , m_predecodeController(predecodeController)
    , m_openController(openController)
    , m_spreadController(spreadController)
    , m_archiveSessionStore(archiveSessionStore)
{
}

void ImageDocumentLoadController::loadSource(const ImageDocumentSourceLoadRequest &request)
{
    m_deletionController.cancel();

    const ImageDocumentSourceLoadPlan plan = ImageDocumentSourceLoadPolicy::plan(
        sourceLoadPolicyInput(m_state, m_spreadController, request));
    executeImageDocumentSourceLoadPlan(request, plan, sourceLoadOperations());
}

ImageDocumentSourceLoadOperations ImageDocumentLoadController::sourceLoadOperations()
{
    ImageDocumentSourceLoadOperations operations;
    operations.cancelNavigationAndPredecode = [this]() {
        m_navigationService.cancelNavigation();
        m_navigationService.cancelContainerNavigation();
        m_predecodeController.cancel();
    };
    operations.finishSpreadTransition = [this]() { m_spreadController.finishTransition(); };
    operations.resetRightToLeftReading = [this]() { m_spreadController.resetRightToLeftReading(); };
    operations.notifyRightToLeftReadingChanged
        = [this]() { m_spreadController.notifyRightToLeftReadingChanged(); };
    operations.clearSecondaryPage = [this]() { m_spreadController.clearSecondaryPage(); };
    operations.clearLoadingContainerNavigationUrl
        = [this]() { m_state.clearLoadingContainerNavigationUrl(); };
    operations.setLoadingContainerNavigationUrl
        = [this](const QUrl &url) { m_state.setLoadingContainerNavigationUrl(url); };
    operations.setContainerNavigationUrl
        = [this](const QUrl &url) { m_state.setContainerNavigationUrl(url); };
    operations.prepareSourceLoad = [this](const ImageDocumentSourceLoadRequest &request) {
        if (m_archiveSessionStore != nullptr) {
            m_archiveSessionStore->prepareForSourceLoad(
                request, m_state.displayedArchiveDocument());
        }
    };
    operations.setSourceUrl = [this](const QUrl &url) { m_state.setSourceUrl(url); };
    operations.beginOpen = [this]() { m_openController.open(); };
    return operations;
}

}
