// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "imagecallback.h"
#include "imagedeletioncontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imageopencontroller.h"
#include "imageopenworkflow.h"
#include "imagepredecodecoordinator.h"
#include "imagepresentationcontroller.h"
#include "imagespreadgeometry.h"
#include "imagespreadpresentationcontroller.h"
#include "imageviewtext.h"

#include <QRectF>
#include <QString>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace KiriView {
ImageDocumentController::ImageDocumentController(
    QObject *parent, RenderContextProvider renderContextProvider, ChangeCallback changeCallback)
    : ImageDocumentController(parent, std::move(renderContextProvider), std::move(changeCallback),
          ImageAsyncDependencies {})
{
}

ImageDocumentController::ImageDocumentController(QObject *parent,
    RenderContextProvider renderContextProvider, ChangeCallback changeCallback,
    ImageAsyncDependencies dependencies, FileDeletionFailedCallback fileDeletionFailedCallback)
    : QObject(parent)
    , m_changeCallback(std::move(changeCallback))
    , m_state([this](ImageDocumentChange change) { notify(change); })
{
    dependencies = imageAsyncDependenciesWithDefaults(std::move(dependencies));
    RenderContextProvider primaryRenderContextProvider = renderContextProvider;
    RenderContextProvider spreadRenderContextProvider = std::move(renderContextProvider);
    m_deletionController = std::make_unique<ImageDeletionController>(this,
        dependencies.candidateProvider, std::move(dependencies.fileOperations),
        ImageDeletionController::Callbacks {
            [this]() { notify(ImageDocumentChange::FileDeletionInProgress); },
            [this]() { clearAfterSuccessfulFileDeletion(); },
            [this](const QUrl &url) { setSourceUrl(url); },
            [this](const QUrl &imageUrl, const QUrl &containerUrl) {
                setSourceUrlForLoad(imageUrl, containerUrl);
            },
            std::move(fileDeletionFailedCallback),
        });
    m_presentationController = std::make_unique<ImagePresentationController>(this,
        std::move(primaryRenderContextProvider),
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
            dependencies.candidateProvider, dependencies.imageDecode);
    m_navigationController = std::make_unique<ImageDocumentNavigationController>(this, m_state,
        *m_presentationController,
        ImageDocumentNavigationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
        },
        dependencies.candidateProvider);
    m_predecodeCoordinator = std::make_unique<ImagePredecodeCoordinator>(
        this, dependencies.candidateProvider, dependencies.imageDecode);
    m_spreadController = std::make_unique<ImageSpreadPresentationController>(this,
        std::move(spreadRenderContextProvider), m_state, *m_presentationController,
        ImageSpreadPresentationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](const QUrl &url) { return takePredecodedImage(url); },
            [this]() { return currentPageNumber(); },
            [this]() { return imageCount(); },
            [this](int pageNumber) { return m_navigationController->urlAtPage(pageNumber); },
        },
        dependencies.candidateProvider, dependencies.imageDecode);
}

ImageDocumentController::~ImageDocumentController()
{
    m_deletionController->cancel();
    m_presentationController->stopAnimation();
    m_spreadController->shutdown();
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

ImageDocumentStatus ImageDocumentController::status() const
{
    return m_spreadController->status(m_state.status());
}

bool ImageDocumentController::loading() const
{
    return m_spreadController->loading(m_state.loading());
}

QString ImageDocumentController::errorString() const { return m_state.errorString(); }

QString ImageDocumentController::windowTitleFileName() const
{
    return m_state.windowTitleFileName();
}

QUrl ImageDocumentController::displayedUrl() const { return m_state.displayedUrl(); }

QSize ImageDocumentController::imageSize() const { return m_spreadController->imageSize(); }

QSize ImageDocumentController::primaryImageSize() const
{
    return m_presentationController->imageSize();
}

QSize ImageDocumentController::secondaryImageSize() const
{
    return m_spreadController->secondaryImageSize();
}

QSizeF ImageDocumentController::viewportSize() const
{
    return m_presentationController->viewportSize();
}

void ImageDocumentController::setViewportSize(const QSizeF &viewportSize)
{
    m_spreadController->setViewportSize(viewportSize);
}

QRectF ImageDocumentController::visibleItemRect() const
{
    return m_spreadController->visibleItemRect();
}

void ImageDocumentController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    m_spreadController->setVisibleItemRect(visibleItemRect);
}

QSizeF ImageDocumentController::displaySize() const { return m_spreadController->displaySize(); }

QSizeF ImageDocumentController::primaryDisplaySize() const
{
    return m_spreadController->primaryDisplaySize();
}

QSizeF ImageDocumentController::secondaryDisplaySize() const
{
    return m_spreadController->secondaryDisplaySize();
}

qreal ImageDocumentController::zoomPercent() const { return m_spreadController->zoomPercent(); }

void ImageDocumentController::setZoomPercent(qreal zoomPercent)
{
    m_spreadController->setZoomPercent(zoomPercent);
}

ImageZoomMode ImageDocumentController::zoomMode() const { return m_spreadController->zoomMode(); }

qreal ImageDocumentController::maximumManualZoomPercent() const
{
    return m_spreadController->maximumManualZoomPercent();
}

qreal ImageDocumentController::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_spreadController->clampedManualZoomPercent(zoomPercent);
}

qreal ImageDocumentController::steppedManualZoomPercent(qreal stepCount) const
{
    return m_spreadController->steppedManualZoomPercent(stepCount);
}

int ImageDocumentController::currentPageNumber() const
{
    return m_navigationController->currentPageNumber();
}

int ImageDocumentController::currentLastPageNumber() const
{
    return imageSpreadCurrentLastPageNumber(currentPageNumber(), secondaryPageVisible());
}

int ImageDocumentController::imageCount() const { return m_navigationController->imageCount(); }

bool ImageDocumentController::containerNavigationAvailable() const
{
    return m_state.containerNavigationAvailable();
}

bool ImageDocumentController::fileDeletionInProgress() const
{
    return m_deletionController->inProgress();
}

bool ImageDocumentController::twoPageModeEnabled() const
{
    return m_spreadController->twoPageModeEnabled();
}

void ImageDocumentController::setTwoPageModeEnabled(bool enabled)
{
    m_spreadController->setTwoPageModeEnabled(enabled);
}

bool ImageDocumentController::twoPageModeAvailable() const
{
    return m_spreadController->twoPageModeAvailable();
}

bool ImageDocumentController::rightToLeftReadingEnabled() const
{
    return m_spreadController->rightToLeftReadingEnabled();
}

void ImageDocumentController::setRightToLeftReadingEnabled(bool enabled)
{
    m_spreadController->setRightToLeftReadingEnabled(enabled);
}

bool ImageDocumentController::rightToLeftReadingAvailable() const
{
    return m_spreadController->rightToLeftReadingAvailable();
}

bool ImageDocumentController::secondaryPageVisible() const
{
    return m_spreadController->secondaryPageVisible();
}

std::shared_ptr<DisplayedImageSurface> ImageDocumentController::imageSurface(
    DisplayedPageRole role) const
{
    if (m_spreadController->transitionInProgress()) {
        return nullptr;
    }

    if (role == DisplayedPageRole::Secondary) {
        return m_spreadController->secondaryImageSurface();
    }

    return m_presentationController->imageSurface();
}

const QImage &ImageDocumentController::image() const { return m_presentationController->image(); }

quint64 ImageDocumentController::imageRevision(DisplayedPageRole role) const
{
    if (m_spreadController->transitionInProgress()) {
        return 0;
    }

    if (role == DisplayedPageRole::Secondary) {
        return m_spreadController->secondaryImageRevision();
    }

    return m_presentationController->imageRevision();
}

void ImageDocumentController::openPreviousImage()
{
    if (!twoPageModeActive() || currentPageNumber() <= 0) {
        m_navigationController->openPreviousImage();
        return;
    }

    bool previousPageIsWide = false;
    if (secondaryPageVisible() && currentPageNumber() > 2) {
        const std::optional<QUrl> previousUrl
            = m_navigationController->urlAtPage(currentPageNumber() - 1);
        if (previousUrl.has_value()) {
            previousPageIsWide = m_spreadController->cachedPageIsWide(*previousUrl).value_or(false);
        }
    }
    openImageAtPage(imageSpreadPreviousPageTarget(
        currentPageNumber(), secondaryPageVisible(), previousPageIsWide));
}

void ImageDocumentController::openNextImage()
{
    if (!twoPageModeActive() || currentPageNumber() <= 0) {
        m_navigationController->openNextImage();
        return;
    }

    const int nextPage = imageSpreadNextPageTarget(currentLastPageNumber(), imageCount());
    if (nextPage <= 0) {
        return;
    }

    openImageAtPage(nextPage);
}

void ImageDocumentController::openPreviousSinglePage() { openImageAtRelativePageOffset(-1); }

void ImageDocumentController::openNextSinglePage() { openImageAtRelativePageOffset(1); }

void ImageDocumentController::openPreviousContainer()
{
    m_navigationController->openPreviousContainer();
}

void ImageDocumentController::openNextContainer() { m_navigationController->openNextContainer(); }

void ImageDocumentController::deleteDisplayedFile(FileDeletionMode mode)
{
    if (!m_presentationController->hasImage()) {
        return;
    }

    m_deletionController->deleteDisplayedFile(m_state.displayedImageLocation(), mode);
}

void ImageDocumentController::openImageAtPage(int pageNumber)
{
    const std::optional<QUrl> pageUrl = m_navigationController->urlAtPage(pageNumber);
    if (!pageUrl.has_value()) {
        return;
    }

    const bool spreadTransition = m_spreadController->shouldBeginTransition(pageNumber);
    if (spreadTransition) {
        m_spreadController->beginTransition();
    }
    setSourceUrlForLoad(*pageUrl, QUrl(), spreadTransition);
}

void ImageDocumentController::resetZoom() { m_spreadController->resetZoom(); }

void ImageDocumentController::setFitMode(ImageZoomMode zoomMode)
{
    m_spreadController->setFitMode(zoomMode);
}

void ImageDocumentController::updateRenderContext() { m_spreadController->updateRenderContext(); }

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

void ImageDocumentController::dispatchEffects(ImageDocumentEffects effects)
{
    for (ImageDocumentEffect &effect : effects) {
        dispatchEffect(std::move(effect));
    }
}

void ImageDocumentController::setSourceUrlForLoad(
    const QUrl &sourceUrl, const QUrl &containerNavigationUrl, bool preserveTwoPageSpreadTransition)
{
    m_deletionController->cancel();

    const bool sourceUrlChanged = m_state.sourceUrl() != sourceUrl;
    const bool resetRightToLeftReading = m_spreadController->shouldResetRightToLeftReadingForLoad(
        m_state.displayedArchiveDocument(), sourceUrl, containerNavigationUrl);
    const ImageOpenSourceLoadPlan plan = ImageOpenWorkflow::sourceLoadPlan(sourceUrlChanged,
        preserveTwoPageSpreadTransition, resetRightToLeftReading, containerNavigationUrl.isEmpty());
    if (plan.cancelNavigationAndPredecode) {
        cancelNavigationAndPredecode();
    }
    if (plan.finishSpreadTransition) {
        m_spreadController->finishTransition();
    }

    const bool notifyRightToLeftReading
        = plan.resetRightToLeftReading && m_spreadController->rightToLeftReadingEnabled();
    if (plan.resetRightToLeftReading) {
        m_spreadController->resetRightToLeftReading();
    }
    if (!sourceUrlChanged && notifyRightToLeftReading) {
        m_spreadController->notifyRightToLeftReadingChanged();
    }
    if (plan.clearSecondaryPage) {
        m_spreadController->clearSecondaryPage();
    }
    if (plan.clearLoadingContainerNavigationUrl) {
        m_state.clearLoadingContainerNavigationUrl();
    }
    if (plan.updateContainerNavigationUrl) {
        m_state.setContainerNavigationUrl(containerNavigationUrl);
    }
    if (plan.setLoadingContainerNavigationUrl) {
        m_state.setLoadingContainerNavigationUrl(containerNavigationUrl);
    }
    if (plan.setSourceUrl) {
        m_state.setSourceUrl(sourceUrl);
    }
    if (plan.beginOpen) {
        m_openController->open();
    }
    if (sourceUrlChanged && notifyRightToLeftReading) {
        m_spreadController->notifyRightToLeftReadingChanged();
    }
}

void ImageDocumentController::clearAfterSuccessfulFileDeletion()
{
    cancelNavigationAndPredecode();
    m_openController->cancel();
    m_spreadController->finishTransition();
    m_spreadController->clearSecondaryPage();
    m_state.setSourceUrl(QUrl());
    m_state.setErrorString(QString());
    dispatchEffects(ImageOpenWorkflow::finishEmptySourceLoad(m_state));
}

void ImageDocumentController::scheduleAdjacentImagePredecode()
{
    m_predecodeCoordinator->scheduleDisplayedImage(
        m_state.displayedImageLocation(), *m_presentationController);
}

void ImageDocumentController::cancelNavigationAndPredecode()
{
    m_navigationController->cancelNavigation();
    m_navigationController->cancelContainerNavigation();
    cancelPredecode();
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
    ImageDocumentEffects effects
        = ImageOpenWorkflow::finishAnimationLoadWithError(m_state, message);
    dispatchEffects(std::move(effects));
}

bool ImageDocumentController::twoPageModeActive() const
{
    return m_spreadController->twoPageModeActive();
}

void ImageDocumentController::openImageAtRelativePageOffset(int offset)
{
    const int targetPage = imageSpreadRelativePageTarget(currentPageNumber(), imageCount(), offset);
    if (targetPage <= 0) {
        return;
    }

    openImageAtPage(targetPage);
}

void ImageDocumentController::notify(ImageDocumentChange change)
{
    const ImageDocumentChangeDispatchPlan plan
        = imageDocumentChangeDispatchPlan(change, m_state.errorString().isEmpty());

    if (plan.finishSpreadTransition) {
        m_spreadController->finishTransition();
    }

    if (plan.refreshSecondaryPage) {
        m_spreadController->refreshSecondaryPage();
    }
    if (plan.notifyRightToLeftReading) {
        m_spreadController->notifyRightToLeftReadingChanged();
    }

    invokeIfSet(m_changeCallback, change);
}

void ImageDocumentController::clearImage()
{
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->clear();
    }
    m_spreadController->finishTransition();
    m_spreadController->clearSecondaryPage();
    m_navigationController->cancelPageNavigationUpdate();
    m_state.clearDisplayedImageUrls();
    m_presentationController->clearImage();
    m_navigationController->clearPageNavigation();
    m_spreadController->notifyRightToLeftReadingChanged();
}
}
