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
#include "imagesecondarypagecontroller.h"
#include "imagespreadgeometry.h"
#include "imagespreadmodecontroller.h"
#include "imagespreadzoomcontroller.h"
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
    RenderContextProvider secondaryRenderContextProvider = renderContextProvider;
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
    m_secondaryPageController = std::make_unique<ImageSecondaryPageController>(this,
        std::move(secondaryRenderContextProvider),
        ImageSecondaryPageController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](ImageSecondaryPageLoadResult result, const DisplayedImageLocation &location,
                const QSize &imageSize) {
                handleSecondaryPageLoadFinished(result, location, imageSize);
            },
            [this]() { notifyTwoPageModeChanged(); },
            [this](const QUrl &url) { return takePredecodedImage(url); },
        },
        dependencies.candidateProvider, dependencies.imageDecode);
    m_spreadModeController = std::make_unique<ImageSpreadModeController>([this]() {
        return ImageSpreadModeAvailability { m_presentationController->hasImage(),
            m_state.displayedImageLocation() };
    });
    m_spreadZoomController
        = std::make_unique<ImageSpreadZoomController>(std::move(spreadRenderContextProvider),
            *m_presentationController, m_secondaryPageController->presentationController());
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
}

ImageDocumentController::~ImageDocumentController()
{
    m_deletionController->cancel();
    m_presentationController->stopAnimation();
    m_secondaryPageController->stopAnimation();
    cancelPredecode();
    m_secondaryPageController->cancel();
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
    if (m_spreadModeController->spreadTransitionInProgress()) {
        return ImageDocumentStatus::Loading;
    }

    return m_state.status();
}

bool ImageDocumentController::loading() const
{
    return m_spreadModeController->spreadTransitionInProgress() || m_state.loading();
}

QString ImageDocumentController::errorString() const { return m_state.errorString(); }

QString ImageDocumentController::windowTitleFileName() const
{
    return m_state.windowTitleFileName();
}

QUrl ImageDocumentController::displayedUrl() const { return m_state.displayedUrl(); }

QSize ImageDocumentController::imageSize() const
{
    return secondaryPageVisible() ? spreadImageSize() : m_presentationController->imageSize();
}

QSize ImageDocumentController::primaryImageSize() const
{
    return m_presentationController->imageSize();
}

QSize ImageDocumentController::secondaryImageSize() const
{
    return secondaryPageVisible() ? m_secondaryPageController->imageSize() : QSize();
}

QSizeF ImageDocumentController::viewportSize() const
{
    return m_presentationController->viewportSize();
}

void ImageDocumentController::setViewportSize(const QSizeF &viewportSize)
{
    m_presentationController->setViewportSize(viewportSize);
    m_secondaryPageController->setViewportSize(viewportSize);
    if (secondaryPageVisible()) {
        updateSpreadZoomState();
    }
}

QRectF ImageDocumentController::visibleItemRect() const
{
    return secondaryPageVisible() ? m_spreadZoomController->visibleItemRect()
                                  : m_presentationController->visibleItemRect();
}

void ImageDocumentController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    m_spreadZoomController->setVisibleItemRect(visibleItemRect);
    if (secondaryPageVisible()) {
        m_spreadZoomController->applyVisibleItemRects(rightToLeftReadingActive());
        notify(ImageDocumentChange::VisibleItemRect);
        return;
    }

    m_presentationController->setVisibleItemRect(visibleItemRect);
}

QSizeF ImageDocumentController::displaySize() const
{
    if (secondaryPageVisible()) {
        return m_spreadZoomController->displaySize();
    }

    return m_presentationController->displaySize();
}

QSizeF ImageDocumentController::primaryDisplaySize() const
{
    if (!secondaryPageVisible()) {
        return m_presentationController->displaySize();
    }

    return m_spreadZoomController->primaryDisplaySize();
}

QSizeF ImageDocumentController::secondaryDisplaySize() const
{
    if (!secondaryPageVisible()) {
        return {};
    }

    return m_spreadZoomController->secondaryDisplaySize();
}

qreal ImageDocumentController::zoomPercent() const
{
    if (secondaryPageVisible()) {
        return m_spreadZoomController->zoomPercent();
    }

    return m_presentationController->zoomPercent();
}

void ImageDocumentController::setZoomPercent(qreal zoomPercent)
{
    if (secondaryPageVisible()) {
        m_spreadZoomController->setZoomPercent(zoomPercent, rightToLeftReadingActive());
        notifySpreadZoomChanged();
        return;
    }

    m_presentationController->setZoomPercent(zoomPercent);
}

ImageZoomMode ImageDocumentController::zoomMode() const
{
    if (secondaryPageVisible()) {
        return m_spreadZoomController->zoomMode();
    }

    return m_presentationController->zoomMode();
}

qreal ImageDocumentController::maximumManualZoomPercent() const
{
    if (secondaryPageVisible()) {
        return m_spreadZoomController->maximumManualZoomPercent();
    }

    return m_presentationController->maximumManualZoomPercent();
}

qreal ImageDocumentController::clampedManualZoomPercent(qreal zoomPercent) const
{
    if (secondaryPageVisible()) {
        return m_spreadZoomController->clampedManualZoomPercent(zoomPercent);
    }

    return m_presentationController->clampedManualZoomPercent(zoomPercent);
}

qreal ImageDocumentController::steppedManualZoomPercent(qreal stepCount) const
{
    if (secondaryPageVisible()) {
        return m_spreadZoomController->steppedManualZoomPercent(stepCount);
    }

    return m_presentationController->steppedManualZoomPercent(stepCount);
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
    return m_spreadModeController->twoPageModeEnabled();
}

void ImageDocumentController::setTwoPageModeEnabled(bool enabled)
{
    const ImageSpreadTwoPageModeChange change
        = m_spreadModeController->setTwoPageModeEnabled(enabled, secondaryPageVisible());
    if (!change.changed) {
        return;
    }

    ImageZoomMode previousZoomMode = ImageZoomMode::Fit;
    qreal previousZoomPercent = 100.0;
    if (change.restorePrimaryZoom) {
        previousZoomMode = zoomMode();
        previousZoomPercent = zoomPercent();
    }

    if (change.resetSpreadZoom) {
        m_spreadZoomController->clearZoomState();
    }
    if (change.finishTransition) {
        finishTwoPageSpreadTransition();
    }
    if (change.clearSecondaryPage) {
        clearSecondaryPage();
    }
    if (change.restorePrimaryZoom) {
        m_spreadZoomController->applyZoomToPrimaryPage(previousZoomMode, previousZoomPercent);
    }
    if (change.refreshSecondaryPage) {
        refreshSecondaryPage();
    }
    if (change.notifyTwoPageMode) {
        notifyTwoPageModeChanged();
    }
}

bool ImageDocumentController::twoPageModeAvailable() const
{
    return m_spreadModeController->twoPageModeAvailable();
}

bool ImageDocumentController::rightToLeftReadingEnabled() const
{
    return m_spreadModeController->rightToLeftReadingEnabled();
}

void ImageDocumentController::setRightToLeftReadingEnabled(bool enabled)
{
    if (!m_spreadModeController->setRightToLeftReadingEnabled(enabled)) {
        return;
    }

    if (secondaryPageVisible()) {
        m_spreadZoomController->applyVisibleItemRects(rightToLeftReadingActive());
    }
    notifyRightToLeftReadingChanged();
}

bool ImageDocumentController::rightToLeftReadingAvailable() const
{
    return m_spreadModeController->rightToLeftReadingAvailable();
}

bool ImageDocumentController::secondaryPageVisible() const
{
    return twoPageModeActive() && m_secondaryPageController->visible();
}

std::shared_ptr<DisplayedImageSurface> ImageDocumentController::imageSurface(
    DisplayedPageRole role) const
{
    if (m_spreadModeController->spreadTransitionInProgress()) {
        return nullptr;
    }

    if (role == DisplayedPageRole::Secondary) {
        return secondaryPageVisible() ? m_secondaryPageController->imageSurface() : nullptr;
    }

    return m_presentationController->imageSurface();
}

const QImage &ImageDocumentController::image() const { return m_presentationController->image(); }

quint64 ImageDocumentController::imageRevision(DisplayedPageRole role) const
{
    if (m_spreadModeController->spreadTransitionInProgress()) {
        return 0;
    }

    if (role == DisplayedPageRole::Secondary) {
        return secondaryPageVisible() ? m_secondaryPageController->imageRevision() : 0;
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
            previousPageIsWide
                = m_secondaryPageController->cachedPageIsWide(*previousUrl).value_or(false);
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

    const bool spreadTransition = shouldBeginTwoPageSpreadTransition(pageNumber);
    if (spreadTransition) {
        beginTwoPageSpreadTransition();
    }
    setSourceUrlForLoad(*pageUrl, QUrl(), spreadTransition);
}

void ImageDocumentController::resetZoom()
{
    if (secondaryPageVisible()) {
        m_spreadZoomController->resetZoom(rightToLeftReadingActive());
        notifySpreadZoomChanged();
        return;
    }

    m_presentationController->resetZoom();
}

void ImageDocumentController::setFitMode(ImageZoomMode zoomMode)
{
    if (secondaryPageVisible()) {
        if (!m_spreadZoomController->setFitMode(zoomMode, rightToLeftReadingActive())) {
            return;
        }
        notifySpreadZoomChanged();
        return;
    }

    m_presentationController->setFitMode(zoomMode);
}

void ImageDocumentController::updateRenderContext()
{
    m_presentationController->updateRenderContext();
    m_secondaryPageController->updateRenderContext();
    if (secondaryPageVisible()) {
        m_spreadZoomController->updateRenderContext(rightToLeftReadingActive());
        notify(ImageDocumentChange::DisplaySize);
        notify(ImageDocumentChange::MaximumManualZoomPercent);
        notify(ImageDocumentChange::Repaint);
        notify(ImageDocumentChange::TwoPageMode);
    }
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
    const bool resetRightToLeftReading
        = m_spreadModeController->shouldResetRightToLeftReadingForLoad(
            m_state.displayedArchiveDocument(), sourceUrl, containerNavigationUrl);
    const ImageOpenSourceLoadPlan plan = ImageOpenWorkflow::sourceLoadPlan(sourceUrlChanged,
        preserveTwoPageSpreadTransition, resetRightToLeftReading, containerNavigationUrl.isEmpty());
    if (plan.cancelNavigationAndPredecode) {
        cancelNavigationAndPredecode();
    }
    if (plan.finishSpreadTransition) {
        finishTwoPageSpreadTransition();
    }

    const bool notifyRightToLeftReading
        = plan.resetRightToLeftReading && m_spreadModeController->rightToLeftReadingEnabled();
    if (plan.resetRightToLeftReading) {
        m_spreadModeController->resetRightToLeftReading();
    }
    if (!sourceUrlChanged && notifyRightToLeftReading) {
        notifyRightToLeftReadingChanged();
    }
    if (plan.clearSecondaryPage) {
        clearSecondaryPage();
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
        notifyRightToLeftReadingChanged();
    }
}

void ImageDocumentController::clearAfterSuccessfulFileDeletion()
{
    cancelNavigationAndPredecode();
    m_openController->cancel();
    finishTwoPageSpreadTransition();
    clearSecondaryPage();
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

void ImageDocumentController::clearSecondaryPage() { m_secondaryPageController->clear(); }

void ImageDocumentController::refreshSecondaryPage()
{
    m_secondaryPageController->cachePageSize(
        m_state.displayedUrl(), m_presentationController->imageSize());

    auto finishWithPrimaryPage = [this]() {
        const bool wasVisible = secondaryPageVisible();
        m_spreadZoomController->applyStoredZoomToPrimaryPage();
        clearSecondaryPage();
        if (wasVisible) {
            notifyTwoPageModeChanged();
        }
        finishTwoPageSpreadTransition();
    };

    const int nextPageNumber = currentPageNumber() + 1;
    const std::optional<QUrl> nextUrl = m_navigationController->urlAtPage(nextPageNumber);
    const bool nextPageIsWide = nextUrl.has_value()
        && m_secondaryPageController->cachedPageIsWide(*nextUrl).value_or(false);
    const bool currentSecondaryMatchesNext = nextUrl.has_value() && secondaryPageVisible()
        && m_secondaryPageController->displayedImageLocation().imageUrl() == *nextUrl;
    const ImageSpreadSecondaryPageDecision decision
        = imageSpreadSecondaryPageDecision(twoPageModeActive(), currentPageNumber(), imageCount(),
            primaryPageIsWide(), nextUrl.has_value(), nextPageIsWide, currentSecondaryMatchesNext);
    if (decision == ImageSpreadSecondaryPageDecision::PrimaryOnly) {
        finishWithPrimaryPage();
        return;
    }
    if (decision == ImageSpreadSecondaryPageDecision::KeepCurrentSecondary) {
        return;
    }

    startSecondaryPageLoad(*nextUrl);
}

void ImageDocumentController::startSecondaryPageLoad(const QUrl &url)
{
    m_secondaryPageController->startLoad(url, m_state.displayedArchiveDocument(),
        m_presentationController->firstDisplayDecodeContext());
    notifyTwoPageModeChanged();
}

void ImageDocumentController::handleSecondaryPageLoadFinished(ImageSecondaryPageLoadResult result,
    const DisplayedImageLocation &location, const QSize &imageSize)
{
    if (result != ImageSecondaryPageLoadResult::Failed) {
        m_secondaryPageController->cachePageSize(location.imageUrl(), imageSize);
    }

    if (result == ImageSecondaryPageLoadResult::Visible) {
        finishSecondaryPageVisible();
        return;
    }

    finishSecondaryPageAsPrimaryOnly();
}

void ImageDocumentController::finishSecondaryPageAsPrimaryOnly()
{
    clearSecondaryPage();
    m_spreadZoomController->applyStoredZoomToPrimaryPage();
    finishTwoPageSpreadTransition();
    notifyTwoPageModeChanged();
}

void ImageDocumentController::finishSecondaryPageVisible()
{
    updateSpreadZoomState();
    finishTwoPageSpreadTransition();
    notifyTwoPageModeChanged();
}

bool ImageDocumentController::shouldBeginTwoPageSpreadTransition(int targetPageNumber) const
{
    return imageSpreadShouldBeginTransition(
        twoPageModeActive(), currentPageNumber(), targetPageNumber, imageCount());
}

void ImageDocumentController::beginTwoPageSpreadTransition()
{
    if (!m_spreadModeController->beginSpreadTransition()) {
        notifyTwoPageSpreadTransitionChanged();
        return;
    }

    notifyTwoPageSpreadTransitionChanged();
}

void ImageDocumentController::finishTwoPageSpreadTransition()
{
    if (!m_spreadModeController->finishSpreadTransition()) {
        return;
    }

    notifyTwoPageSpreadTransitionChanged();
}

void ImageDocumentController::notifyTwoPageSpreadTransitionChanged()
{
    notifyChanges(imageDocumentSpreadTransitionNotifications());
}

void ImageDocumentController::updateSpreadZoomState()
{
    if (!secondaryPageVisible()) {
        return;
    }

    m_spreadZoomController->updateFromPrimaryPresentation(rightToLeftReadingActive());
}

QSize ImageDocumentController::spreadImageSize() const
{
    return m_spreadZoomController->spreadImageSize();
}

bool ImageDocumentController::twoPageModeActive() const
{
    return m_spreadModeController->twoPageModeActive();
}

bool ImageDocumentController::rightToLeftReadingActive() const
{
    return m_spreadModeController->rightToLeftReadingActive();
}

bool ImageDocumentController::primaryPageIsWide() const
{
    return imageSpreadPageIsWide(m_presentationController->imageSize());
}

void ImageDocumentController::openImageAtRelativePageOffset(int offset)
{
    const int targetPage = imageSpreadRelativePageTarget(currentPageNumber(), imageCount(), offset);
    if (targetPage <= 0) {
        return;
    }

    openImageAtPage(targetPage);
}

void ImageDocumentController::notifyTwoPageModeChanged()
{
    notifyChanges(imageDocumentTwoPageModeNotifications());
}

void ImageDocumentController::notifySpreadZoomChanged()
{
    notifyChanges(imageDocumentSpreadZoomNotifications());
}

void ImageDocumentController::notifyRightToLeftReadingChanged()
{
    notifyChanges(imageDocumentRightToLeftReadingNotifications(secondaryPageVisible()));
}

void ImageDocumentController::notifyChanges(const std::vector<ImageDocumentChange> &changes)
{
    for (ImageDocumentChange change : changes) {
        notify(change);
    }
}

void ImageDocumentController::notify(ImageDocumentChange change)
{
    const ImageDocumentChangeDispatchPlan plan
        = imageDocumentChangeDispatchPlan(change, m_state.errorString().isEmpty());

    if (plan.finishSpreadTransition) {
        finishTwoPageSpreadTransition();
    }

    if (plan.refreshSecondaryPage) {
        refreshSecondaryPage();
    }
    if (plan.notifyRightToLeftReading) {
        notifyRightToLeftReadingChanged();
    }

    invokeIfSet(m_changeCallback, change);
}

void ImageDocumentController::clearImage()
{
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->clear();
    }
    finishTwoPageSpreadTransition();
    clearSecondaryPage();
    m_navigationController->cancelPageNavigationUpdate();
    m_state.clearDisplayedImageUrls();
    m_presentationController->clearImage();
    m_navigationController->clearPageNavigation();
    notifyRightToLeftReadingChanged();
}
}
