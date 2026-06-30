// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentdeletioncontroller.h"

#include "async/imagecallback.h"
#include "imagedocumentstate.h"
#include "localization/imageerrortext.h"
#include "location/imagedocumentlocation.h"
#include "presentation/imagepagesurfacecontroller.h"

#include <utility>

namespace {
QString genericFileDeletionErrorMessage()
{
    return kiriview::imageErrorText(kiriview::ImageErrorTextId::DeleteFile);
}

bool documentReadyForFileDeletion(const kiriview::ImageDocumentState& state)
{
    return state.status() == kiriview::ImageDocumentStatus::Ready;
}

bool displayedOpenedCollectionVideoHasDeletionTarget(const kiriview::ImageDocumentState& state)
{
    return state.sourceKind() == kiriview::ImageDocumentPageKind::Video
        && kiriview::displayedLocationIsInsideOpenedCollectionScope(state.displayedImageLocation());
}
}

namespace kiriview {
ImageDocumentDeletionController::ImageDocumentDeletionController(QObject* parent,
    ImageDocumentState& state, ImagePageSurfaceController& pageSurfaceController,
    ImageDocumentPageCandidateProvider candidateProvider, FileDeletionProvider fileDeletionProvider,
    Callbacks callbacks)
    : m_parent(parent)
    , m_state(state)
    , m_pageSurfaceController(pageSurfaceController)
    , m_callbacks(std::move(callbacks))
    , m_fileDeletionProvider(fileDeletionProviderWithDefault(std::move(fileDeletionProvider)))
    , m_fallbackController(m_parent, std::move(candidateProvider),
          [this](ImageDocumentRuntimePlan plan) { reportRuntimePlan(std::move(plan)); })
{
}

ImageDocumentDeletionController::~ImageDocumentDeletionController() = default;

bool ImageDocumentDeletionController::inProgress() const { return m_deletionState.inProgress(); }

void ImageDocumentDeletionController::deleteDisplayedFile(FileDeletionMode mode)
{
    if (!documentReadyForFileDeletion(m_state)) {
        return;
    }

    if (!m_pageSurfaceController.hasImage()
        && !displayedOpenedCollectionVideoHasDeletionTarget(m_state)) {
        return;
    }

    const ImageRemovalPlan removalPlan
        = imageRemovalPlanForDisplayedLocation(m_state.displayedImageLocation());
    if (!removalPlan.hasTarget()) {
        return;
    }

    m_fallbackController.cancel();
    m_fileDeletionJob.cancel();
    const ImageDocumentDeletionFileOperationStart operation = m_deletionState.startFileDeletion();
    notifyInProgressChangedIf(operation.inProgressChanged);
    m_fileDeletionJob
        = m_fileDeletionProvider(m_parent, FileDeletionRequest { removalPlan.targetUrl, mode },
            [this, operationId = operation.operationId, fallbackPlan = removalPlan.fallbackPlan](
                FileDeletionResult result, const KioOperationFailure& failure) {
                finishFileDeletion(operationId, fallbackPlan, result, failure);
            });
}

void ImageDocumentDeletionController::finishFileDeletion(quint64 operationId,
    const ImageRemovalFallbackPlan& fallbackPlan, FileDeletionResult result,
    const KioOperationFailure& failure)
{
    const ImageDocumentDeletionFileOperationFinish operation
        = m_deletionState.finishFileDeletion(operationId);
    if (!operation.accepted) {
        return;
    }

    notifyInProgressChangedIf(operation.inProgressChanged);

    switch (fileDeletionCompletionAction(result)) {
    case FileDeletionCompletionAction::ClearDeletedTargetAndOpenFallback:
        reportRuntimePlan(imageDocumentClearDeletedImagePlan());
        m_fallbackController.open(fallbackPlan);
        return;
    case FileDeletionCompletionAction::Ignore:
        return;
    case FileDeletionCompletionAction::ReportFailure:
        reportFailure(failure);
        return;
    }
}

void ImageDocumentDeletionController::notifyInProgressChangedIf(bool changed)
{
    if (changed) {
        invokeIfSet(m_callbacks.inProgressChanged);
    }
}

void ImageDocumentDeletionController::cancel()
{
    cancelFileDeletion();
    m_fallbackController.cancel();
}

void ImageDocumentDeletionController::cancelFileDeletion()
{
    const bool inProgressChanged = m_deletionState.cancelFileDeletion();
    m_fileDeletionJob.cancel();
    notifyInProgressChangedIf(inProgressChanged);
}

void ImageDocumentDeletionController::reportRuntimePlan(ImageDocumentRuntimePlan plan)
{
    invokeIfSet(m_callbacks.runtimePlan, std::move(plan));
}

void ImageDocumentDeletionController::reportFailure(const KioOperationFailure& failure)
{
    const QString message
        = failure.userMessage.isEmpty() ? genericFileDeletionErrorMessage() : failure.userMessage;
    invokeIfSet(m_callbacks.failed, message);
}
}
