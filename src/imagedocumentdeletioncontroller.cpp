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

bool ImageDocumentDeletionController::inProgress() const { return m_inProgress; }

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
    const quint64 operationId = nextOperationId();
    m_fileDeletionOperationId = operationId;
    setInProgress(true);
    m_fileDeletionJob
        = m_fileOperationProvider(m_parent, FileDeletionRequest { removalPlan.targetUrl, mode },
            [this, operationId, fallbackPlan = removalPlan.fallbackPlan](
                FileDeletionResult result, const QString &errorString) {
                finishFileDeletion(operationId, fallbackPlan, result, errorString);
            });
}

void ImageDocumentDeletionController::finishFileDeletion(quint64 operationId,
    const ImageRemovalFallbackPlan &fallbackPlan, FileDeletionResult result,
    const QString &errorString)
{
    if (!currentFileDeletionOperation(operationId)) {
        return;
    }

    clearFileDeletionOperation(operationId);
    setInProgress(false);

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

    const quint64 operationId = nextOperationId();
    m_fallbackOperationId = operationId;
    std::visit([this, operationId](const auto &plan) { openFallbackPlan(operationId, plan); },
        fallbackPlan);
}

void ImageDocumentDeletionController::openFallbackPlan(
    quint64 operationId, const NoImageRemovalFallback &)
{
    clearFallbackOperation(operationId);
}

void ImageDocumentDeletionController::openFallbackPlan(
    quint64 operationId, const ImageRemovalFallback &fallback)
{
    m_fallbackJob = m_candidateRepository.loadImages(
        m_parent, fallback.imageContext,
        [this, operationId, fallback](std::vector<ImageNavigationCandidate> candidates) {
            if (!currentFallbackOperation(operationId)) {
                return;
            }

            const std::optional<QUrl> fallbackUrl
                = imageRemovalFallbackUrl(std::move(candidates), fallback);
            clearFallbackOperation(operationId);
            if (fallbackUrl.has_value()) {
                reportDocumentEffect(ImageDocumentEffect::openUrl(*fallbackUrl));
            }
        },
        [this, operationId](const QString &) { clearFallbackOperation(operationId); });
}

void ImageDocumentDeletionController::openFallbackPlan(
    quint64 operationId, const ComicBookRemovalFallback &fallback)
{
    if (!fallback.candidateDirectoryUrl.isValid() || fallback.candidateDirectoryUrl.isEmpty()) {
        clearFallbackOperation(operationId);
        return;
    }

    m_fallbackJob = m_candidateRepository.loadContainers(
        m_parent, fallback.candidateDirectoryUrl,
        [this, operationId, fallback](std::vector<ContainerNavigationCandidate> candidates) {
            if (!currentFallbackOperation(operationId)) {
                return;
            }

            const ComicBookRemovalFallbackCandidates fallbackCandidates
                = comicBookRemovalFallbackCandidates(std::move(candidates), fallback);
            openComicBookFallbackCandidate(
                operationId, fallbackCandidates.preferred, fallbackCandidates.fallback);
        },
        [this, operationId](const QString &) { clearFallbackOperation(operationId); });
}

void ImageDocumentDeletionController::openComicBookFallbackCandidate(quint64 operationId,
    const std::optional<ContainerNavigationCandidate> &candidate,
    const std::optional<ContainerNavigationCandidate> &fallbackCandidate)
{
    if (!currentFallbackOperation(operationId)) {
        return;
    }

    if (!candidate.has_value()) {
        if (fallbackCandidate.has_value()) {
            openComicBookFallbackCandidate(operationId, fallbackCandidate, std::nullopt);
            return;
        }
        clearFallbackOperation(operationId);
        return;
    }

    m_fallbackJob = m_candidateRepository.loadFirstImageInContainer(
        m_parent, *candidate,
        [this, operationId](const QUrl &imageUrl, const QUrl &containerUrl) {
            if (!currentFallbackOperation(operationId)) {
                return;
            }

            clearFallbackOperation(operationId);
            reportDocumentEffect(
                ImageDocumentEffect::containerImageSelected(imageUrl, containerUrl));
        },
        [this, operationId, fallbackCandidate](
            const QUrl &, ImageCandidateRepositoryError, const QString &) {
            openComicBookFallbackCandidate(operationId, fallbackCandidate, std::nullopt);
        });
}

void ImageDocumentDeletionController::setInProgress(bool inProgress)
{
    if (m_inProgress == inProgress) {
        return;
    }

    m_inProgress = inProgress;
    invokeIfSet(m_callbacks.inProgressChanged);
}

void ImageDocumentDeletionController::cancel()
{
    cancelFileDeletion();
    cancelFallback();
}

void ImageDocumentDeletionController::cancelFileDeletion()
{
    m_fileDeletionOperationId = 0;
    m_fileDeletionJob.cancel();
    setInProgress(false);
}

void ImageDocumentDeletionController::cancelFallback()
{
    m_fallbackOperationId = 0;
    m_fallbackJob.cancel();
}

quint64 ImageDocumentDeletionController::nextOperationId()
{
    ++m_nextOperationId;
    if (m_nextOperationId == 0) {
        ++m_nextOperationId;
    }
    return m_nextOperationId;
}

bool ImageDocumentDeletionController::currentFileDeletionOperation(quint64 operationId) const
{
    return operationId != 0 && operationId == m_fileDeletionOperationId;
}

bool ImageDocumentDeletionController::currentFallbackOperation(quint64 operationId) const
{
    return operationId != 0 && operationId == m_fallbackOperationId;
}

void ImageDocumentDeletionController::clearFileDeletionOperation(quint64 operationId)
{
    if (currentFileDeletionOperation(operationId)) {
        m_fileDeletionOperationId = 0;
    }
}

void ImageDocumentDeletionController::clearFallbackOperation(quint64 operationId)
{
    if (currentFallbackOperation(operationId)) {
        m_fallbackOperationId = 0;
    }
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
