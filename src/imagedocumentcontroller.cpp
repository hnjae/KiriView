// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "filedeletion.h"
#include "filedeletionfallback.h"
#include "imagecallback.h"
#include "imagedocumentnavigationcontroller.h"
#include "imageopencontroller.h"
#include "imageopenworkflow.h"
#include "imagepredecodecoordinator.h"
#include "imagepresentationcontroller.h"
#include "imageurl.h"
#include "imageviewtext.h"

#include <QString>
#include <memory>
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
ImageDocumentController::ImageDocumentController(QObject *parent,
    RenderContextProvider renderContextProvider, ChangeCallback changeCallback,
    ImageAsyncDependencies dependencies, FileDeletionFailedCallback fileDeletionFailedCallback)
    : QObject(parent)
    , m_changeCallback(std::move(changeCallback))
    , m_fileDeletionFailedCallback(std::move(fileDeletionFailedCallback))
    , m_state([this](ImageDocumentChange change) { notify(change); })
    , m_deletionCandidateRepository(dependencies.candidateProvider)
    , m_fileOperationProvider(dependencies.fileOperations ? std::move(dependencies.fileOperations)
                                                          : defaultFileOperationProvider())
{
    m_presentationController
        = std::make_unique<ImagePresentationController>(this, std::move(renderContextProvider),
            ImagePresentationController::Callbacks {
                [this](ImageDocumentChange change) { notify(change); },
                [this](const QString &errorString) {
                    dispatchEffect(ImageDocumentEffect::animationFailed(errorString));
                },
            });
    m_openController
        = std::make_unique<ImageOpenController>(this, m_state, *m_presentationController,
            ImageOpenController::Callbacks {
                [this](const QUrl &url) { return takePredecodedImage(url); },
                [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
            },
            dependencies);
    m_navigationController = std::make_unique<ImageDocumentNavigationController>(this, m_state,
        *m_presentationController,
        ImageDocumentNavigationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
        },
        dependencies.candidateProvider);
    m_predecodeCoordinator = std::make_unique<ImagePredecodeCoordinator>(this, dependencies);
}

ImageDocumentController::~ImageDocumentController()
{
    cancelFileDeletion();
    cancelFileDeletionFallback();
    m_presentationController->stopAnimation();
    cancelPredecode();
    m_navigationController->cancelPageNavigationUpdate();
    m_navigationController->cancelContainerNavigation();
    m_navigationController->cancelNavigation();
    m_openController->cancel();
}

QUrl ImageDocumentController::sourceUrl() const { return m_state.sourceUrl(); }

void ImageDocumentController::setSourceUrl(const QUrl &sourceUrl)
{
    setSourceUrlForLoad(sourceUrl, QUrl());
}

ImageDocumentStatus ImageDocumentController::status() const { return m_state.status(); }

bool ImageDocumentController::loading() const { return m_state.loading(); }

QString ImageDocumentController::errorString() const { return m_state.errorString(); }

QString ImageDocumentController::windowTitleFileName() const
{
    return m_state.windowTitleFileName();
}

QUrl ImageDocumentController::displayedUrl() const { return m_state.displayedUrl(); }

QSize ImageDocumentController::imageSize() const { return m_presentationController->imageSize(); }

QSizeF ImageDocumentController::viewportSize() const
{
    return m_presentationController->viewportSize();
}

void ImageDocumentController::setViewportSize(const QSizeF &viewportSize)
{
    m_presentationController->setViewportSize(viewportSize);
}

QRectF ImageDocumentController::visibleItemRect() const
{
    return m_presentationController->visibleItemRect();
}

void ImageDocumentController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    m_presentationController->setVisibleItemRect(visibleItemRect);
}

QSizeF ImageDocumentController::displaySize() const
{
    return m_presentationController->displaySize();
}

qreal ImageDocumentController::zoomPercent() const
{
    return m_presentationController->zoomPercent();
}

void ImageDocumentController::setZoomPercent(qreal zoomPercent)
{
    m_presentationController->setZoomPercent(zoomPercent);
}

ImageZoomMode ImageDocumentController::zoomMode() const
{
    return m_presentationController->zoomMode();
}

qreal ImageDocumentController::maximumManualZoomPercent() const
{
    return m_presentationController->maximumManualZoomPercent();
}

qreal ImageDocumentController::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_presentationController->clampedManualZoomPercent(zoomPercent);
}

int ImageDocumentController::currentPageNumber() const
{
    return m_navigationController->currentPageNumber();
}

int ImageDocumentController::imageCount() const { return m_navigationController->imageCount(); }

bool ImageDocumentController::containerNavigationAvailable() const
{
    return m_state.containerNavigationAvailable();
}

bool ImageDocumentController::fileDeletionInProgress() const { return m_fileDeletionInProgress; }

std::shared_ptr<DisplayedImageSurface> ImageDocumentController::imageSurface() const
{
    return m_presentationController->imageSurface();
}

const QImage &ImageDocumentController::image() const { return m_presentationController->image(); }

quint64 ImageDocumentController::imageRevision() const
{
    return m_presentationController->imageRevision();
}

void ImageDocumentController::openPreviousImage() { m_navigationController->openPreviousImage(); }

void ImageDocumentController::openNextImage() { m_navigationController->openNextImage(); }

void ImageDocumentController::openPreviousContainer()
{
    m_navigationController->openPreviousContainer();
}

void ImageDocumentController::openNextContainer() { m_navigationController->openNextContainer(); }

void ImageDocumentController::deleteDisplayedFile(FileDeletionMode mode)
{
    if (m_fileDeletionInProgress || !m_presentationController->hasImage()) {
        return;
    }

    const DisplayedImageLocation location = m_state.displayedImageLocation();
    const QUrl targetUrl = deletionTargetUrlForDisplayedLocation(location);
    if (targetUrl.isEmpty()) {
        return;
    }

    const DeletionFallbackPlan fallbackPlan = deletionFallbackPlanForDisplayedLocation(location);

    setFileDeletionInProgress(true);
    m_fileDeletionJob = m_fileOperationProvider(this, FileDeletionRequest { targetUrl, mode },
        [this, fallbackPlan](FileDeletionResult result, const QString &errorString) {
            finishFileDeletion(fallbackPlan, result, errorString);
        });
}

void ImageDocumentController::openImageAtPage(int pageNumber)
{
    m_navigationController->openImageAtPage(pageNumber);
}

void ImageDocumentController::resetZoom() { m_presentationController->resetZoom(); }

void ImageDocumentController::setFitMode(ImageZoomMode zoomMode)
{
    m_presentationController->setFitMode(zoomMode);
}

void ImageDocumentController::updateRenderContext()
{
    m_presentationController->updateRenderContext();
}

void ImageDocumentController::dispatchEffect(ImageDocumentEffect effect)
{
    std::visit([this](const auto &payload) { dispatchEffectPayload(payload); }, effect.payload);
}

void ImageDocumentController::dispatchEffectPayload(const ClearImageEffect &) { clearImage(); }

void ImageDocumentController::dispatchEffectPayload(const ResetZoomEffect &) { resetZoom(); }

void ImageDocumentController::dispatchEffectPayload(const UpdatePageNavigationEffect &)
{
    m_navigationController->updatePageNavigation();
}

void ImageDocumentController::dispatchEffectPayload(const ScheduleAdjacentImagePredecodeEffect &)
{
    scheduleAdjacentImagePredecode();
}

void ImageDocumentController::dispatchEffectPayload(const OpenUrlEffect &payload)
{
    setSourceUrl(payload.url);
}

void ImageDocumentController::dispatchEffectPayload(const ContainerImageSelectedEffect &payload)
{
    setSourceUrlForLoad(payload.imageUrl, payload.containerUrl);
}

void ImageDocumentController::dispatchEffectPayload(const EmptyContainerSelectedEffect &payload)
{
    m_openController->finishContainerNavigationWithEmptyContainer(payload.containerUrl);
}

void ImageDocumentController::dispatchEffectPayload(const ContainerNavigationFailedEffect &payload)
{
    m_openController->finishContainerNavigationLoadWithError(
        payload.containerUrl, payload.errorString);
}

void ImageDocumentController::dispatchEffectPayload(const PrepareFailedContainerEffect &payload)
{
    m_presentationController->prepareFailedContainer(payload.containerUrl);
}

void ImageDocumentController::dispatchEffectPayload(const AnimationFailedEffect &payload)
{
    finishWithAnimationError(payload.errorString);
}

void ImageDocumentController::dispatchEffects(const ImageDocumentEffects &effects)
{
    for (const ImageDocumentEffect &effect : effects) {
        dispatchEffect(effect);
    }
}

void ImageDocumentController::setSourceUrlForLoad(
    const QUrl &sourceUrl, const QUrl &containerNavigationUrl)
{
    cancelFileDeletion();
    cancelFileDeletionFallback();

    if (m_state.sourceUrl() == sourceUrl) {
        m_state.clearLoadingContainerNavigationUrl();
        if (!containerNavigationUrl.isEmpty()) {
            m_state.setContainerNavigationUrl(containerNavigationUrl);
        }
        return;
    }

    m_navigationController->cancelNavigation();
    m_navigationController->cancelContainerNavigation();
    cancelPredecode();
    m_state.setLoadingContainerNavigationUrl(containerNavigationUrl);
    m_state.setSourceUrl(sourceUrl);
    m_openController->open();
}

void ImageDocumentController::clearAfterSuccessfulFileDeletion()
{
    m_navigationController->cancelNavigation();
    m_navigationController->cancelContainerNavigation();
    cancelPredecode();
    m_openController->cancel();
    m_state.setSourceUrl(QUrl());
    m_state.setErrorString(QString());
    dispatchEffects(ImageOpenWorkflow::finishEmptySourceLoad(m_state));
}

void ImageDocumentController::finishFileDeletion(
    const DeletionFallbackPlan &fallbackPlan, FileDeletionResult result, const QString &errorString)
{
    setFileDeletionInProgress(false);

    switch (result) {
    case FileDeletionResult::Succeeded:
        clearAfterSuccessfulFileDeletion();
        openDeletionFallback(fallbackPlan);
        return;
    case FileDeletionResult::Canceled:
        return;
    case FileDeletionResult::Failed:
        reportFileDeletionFailure(errorString);
        return;
    }
}

void ImageDocumentController::openDeletionFallback(const DeletionFallbackPlan &fallbackPlan)
{
    switch (fallbackPlan.kind) {
    case DeletionFallbackKind::Image:
        openImageDeletionFallback(fallbackPlan);
        return;
    case DeletionFallbackKind::ComicBookArchive:
        openComicBookDeletionFallback(fallbackPlan);
        return;
    case DeletionFallbackKind::None:
        return;
    }
}

void ImageDocumentController::openImageDeletionFallback(const DeletionFallbackPlan &fallbackPlan)
{
    if (!fallbackPlan.imageContext.has_value()) {
        return;
    }

    m_fileDeletionFallbackJob = m_deletionCandidateRepository.loadImages(
        this, *fallbackPlan.imageContext,
        [this, fallbackPlan](std::vector<ImageNavigationCandidate> candidates) {
            const std::optional<QUrl> fallbackUrl
                = imageDeletionFallbackUrl(std::move(candidates), fallbackPlan);
            if (fallbackUrl.has_value()) {
                setSourceUrl(*fallbackUrl);
            }
        },
        [](const QString &) {});
}

void ImageDocumentController::openComicBookDeletionFallback(
    const DeletionFallbackPlan &fallbackPlan)
{
    if (fallbackPlan.currentContainerUrl.isEmpty()) {
        return;
    }

    const QUrl parentUrl = parentUrlForContainerNavigation(fallbackPlan.currentContainerUrl);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return;
    }

    m_fileDeletionFallbackJob = m_deletionCandidateRepository.loadContainers(
        this, parentUrl,
        [this, fallbackPlan](std::vector<ContainerNavigationCandidate> candidates) {
            const ComicBookDeletionFallbackCandidates fallbackCandidates
                = comicBookDeletionFallbackCandidates(std::move(candidates), fallbackPlan);
            openComicBookDeletionFallbackCandidate(
                fallbackCandidates.preferred, fallbackCandidates.fallback);
        },
        [](const QString &) {});
}

void ImageDocumentController::openComicBookDeletionFallbackCandidate(
    const std::optional<ContainerNavigationCandidate> &candidate,
    const std::optional<ContainerNavigationCandidate> &fallbackCandidate)
{
    if (!candidate.has_value()) {
        if (fallbackCandidate.has_value()) {
            openComicBookDeletionFallbackCandidate(fallbackCandidate, std::nullopt);
        }
        return;
    }

    m_fileDeletionFallbackJob = m_deletionCandidateRepository.loadFirstImageInContainer(
        this, *candidate,
        [this](const QUrl &imageUrl, const QUrl &containerUrl) {
            setSourceUrlForLoad(imageUrl, containerUrl);
        },
        [this, fallbackCandidate](const QUrl &, ImageCandidateRepositoryError, const QString &) {
            openComicBookDeletionFallbackCandidate(fallbackCandidate, std::nullopt);
        });
}

void ImageDocumentController::setFileDeletionInProgress(bool inProgress)
{
    if (m_fileDeletionInProgress == inProgress) {
        return;
    }

    m_fileDeletionInProgress = inProgress;
    notify(ImageDocumentChange::FileDeletionInProgress);
}

void ImageDocumentController::cancelFileDeletion()
{
    m_fileDeletionJob.cancel();
    setFileDeletionInProgress(false);
}

void ImageDocumentController::cancelFileDeletionFallback() { m_fileDeletionFallbackJob.cancel(); }

void ImageDocumentController::reportFileDeletionFailure(const QString &errorString)
{
    const QString message = errorString.isEmpty() ? genericFileDeletionErrorMessage() : errorString;
    invokeIfSet(m_fileDeletionFailedCallback, message);
}

void ImageDocumentController::scheduleAdjacentImagePredecode()
{
    if (!m_presentationController->hasImage() || m_state.displayedUrl().isEmpty()) {
        cancelPredecode();
        return;
    }

    std::optional<StaticImagePayload> staticImage = m_presentationController->staticImage();
    if (!staticImage.has_value()) {
        cancelPredecode();
        return;
    }

    m_predecodeCoordinator->schedule(ImagePredecodeCoordinator::Context {
        m_state.displayedImageLocation(), m_presentationController->isPredecodeCacheable(),
        std::move(*staticImage), m_presentationController->firstDisplayDecodeContext() });
}

void ImageDocumentController::cancelPredecode()
{
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->cancel();
    }
}

std::optional<PredecodedImage> ImageDocumentController::takePredecodedImage(const QUrl &url) const
{
    if (m_predecodeCoordinator == nullptr) {
        return std::nullopt;
    }

    return m_predecodeCoordinator->tryTake(url);
}

void ImageDocumentController::finishWithAnimationError(const QString &errorString)
{
    const QString message = errorString.isEmpty()
        ? imageViewText("Could not decode the selected image animation.")
        : errorString;
    const ImageDocumentEffects effects
        = ImageOpenWorkflow::finishAnimationLoadWithError(m_state, message);
    dispatchEffects(effects);
}

void ImageDocumentController::notify(ImageDocumentChange change)
{
    invokeIfSet(m_changeCallback, change);
}

void ImageDocumentController::clearImage()
{
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->clear();
    }
    m_navigationController->cancelPageNavigationUpdate();
    m_state.clearDisplayedImageUrls();
    m_presentationController->clearImage();
    m_navigationController->clearPageNavigation();
}
}
