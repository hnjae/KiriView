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

    const DisplayedImageLocation location = m_state.displayedImageLocation();
    const QUrl targetUrl = deletionTargetUrlForDisplayedLocation(location);
    if (targetUrl.isEmpty()) {
        return;
    }

    const ImageRemovalFallbackPlan fallbackPlan
        = imageRemovalFallbackPlanForDisplayedLocation(location);

    setInProgress(true);
    m_fileDeletionJob = m_fileOperationProvider(m_parent, FileDeletionRequest { targetUrl, mode },
        [this, fallbackPlan](FileDeletionResult result, const QString &errorString) {
            finishFileDeletion(fallbackPlan, result, errorString);
        });
}

void ImageDocumentDeletionController::finishFileDeletion(
    const ImageRemovalFallbackPlan &fallbackPlan, FileDeletionResult result,
    const QString &errorString)
{
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
    std::visit([this](const auto &plan) { openFallbackPlan(plan); }, fallbackPlan);
}

void ImageDocumentDeletionController::openFallbackPlan(const NoImageRemovalFallback &) { }

void ImageDocumentDeletionController::openFallbackPlan(const ImageRemovalFallback &fallback)
{
    m_fallbackJob = m_candidateRepository.loadImages(
        m_parent, fallback.imageContext,
        [this, fallback](std::vector<ImageNavigationCandidate> candidates) {
            const std::optional<QUrl> fallbackUrl
                = imageRemovalFallbackUrl(std::move(candidates), fallback);
            if (fallbackUrl.has_value()) {
                reportDocumentEffect(ImageDocumentEffect::openUrl(*fallbackUrl));
            }
        },
        [](const QString &) {});
}

void ImageDocumentDeletionController::openFallbackPlan(const ComicBookRemovalFallback &fallback)
{
    if (!fallback.candidateDirectoryUrl.isValid() || fallback.candidateDirectoryUrl.isEmpty()) {
        return;
    }

    m_fallbackJob = m_candidateRepository.loadContainers(
        m_parent, fallback.candidateDirectoryUrl,
        [this, fallback](std::vector<ContainerNavigationCandidate> candidates) {
            const ComicBookRemovalFallbackCandidates fallbackCandidates
                = comicBookRemovalFallbackCandidates(std::move(candidates), fallback);
            openComicBookFallbackCandidate(
                fallbackCandidates.preferred, fallbackCandidates.fallback);
        },
        [](const QString &) {});
}

void ImageDocumentDeletionController::openComicBookFallbackCandidate(
    const std::optional<ContainerNavigationCandidate> &candidate,
    const std::optional<ContainerNavigationCandidate> &fallbackCandidate)
{
    if (!candidate.has_value()) {
        if (fallbackCandidate.has_value()) {
            openComicBookFallbackCandidate(fallbackCandidate, std::nullopt);
        }
        return;
    }

    m_fallbackJob = m_candidateRepository.loadFirstImageInContainer(
        m_parent, *candidate,
        [this](const QUrl &imageUrl, const QUrl &containerUrl) {
            reportDocumentEffect(
                ImageDocumentEffect::containerImageSelected(imageUrl, containerUrl));
        },
        [this, fallbackCandidate](const QUrl &, ImageCandidateRepositoryError, const QString &) {
            openComicBookFallbackCandidate(fallbackCandidate, std::nullopt);
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
    m_fileDeletionJob.cancel();
    setInProgress(false);
}

void ImageDocumentDeletionController::cancelFallback() { m_fallbackJob.cancel(); }

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
