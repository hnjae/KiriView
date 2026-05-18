// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentdeletioncontroller.h"

#include "imagecallback.h"
#include "imagedocumentstate.h"
#include "imagepresentationcontroller.h"
#include "imageviewtext.h"

#include <utility>
#include <variant>
#include <vector>

namespace {
QString genericFileDeletionErrorMessage()
{
    return KiriView::imageViewText("Could not delete the selected file.");
}
}

namespace KiriView {
ImageDocumentDeletionController::ImageDocumentDeletionController(QObject *parent,
    ImageDocumentState &state, ImagePresentationController &presentationController,
    ImageNavigationCandidateProvider candidateProvider, FileOperationProvider fileOperationProvider,
    Callbacks callbacks)
    : m_parent(parent)
    , m_state(state)
    , m_presentationController(presentationController)
    , m_callbacks(std::move(callbacks))
    , m_candidateRepository(std::move(candidateProvider))
    , m_fileOperationProvider(fileOperationProviderWithDefault(std::move(fileOperationProvider)))
{
}

ImageDocumentDeletionController::~ImageDocumentDeletionController() = default;

bool ImageDocumentDeletionController::inProgress() const { return m_deletionState.inProgress(); }

void ImageDocumentDeletionController::deleteDisplayedFile(FileDeletionMode mode)
{
    if (!m_presentationController.hasImage()) {
        return;
    }

    const ImageRemovalPlan removalPlan
        = imageRemovalPlanForDisplayedLocation(m_state.displayedImageLocation());
    if (!removalPlan.hasTarget()) {
        return;
    }

    cancelFallback();
    m_fileDeletionJob.cancel();
    const ImageDocumentDeletionFileOperationStart operation = m_deletionState.startFileDeletion();
    notifyInProgressChangedIf(operation.inProgressChanged);
    m_fileDeletionJob
        = m_fileOperationProvider(m_parent, FileDeletionRequest { removalPlan.targetUrl, mode },
            [this, operationId = operation.operationId, fallbackPlan = removalPlan.fallbackPlan](
                FileDeletionResult result, const QString &errorString) {
                finishFileDeletion(operationId, fallbackPlan, result, errorString);
            });
}

void ImageDocumentDeletionController::finishFileDeletion(quint64 operationId,
    const ImageRemovalFallbackPlan &fallbackPlan, FileDeletionResult result,
    const QString &errorString)
{
    const ImageDocumentDeletionFileOperationFinish operation
        = m_deletionState.finishFileDeletion(operationId);
    if (!operation.accepted) {
        return;
    }

    notifyInProgressChangedIf(operation.inProgressChanged);

    switch (fileDeletionCompletionAction(result)) {
    case FileDeletionCompletionAction::ClearDeletedImageAndOpenFallback:
        reportDocumentEffect(ImageDocumentEffect::clearDeletedImage());
        openRemovalFallback(fallbackPlan);
        return;
    case FileDeletionCompletionAction::Ignore:
        return;
    case FileDeletionCompletionAction::ReportFailure:
        reportFailure(errorString);
        return;
    }
}

void ImageDocumentDeletionController::openRemovalFallback(
    const ImageRemovalFallbackPlan &fallbackPlan)
{
    cancelFallback();

    const quint64 operationId = m_deletionState.startFallback();
    std::visit([this, operationId](const auto &plan) { openFallbackPlan(operationId, plan); },
        fallbackPlan);
}

void ImageDocumentDeletionController::openFallbackPlan(
    quint64 operationId, const NoImageRemovalFallback &)
{
    m_deletionState.finishFallback(operationId);
}

void ImageDocumentDeletionController::openFallbackPlan(
    quint64 operationId, const ImageRemovalFallback &fallback)
{
    m_fallbackJob = m_candidateRepository.loadImages(
        m_parent, fallback.imageContext,
        [this, operationId, fallback](std::vector<ImageNavigationCandidate> candidates) {
            if (!m_deletionState.acceptsFallback(operationId)) {
                return;
            }

            const std::optional<QUrl> fallbackUrl
                = imageRemovalFallbackUrl(std::move(candidates), fallback);
            m_deletionState.finishFallback(operationId);
            if (fallbackUrl.has_value()) {
                reportDocumentEffect(ImageDocumentEffect::openUrl(*fallbackUrl));
            }
        },
        [this, operationId](const QString &) { m_deletionState.finishFallback(operationId); });
}

void ImageDocumentDeletionController::openFallbackPlan(
    quint64 operationId, const ComicBookRemovalFallback &fallback)
{
    if (!fallback.candidateDirectoryUrl.isValid() || fallback.candidateDirectoryUrl.isEmpty()) {
        m_deletionState.finishFallback(operationId);
        return;
    }

    m_fallbackJob = m_candidateRepository.loadContainers(
        m_parent, fallback.candidateDirectoryUrl,
        [this, operationId, fallback](std::vector<ContainerNavigationCandidate> candidates) {
            if (!m_deletionState.acceptsFallback(operationId)) {
                return;
            }

            const ComicBookRemovalFallbackCandidates fallbackCandidates
                = comicBookRemovalFallbackCandidates(std::move(candidates), fallback);
            openComicBookFallbackCandidate(
                operationId, fallbackCandidates.preferred, fallbackCandidates.fallback);
        },
        [this, operationId](const QString &) { m_deletionState.finishFallback(operationId); });
}

void ImageDocumentDeletionController::openComicBookFallbackCandidate(quint64 operationId,
    const std::optional<ContainerNavigationCandidate> &candidate,
    const std::optional<ContainerNavigationCandidate> &fallbackCandidate)
{
    if (!m_deletionState.acceptsFallback(operationId)) {
        return;
    }

    if (!candidate.has_value()) {
        if (fallbackCandidate.has_value()) {
            openComicBookFallbackCandidate(operationId, fallbackCandidate, std::nullopt);
            return;
        }
        m_deletionState.finishFallback(operationId);
        return;
    }

    m_fallbackJob = m_candidateRepository.loadFirstImageInContainer(
        m_parent, *candidate,
        [this, operationId](const QUrl &imageUrl, const QUrl &containerUrl) {
            if (!m_deletionState.acceptsFallback(operationId)) {
                return;
            }

            m_deletionState.finishFallback(operationId);
            reportDocumentEffect(
                ImageDocumentEffect::containerImageSelected(imageUrl, containerUrl));
        },
        [this, operationId, fallbackCandidate](
            const QUrl &, ImageCandidateRepositoryError, const QString &) {
            openComicBookFallbackCandidate(operationId, fallbackCandidate, std::nullopt);
        });
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
    cancelFallback();
}

void ImageDocumentDeletionController::cancelFileDeletion()
{
    const bool inProgressChanged = m_deletionState.cancelFileDeletion();
    m_fileDeletionJob.cancel();
    notifyInProgressChangedIf(inProgressChanged);
}

void ImageDocumentDeletionController::cancelFallback()
{
    m_deletionState.cancelFallback();
    m_fallbackJob.cancel();
}

void ImageDocumentDeletionController::reportDocumentEffect(ImageDocumentEffect effect)
{
    invokeIfSet(m_callbacks.effect, std::move(effect));
}

void ImageDocumentDeletionController::reportFailure(const QString &errorString)
{
    const QString message = errorString.isEmpty() ? genericFileDeletionErrorMessage() : errorString;
    invokeIfSet(m_callbacks.failed, message);
}
}
