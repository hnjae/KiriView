// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedeletioncontroller.h"

#include "imagecallback.h"
#include "imageremovalfallback.h"
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
ImageDeletionController::ImageDeletionController(QObject *parent,
    ImageNavigationCandidateProvider candidateProvider, FileOperationProvider fileOperationProvider,
    Callbacks callbacks)
    : QObject(parent)
    , m_callbacks(std::move(callbacks))
    , m_candidateRepository(std::move(candidateProvider))
    , m_fileOperationProvider(fileOperationProviderWithDefault(std::move(fileOperationProvider)))
{
}

ImageDeletionController::~ImageDeletionController() { cancel(); }

bool ImageDeletionController::inProgress() const { return m_inProgress; }

void ImageDeletionController::deleteDisplayedFile(
    const DisplayedImageLocation &location, FileDeletionMode mode)
{
    if (m_inProgress) {
        return;
    }

    const QUrl targetUrl = deletionTargetUrlForDisplayedLocation(location);
    if (targetUrl.isEmpty()) {
        return;
    }

    const ImageRemovalFallbackPlan fallbackPlan
        = imageRemovalFallbackPlanForDisplayedLocation(location);

    setInProgress(true);
    m_fileDeletionJob = m_fileOperationProvider(this, FileDeletionRequest { targetUrl, mode },
        [this, fallbackPlan](FileDeletionResult result, const QString &errorString) {
            finishFileDeletion(fallbackPlan, result, errorString);
        });
}

void ImageDeletionController::finishFileDeletion(const ImageRemovalFallbackPlan &fallbackPlan,
    FileDeletionResult result, const QString &errorString)
{
    setInProgress(false);

    switch (fileDeletionCompletionAction(result)) {
    case FileDeletionCompletionAction::ClearDeletedImageAndOpenFallback:
        report(ImageDeletionEffect::clearDeletedImage());
        openRemovalFallback(fallbackPlan);
        return;
    case FileDeletionCompletionAction::Ignore:
        return;
    case FileDeletionCompletionAction::ReportFailure:
        reportFailure(errorString);
        return;
    }
}

void ImageDeletionController::openRemovalFallback(const ImageRemovalFallbackPlan &fallbackPlan)
{
    std::visit([this](const auto &plan) { openFallbackPlan(plan); }, fallbackPlan);
}

void ImageDeletionController::openFallbackPlan(const NoImageRemovalFallback &) { }

void ImageDeletionController::openFallbackPlan(const ImageRemovalFallback &fallback)
{
    m_fallbackJob = m_candidateRepository.loadImages(
        this, fallback.imageContext,
        [this, fallback](std::vector<ImageNavigationCandidate> candidates) {
            const std::optional<QUrl> fallbackUrl
                = imageRemovalFallbackUrl(std::move(candidates), fallback);
            if (fallbackUrl.has_value()) {
                report(ImageDeletionEffect::openImageFallback(*fallbackUrl));
            }
        },
        [](const QString &) {});
}

void ImageDeletionController::openFallbackPlan(const ComicBookRemovalFallback &fallback)
{
    if (!fallback.candidateDirectoryUrl.isValid() || fallback.candidateDirectoryUrl.isEmpty()) {
        return;
    }

    m_fallbackJob = m_candidateRepository.loadContainers(
        this, fallback.candidateDirectoryUrl,
        [this, fallback](std::vector<ContainerNavigationCandidate> candidates) {
            const ComicBookRemovalFallbackCandidates fallbackCandidates
                = comicBookRemovalFallbackCandidates(std::move(candidates), fallback);
            openComicBookFallbackCandidate(
                fallbackCandidates.preferred, fallbackCandidates.fallback);
        },
        [](const QString &) {});
}

void ImageDeletionController::openComicBookFallbackCandidate(
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
        this, *candidate,
        [this](const QUrl &imageUrl, const QUrl &containerUrl) {
            report(ImageDeletionEffect::openContainerImageFallback(imageUrl, containerUrl));
        },
        [this, fallbackCandidate](const QUrl &, ImageCandidateRepositoryError, const QString &) {
            openComicBookFallbackCandidate(fallbackCandidate, std::nullopt);
        });
}

void ImageDeletionController::setInProgress(bool inProgress)
{
    if (m_inProgress == inProgress) {
        return;
    }

    m_inProgress = inProgress;
    invokeIfSet(m_callbacks.inProgressChanged);
}

void ImageDeletionController::cancel()
{
    cancelFileDeletion();
    cancelFallback();
}

void ImageDeletionController::cancelFileDeletion()
{
    m_fileDeletionJob.cancel();
    setInProgress(false);
}

void ImageDeletionController::cancelFallback() { m_fallbackJob.cancel(); }

void ImageDeletionController::report(ImageDeletionEffect effect)
{
    invokeIfSet(m_callbacks.effect, std::move(effect));
}

void ImageDeletionController::reportFailure(const QString &errorString)
{
    const QString message = errorString.isEmpty() ? genericFileDeletionErrorMessage() : errorString;
    report(ImageDeletionEffect::reportFailure(message));
}
}
