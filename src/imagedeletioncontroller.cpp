// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedeletioncontroller.h"

#include "filedeletionfallback.h"
#include "imagecallback.h"
#include "imagecontainer.h"
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

    const DeletionFallbackPlan fallbackPlan = deletionFallbackPlanForDisplayedLocation(location);

    setInProgress(true);
    m_fileDeletionJob = m_fileOperationProvider(this, FileDeletionRequest { targetUrl, mode },
        [this, fallbackPlan](FileDeletionResult result, const QString &errorString) {
            finishFileDeletion(fallbackPlan, result, errorString);
        });
}

void ImageDeletionController::finishFileDeletion(
    const DeletionFallbackPlan &fallbackPlan, FileDeletionResult result, const QString &errorString)
{
    setInProgress(false);

    switch (fileDeletionCompletionAction(result)) {
    case FileDeletionCompletionAction::ClearDeletedImageAndOpenFallback:
        invokeIfSet(m_callbacks.clearDeletedImage);
        openDeletionFallback(fallbackPlan);
        return;
    case FileDeletionCompletionAction::Ignore:
        return;
    case FileDeletionCompletionAction::ReportFailure:
        reportFailure(errorString);
        return;
    }
}

void ImageDeletionController::openDeletionFallback(const DeletionFallbackPlan &fallbackPlan)
{
    std::visit([this](const auto &plan) { openDeletionFallbackPlan(plan); }, fallbackPlan);
}

void ImageDeletionController::openDeletionFallbackPlan(const NoDeletionFallbackPlan &) { }

void ImageDeletionController::openDeletionFallbackPlan(
    const ImageDeletionFallbackPlan &fallbackPlan)
{
    m_fallbackJob = m_candidateRepository.loadImages(
        this, fallbackPlan.imageContext,
        [this, fallbackPlan](std::vector<ImageNavigationCandidate> candidates) {
            const std::optional<QUrl> fallbackUrl
                = imageDeletionFallbackUrl(std::move(candidates), fallbackPlan);
            if (fallbackUrl.has_value()) {
                invokeIfSet(m_callbacks.openUrl, *fallbackUrl);
            }
        },
        [](const QString &) {});
}

void ImageDeletionController::openDeletionFallbackPlan(
    const ComicBookDeletionFallbackPlan &fallbackPlan)
{
    if (!fallbackPlan.candidateDirectoryUrl.isValid()
        || fallbackPlan.candidateDirectoryUrl.isEmpty()) {
        return;
    }

    m_fallbackJob = m_candidateRepository.loadContainers(
        this, fallbackPlan.candidateDirectoryUrl,
        [this, fallbackPlan](std::vector<ContainerNavigationCandidate> candidates) {
            const ComicBookDeletionFallbackCandidates fallbackCandidates
                = comicBookDeletionFallbackCandidates(std::move(candidates), fallbackPlan);
            openComicBookDeletionFallbackCandidate(
                fallbackCandidates.preferred, fallbackCandidates.fallback);
        },
        [](const QString &) {});
}

void ImageDeletionController::openComicBookDeletionFallbackCandidate(
    const std::optional<ContainerNavigationCandidate> &candidate,
    const std::optional<ContainerNavigationCandidate> &fallbackCandidate)
{
    if (!candidate.has_value()) {
        if (fallbackCandidate.has_value()) {
            openComicBookDeletionFallbackCandidate(fallbackCandidate, std::nullopt);
        }
        return;
    }

    m_fallbackJob = m_candidateRepository.loadFirstImageInContainer(
        this, *candidate,
        [this](const QUrl &imageUrl, const QUrl &containerUrl) {
            invokeIfSet(m_callbacks.openContainerImage, imageUrl, containerUrl);
        },
        [this, fallbackCandidate](const QUrl &, ImageCandidateRepositoryError, const QString &) {
            openComicBookDeletionFallbackCandidate(fallbackCandidate, std::nullopt);
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

void ImageDeletionController::reportFailure(const QString &errorString)
{
    const QString message = errorString.isEmpty() ? genericFileDeletionErrorMessage() : errorString;
    invokeIfSet(m_callbacks.failed, message);
}
}
