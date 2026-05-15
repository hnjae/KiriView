// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedeletioncontroller.h"

#include "filedeletionfallback.h"
#include "imagecallback.h"
#include "imagecontainer.h"
#include "imageviewtext.h"

#include <optional>
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
class ImageDeletionFallbackExecutor final
{
public:
    using EffectCallback = std::function<void(ImageDeletionEffect)>;

    ImageDeletionFallbackExecutor(QObject *receiver,
        ImageNavigationCandidateProvider candidateProvider, EffectCallback effectCallback)
        : m_receiver(receiver)
        , m_candidateRepository(std::move(candidateProvider))
        , m_effectCallback(std::move(effectCallback))
    {
    }

    void open(const DeletionFallbackPlan &fallbackPlan)
    {
        std::visit([this](const auto &plan) { openPlan(plan); }, fallbackPlan);
    }

    void cancel() { m_job.cancel(); }

private:
    void openPlan(const NoDeletionFallbackPlan &) { }

    void openPlan(const ImageDeletionFallbackPlan &fallbackPlan)
    {
        m_job = m_candidateRepository.loadImages(
            m_receiver, fallbackPlan.imageContext,
            [this, fallbackPlan](std::vector<ImageNavigationCandidate> candidates) {
                const std::optional<QUrl> fallbackUrl
                    = imageDeletionFallbackUrl(std::move(candidates), fallbackPlan);
                if (fallbackUrl.has_value()) {
                    report(ImageDeletionEffect::openImageFallback(*fallbackUrl));
                }
            },
            [](const QString &) {});
    }

    void openPlan(const ComicBookDeletionFallbackPlan &fallbackPlan)
    {
        if (!fallbackPlan.candidateDirectoryUrl.isValid()
            || fallbackPlan.candidateDirectoryUrl.isEmpty()) {
            return;
        }

        m_job = m_candidateRepository.loadContainers(
            m_receiver, fallbackPlan.candidateDirectoryUrl,
            [this, fallbackPlan](std::vector<ContainerNavigationCandidate> candidates) {
                const ComicBookDeletionFallbackCandidates fallbackCandidates
                    = comicBookDeletionFallbackCandidates(std::move(candidates), fallbackPlan);
                openComicBookCandidate(fallbackCandidates.preferred, fallbackCandidates.fallback);
            },
            [](const QString &) {});
    }

    void openComicBookCandidate(const std::optional<ContainerNavigationCandidate> &candidate,
        const std::optional<ContainerNavigationCandidate> &fallbackCandidate)
    {
        if (!candidate.has_value()) {
            if (fallbackCandidate.has_value()) {
                openComicBookCandidate(fallbackCandidate, std::nullopt);
            }
            return;
        }

        m_job = m_candidateRepository.loadFirstImageInContainer(
            m_receiver, *candidate,
            [this](const QUrl &imageUrl, const QUrl &containerUrl) {
                report(ImageDeletionEffect::openContainerImageFallback(imageUrl, containerUrl));
            },
            [this, fallbackCandidate](const QUrl &, ImageCandidateRepositoryError,
                const QString &) { openComicBookCandidate(fallbackCandidate, std::nullopt); });
    }

    void report(ImageDeletionEffect effect) { invokeIfSet(m_effectCallback, std::move(effect)); }

    QObject *m_receiver = nullptr;
    ImageCandidateRepository m_candidateRepository;
    EffectCallback m_effectCallback;
    ImageIoJob m_job;
};

ImageDeletionController::ImageDeletionController(QObject *parent,
    ImageNavigationCandidateProvider candidateProvider, FileOperationProvider fileOperationProvider,
    Callbacks callbacks)
    : QObject(parent)
    , m_callbacks(std::move(callbacks))
    , m_fallbackExecutor(
          std::make_unique<ImageDeletionFallbackExecutor>(this, std::move(candidateProvider),
              [this](ImageDeletionEffect effect) { report(std::move(effect)); }))
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
        report(ImageDeletionEffect::clearDeletedImage());
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
    m_fallbackExecutor->open(fallbackPlan);
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

void ImageDeletionController::cancelFallback() { m_fallbackExecutor->cancel(); }

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
