// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadpresentationcontroller.h"

#include "async/imagecallback.h"
#include "document/imagedocumentnotifications.h"
#include "presentation/imagepresentationcontroller.h"
#include "presentation/imagesecondarypagecontroller.h"
#include "presentation/imagespreadgeometry.h"
#include "presentation/imagespreadmodecontroller.h"
#include "presentation/imagespreadnavigation.h"

#include <utility>

namespace {
bool hasSpreadZoomStateChange(const KiriView::ImageZoomChangeSet &changes)
{
    return changes.imageSizeChanged || changes.viewportSizeChanged || changes.zoomModeChanged
        || changes.zoomPercentChanged || changes.displaySizeChanged
        || changes.maximumManualZoomPercentChanged || changes.scheduleVisibleTileDecode;
}

bool spreadZoomChangesAffectViewportFrame(const KiriView::ImageZoomChangeSet &changes)
{
    return changes.imageSizeChanged || changes.viewportSizeChanged || changes.displaySizeChanged;
}
}

namespace KiriView {
ImageSpreadPresentationController::ImageSpreadPresentationController(QObject *parent,
    RenderContextProvider renderContextProvider, ImageDocumentState &state,
    ImagePresentationController &primaryPresentation,
    ImageSpreadPresentationController::Callbacks callbacks,
    ImageDocumentPageCandidateProvider candidateProvider,
    ImageDecodeDependencies decodeDependencies, ImageCacheBudgets cacheBudgets)
    : m_state(state)
    , m_primaryPresentation(primaryPresentation)
    , m_callbacks(std::move(callbacks))
{
    RenderContextProvider secondaryRenderContextProvider = renderContextProvider;
    RenderContextProvider activeRenderContextProvider = std::move(renderContextProvider);
    m_secondaryPageController = std::make_unique<ImageSecondaryPageController>(parent,
        std::move(secondaryRenderContextProvider),
        ImageSecondaryPageController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](ImageSecondaryPageLoadResult result, const DisplayedImageLocation &location,
                const QSize &imageSize) {
                handleSecondaryPageLoadFinished(result, location, imageSize);
            },
            [this]() { notifyTwoPageModeChanged(); },
            [this](const QUrl &url) {
                if (!m_callbacks.findPredecodedImage) {
                    return std::optional<PredecodedImage>();
                }

                return m_callbacks.findPredecodedImage(url);
            },
        },
        std::move(candidateProvider), std::move(decodeDependencies), cacheBudgets);
    m_modeController = std::make_unique<ImageSpreadModeController>();
    m_activePresentation
        = std::make_unique<ImagePresentationActiveState>(std::move(activeRenderContextProvider),
            m_primaryPresentation, m_secondaryPageController->presentationController());
}

ImageSpreadPresentationController::~ImageSpreadPresentationController() { shutdown(); }

bool ImageSpreadPresentationController::transitionInProgress() const
{
    return m_activePresentation->transitionInProgress();
}

ImagePresentationTransitionState
ImageSpreadPresentationController::presentationTransitionState() const
{
    return m_activePresentation->transitionState();
}

ImageDocumentStatus ImageSpreadPresentationController::status(
    ImageDocumentStatus documentStatus) const
{
    return transitionInProgress() ? ImageDocumentStatus::Loading : documentStatus;
}

bool ImageSpreadPresentationController::loading(bool documentLoading) const
{
    return transitionInProgress() || documentLoading;
}

QSize ImageSpreadPresentationController::imageSize() const
{
    return m_activePresentation->imageSize();
}

QSize ImageSpreadPresentationController::primaryImageSize() const
{
    return secondaryPageVisible() ? m_primaryPresentation.imageSize()
                                  : m_activePresentation->imageSize();
}

QSize ImageSpreadPresentationController::secondaryImageSize() const
{
    return secondaryPageVisible() ? m_secondaryPageController->imageSize() : QSize();
}

QSizeF ImageSpreadPresentationController::displaySize() const
{
    return m_activePresentation->displaySize();
}

QSizeF ImageSpreadPresentationController::primaryDisplaySize() const
{
    return m_activePresentation->primaryDisplaySize();
}

QSizeF ImageSpreadPresentationController::secondaryDisplaySize() const
{
    return m_activePresentation->secondaryDisplaySize();
}

QPointF ImageSpreadPresentationController::viewportContentPosition() const
{
    return viewportFrame().contentPosition;
}

void ImageSpreadPresentationController::setViewportContentPosition(
    const QPointF &viewportContentPosition)
{
    refreshViewportFrameForContentPosition(viewportContentPosition);
}

ImageViewportCommand ImageSpreadPresentationController::requestViewportContentPosition(
    const QPointF &viewportContentPosition)
{
    const ImageViewportCommand command
        = m_activePresentation->requestViewportContentPosition(viewportContentPosition);
    applyViewportFrameVisibleItemRect(true);
    notify(ImageDocumentChange::ViewportFrame);
    return command;
}

bool ImageSpreadPresentationController::beginViewportCommandApplication(quint64 commandRevision)
{
    if (!m_activePresentation->beginViewportCommandApplication(commandRevision)) {
        notify(ImageDocumentChange::ViewportFrame);
        return false;
    }

    notify(ImageDocumentChange::ViewportFrame);
    return true;
}

bool ImageSpreadPresentationController::completeViewportCommandApplication(
    quint64 commandRevision, const QPointF &actualContentPosition)
{
    if (!m_activePresentation->completeViewportCommandApplication(
            commandRevision, actualContentPosition)) {
        notify(ImageDocumentChange::ViewportFrame);
        return false;
    }

    applyViewportFrameVisibleItemRect(true);
    notify(ImageDocumentChange::ViewportFrame);
    return true;
}

bool ImageSpreadPresentationController::acknowledgeViewportCommand(
    quint64 commandRevision, const QPointF &actualContentPosition)
{
    if (!m_activePresentation->acknowledgeViewportCommand(commandRevision, actualContentPosition)) {
        notify(ImageDocumentChange::ViewportFrame);
        return false;
    }

    applyViewportFrameVisibleItemRect(true);
    notify(ImageDocumentChange::ViewportFrame);
    return true;
}

bool ImageSpreadPresentationController::observeViewportContentPosition(
    const QPointF &contentPosition, ImageViewportObservationOrigin origin)
{
    if (!m_activePresentation->observeViewportContentPosition(contentPosition, origin)) {
        return false;
    }

    applyViewportFrameVisibleItemRect(true);
    notify(ImageDocumentChange::ViewportFrame);
    return true;
}

quint64 ImageSpreadPresentationController::viewportCommandRevision() const
{
    return m_activePresentation->viewportProjection().commandRevision;
}

quint64 ImageSpreadPresentationController::viewportAppliedCommandRevision() const
{
    return m_activePresentation->viewportProjection().appliedCommandRevision;
}

quint64 ImageSpreadPresentationController::viewportObservationRevision() const
{
    return m_activePresentation->viewportProjection().observationRevision;
}

ImageViewportCommandStatus ImageSpreadPresentationController::viewportCommandStatus() const
{
    return m_activePresentation->viewportProjection().status;
}

ImageViewportObservationOrigin ImageSpreadPresentationController::viewportObservationOrigin() const
{
    return m_activePresentation->viewportProjection().observationOrigin;
}

QSizeF ImageSpreadPresentationController::viewportContentSize() const
{
    return viewportFrame().contentSize;
}

QRectF ImageSpreadPresentationController::viewportImageRect() const
{
    return viewportFrame().imageRect;
}

bool ImageSpreadPresentationController::viewportHorizontallyPannable() const
{
    return viewportFrame().horizontalPannable;
}

bool ImageSpreadPresentationController::viewportVerticallyPannable() const
{
    return viewportFrame().verticalPannable;
}

bool ImageSpreadPresentationController::viewportPannable() const
{
    return viewportFrame().pannable;
}

QRectF ImageSpreadPresentationController::visibleItemRect() const
{
    return viewportFrame().visibleItemRect;
}

void ImageSpreadPresentationController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    refreshViewportFrameForContentPosition(
        QPointF(visibleItemRect.x() + viewportFrame().imageRect.x(),
            visibleItemRect.y() + viewportFrame().imageRect.y()),
        true);
}

qreal ImageSpreadPresentationController::zoomPercent() const
{
    return m_activePresentation->zoomPercent();
}

void ImageSpreadPresentationController::setZoomPercent(qreal zoomPercent)
{
    applyActivePresentationChanges(updateZoomState(), false);
    applyActivePresentationChanges(m_activePresentation->setZoomPercent(zoomPercent));
}

ImageZoomMode ImageSpreadPresentationController::zoomMode() const
{
    return m_activePresentation->zoomMode();
}

qreal ImageSpreadPresentationController::maximumManualZoomPercent() const
{
    return m_activePresentation->maximumManualZoomPercent();
}

qreal ImageSpreadPresentationController::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_activePresentation->clampedManualZoomPercent(zoomPercent);
}

qreal ImageSpreadPresentationController::steppedManualZoomPercent(qreal stepCount) const
{
    return m_activePresentation->steppedManualZoomPercent(stepCount);
}

int ImageSpreadPresentationController::rotationDegrees() const
{
    return m_activePresentation->rotationDegrees();
}

int ImageSpreadPresentationController::currentLastPageNumber() const
{
    return m_secondaryPageRefresh.currentLastPageNumber(pageNavigationContext());
}

ImageDocumentPageActiveNavigationSnapshot
ImageSpreadPresentationController::activeNavigationSnapshot() const
{
    return m_secondaryPageRefresh.activeNavigationSnapshot(pageNavigationContext());
}

ImageSpreadPageNavigationTarget
ImageSpreadPresentationController::imageDocumentPageNavigationTarget(
    NavigationDirection direction) const
{
    return m_secondaryPageRefresh.pageNavigationTarget(direction, pageNavigationContext());
}

int ImageSpreadPresentationController::relativePageNavigationTarget(int offset) const
{
    return m_secondaryPageRefresh.relativePageNavigationTarget(offset, pageNavigationContext());
}

bool ImageSpreadPresentationController::twoPageModeEnabled() const
{
    return m_modeController->twoPageModeEnabled();
}

void ImageSpreadPresentationController::setTwoPageModeEnabled(bool enabled)
{
    const ImageSpreadTwoPageModeChange change
        = m_modeController->setTwoPageModeEnabled(enabled, secondaryPageVisible());
    if (!change.changed) {
        return;
    }

    if (enabled) {
        if (m_activePresentation->resetRotation()) {
            applyActivePresentationChanges(updateZoomState());
            notifyChanges({ ImageDocumentChange::Rotation, ImageDocumentChange::Repaint });
        }
    } else {
        m_activePresentation->setMode(ImagePresentationActiveMode::SinglePage);
        applyActivePresentationChanges(updateZoomState(), false);
    }
    if (change.finishTransition) {
        finishTransition();
    }
    if (change.clearSecondaryPage) {
        clearSecondaryPage();
    }
    if (change.refreshSecondaryPage) {
        refreshSecondaryPage();
    }
    if (change.notifyTwoPageMode) {
        notifyTwoPageModeChanged();
    }
}

bool ImageSpreadPresentationController::twoPageModeAvailable() const
{
    return m_modeController->twoPageModeAvailable(readingAvailability());
}

bool ImageSpreadPresentationController::twoPageModeActive() const
{
    return m_modeController->twoPageModeActive(readingAvailability());
}

bool ImageSpreadPresentationController::rightToLeftReadingEnabled() const
{
    return m_modeController->rightToLeftReadingEnabled();
}

void ImageSpreadPresentationController::setRightToLeftReadingEnabled(bool enabled)
{
    if (!m_modeController->setRightToLeftReadingEnabled(enabled)) {
        return;
    }

    if (secondaryPageVisible()) {
        applyViewportFrameVisibleItemRect(true);
    }
    notifyRightToLeftReadingChanged();
}

bool ImageSpreadPresentationController::rightToLeftReadingAvailable() const
{
    return m_modeController->rightToLeftReadingAvailable(readingAvailability());
}

bool ImageSpreadPresentationController::rightToLeftReadingActive() const
{
    return m_modeController->rightToLeftReadingActive(readingAvailability());
}

bool ImageSpreadPresentationController::secondaryPageVisible() const
{
    if (presentationTransitionState() == ImagePresentationTransitionState::PreviousActive) {
        return m_activePresentation->mode() == ImagePresentationActiveMode::TwoPageSpread
            && m_secondaryPageController->visible();
    }

    return twoPageModeActive() && m_secondaryPageController->visible();
}

std::optional<DisplayedPredecodeImage>
ImageSpreadPresentationController::secondaryDisplayedPredecodeImage() const
{
    if (!secondaryPageVisible()) {
        return std::nullopt;
    }

    const ImagePresentationController &presentation
        = m_secondaryPageController->presentationController();
    return DisplayedPredecodeImage {
        m_secondaryPageController->displayedImageLocation(),
        presentation.isPredecodeCacheable(),
        presentation.staticImage(),
        {},
    };
}

DisplayedImageRenderSnapshot ImageSpreadPresentationController::renderSnapshot(
    DisplayedPageRole role) const
{
    if (presentationTransitionState()
        == ImagePresentationTransitionState::TransitioningPlaceholder) {
        return {};
    }

    if (role == DisplayedPageRole::Secondary) {
        DisplayedImageRenderSnapshot snapshot = m_secondaryPageController->renderSnapshot();
        const ImagePresentationPageProjection projection
            = m_activePresentation->secondaryProjection(rightToLeftReadingActive());
        snapshot.displaySize = projection.displaySize;
        snapshot.visibleItemRect = secondaryPageVisible() ? projection.visibleItemRect : QRectF();
        snapshot.rotationDegrees = projection.rotationDegrees;
        snapshot.pageRole = DisplayedPageRole::Secondary;
        return snapshot;
    }

    DisplayedImageRenderSnapshot snapshot = m_primaryPresentation.renderSnapshot();
    const ImagePresentationPageProjection projection
        = m_activePresentation->primaryProjection(rightToLeftReadingActive());
    snapshot.imageSize = primaryImageSize();
    snapshot.displaySize = projection.displaySize;
    snapshot.visibleItemRect = projection.visibleItemRect;
    snapshot.rotationDegrees = projection.rotationDegrees;
    snapshot.pageRole = DisplayedPageRole::Primary;
    return snapshot;
}

void ImageSpreadPresentationController::setViewportSize(const QSizeF &viewportSize)
{
    m_primaryPresentation.setViewportSize(viewportSize);
    m_secondaryPageController->setViewportSize(viewportSize);
    applyActivePresentationChanges(updateZoomState());
}

void ImageSpreadPresentationController::resetZoom()
{
    applyActivePresentationChanges(updateZoomState(), false);
    applyActivePresentationChanges(m_activePresentation->resetZoom());
}

void ImageSpreadPresentationController::setFitMode(ImageZoomMode zoomMode)
{
    applyActivePresentationChanges(updateZoomState(), false);
    applyActivePresentationChanges(m_activePresentation->setFitMode(zoomMode));
}

void ImageSpreadPresentationController::rotateClockwise()
{
    if (twoPageModeActive()) {
        return;
    }

    if (!m_activePresentation->rotateClockwise()) {
        return;
    }

    applyActivePresentationChanges(updateZoomState());
    notifyChanges({ ImageDocumentChange::Rotation, ImageDocumentChange::ImageSize,
        ImageDocumentChange::Repaint });
}

void ImageSpreadPresentationController::rotateCounterclockwise()
{
    if (twoPageModeActive()) {
        return;
    }

    if (!m_activePresentation->rotateCounterclockwise()) {
        return;
    }

    applyActivePresentationChanges(updateZoomState());
    notifyChanges({ ImageDocumentChange::Rotation, ImageDocumentChange::ImageSize,
        ImageDocumentChange::Repaint });
}

void ImageSpreadPresentationController::updateRenderContext()
{
    m_primaryPresentation.updateRenderContext();
    m_secondaryPageController->updateRenderContext();
    applyActivePresentationChanges(m_activePresentation->updateRenderContext());
}

void ImageSpreadPresentationController::refreshSecondaryPage()
{
    m_secondaryPageRefresh.cachePageSize(m_state.displayedUrl(), m_primaryPresentation.imageSize());

    auto finishWithPrimaryPage = [this]() {
        const bool wasVisible = secondaryPageVisible();
        m_activePresentation->setMode(ImagePresentationActiveMode::SinglePage);
        discardSecondaryPage();
        applyActivePresentationChanges(updateZoomState(), false);
        if (wasVisible) {
            notifyTwoPageModeChanged();
            scheduleAdjacentPredecode();
        }
        finishTransition();
    };

    const ImageSpreadSecondaryPageRefreshResult result
        = m_secondaryPageRefresh.planRefresh(ImageSpreadSecondaryPageRefreshRequest {
            twoPageModeActive(),
            primaryPageIsWide(),
            secondaryPageVisible(),
            m_secondaryPageController->displayedImageLocation().imageUrl(),
            pageNavigationSnapshot(),
        });
    if (result.action == ImageSpreadSecondaryPageRefreshAction::PrimaryOnly) {
        finishWithPrimaryPage();
        return;
    }
    if (result.action == ImageSpreadSecondaryPageRefreshAction::KeepCurrentSecondary) {
        return;
    }

    if (result.targetUrl.isEmpty()) {
        finishWithPrimaryPage();
        return;
    }

    startSecondaryPageLoad(result.targetUrl);
}

void ImageSpreadPresentationController::handleDocumentChange(ImageDocumentChange change)
{
    if (change == ImageDocumentChange::ErrorString && !m_state.errorString().isEmpty()) {
        finishTransition();
    }

    if (change == ImageDocumentChange::ImageSize || change == ImageDocumentChange::ViewportSize
        || change == ImageDocumentChange::DisplaySize || change == ImageDocumentChange::Rotation
        || change == ImageDocumentChange::TwoPageMode) {
        applyActivePresentationChanges(updateZoomState(), false);
        refreshViewportFrame(change == ImageDocumentChange::TwoPageMode);
    }

    if (change == ImageDocumentChange::PageNavigation) {
        if (m_secondaryPageRefresh.primarySelectionMatchesDisplayed(
                pageNavigationSnapshot(), m_state.displayedUrl())) {
            refreshSecondaryPage();
        }
        notifyRightToLeftReadingChanged();
    }
}

bool ImageSpreadPresentationController::shouldBeginTransition(int targetPageNumber) const
{
    return m_secondaryPageRefresh.shouldBeginNavigationTransition(
        targetPageNumber, pageNavigationContext());
}

void ImageSpreadPresentationController::beginTransition()
{
    if (!m_activePresentation->beginTransition()) {
        notifyTransitionChanged();
        return;
    }

    notifyTransitionChanged();
}

void ImageSpreadPresentationController::showTransitionPlaceholder()
{
    if (!m_activePresentation->showTransitionPlaceholder()) {
        return;
    }

    notifyTransitionChanged();
}

void ImageSpreadPresentationController::finishTransition()
{
    if (!m_activePresentation->finishTransition()) {
        return;
    }

    notifyTransitionChanged();
}

void ImageSpreadPresentationController::abortTransition()
{
    if (!m_activePresentation->abortTransition()) {
        return;
    }

    notifyTransitionChanged();
}

void ImageSpreadPresentationController::clearSecondaryPage()
{
    if (presentationTransitionState() == ImagePresentationTransitionState::PreviousActive) {
        return;
    }

    discardSecondaryPage();
}

void ImageSpreadPresentationController::discardSecondaryPage()
{
    m_secondaryPageController->clear();
}

void ImageSpreadPresentationController::shutdown()
{
    if (m_secondaryPageController == nullptr) {
        return;
    }

    m_secondaryPageController->stopAnimation();
    m_secondaryPageController->cancel();
}

void ImageSpreadPresentationController::resetRightToLeftReading()
{
    m_modeController->resetRightToLeftReading();
}

void ImageSpreadPresentationController::notifyRightToLeftReadingChanged()
{
    notifyChanges(imageDocumentRightToLeftReadingNotifications(secondaryPageVisible()));
}

void ImageSpreadPresentationController::startSecondaryPageLoad(const QUrl &url)
{
    m_secondaryPageController->startLoad(url, m_state.displayedOpenedCollectionScope(),
        m_primaryPresentation.firstDisplayDecodeContext());
    notifyTwoPageModeChanged();
}

void ImageSpreadPresentationController::handleSecondaryPageLoadFinished(
    ImageSecondaryPageLoadResult result, const DisplayedImageLocation &location,
    const QSize &imageSize)
{
    if (result != ImageSecondaryPageLoadResult::Failed) {
        m_secondaryPageRefresh.cachePageSize(location.imageUrl(), imageSize);
    }

    if (result == ImageSecondaryPageLoadResult::Visible) {
        finishSecondaryPageVisible();
        return;
    }

    finishSecondaryPageAsPrimaryOnly();
}

void ImageSpreadPresentationController::finishSecondaryPageAsPrimaryOnly()
{
    discardSecondaryPage();
    m_activePresentation->setMode(ImagePresentationActiveMode::SinglePage);
    applyActivePresentationChanges(updateZoomState(), false);
    finishTransition();
    notifyTwoPageModeChanged();
    scheduleAdjacentPredecode();
}

void ImageSpreadPresentationController::finishSecondaryPageVisible()
{
    m_activePresentation->setMode(ImagePresentationActiveMode::TwoPageSpread);
    applyActivePresentationChanges(updateZoomState(), false);
    finishTransition();
    notifyTwoPageModeChanged();
    scheduleAdjacentPredecode();
}

void ImageSpreadPresentationController::notifyTransitionChanged()
{
    notifyChanges(imageDocumentSpreadTransitionNotifications());
}

ImageZoomChangeSet ImageSpreadPresentationController::updateZoomState()
{
    const QUrl scopeRootUrl = m_state.displayedOpenedCollectionScope().rootUrl();
    const bool activeZoomScopeChanged = m_activeZoomScopeRootUrl != scopeRootUrl;
    m_activeZoomScopeRootUrl = scopeRootUrl;
    return m_activePresentation->updateFromPageState(activeZoomScopeChanged);
}

QSize ImageSpreadPresentationController::spreadImageSize() const
{
    return m_activePresentation->spreadImageSize();
}

bool ImageSpreadPresentationController::primaryPageIsWide() const
{
    return imageSpreadPageIsWide(m_primaryPresentation.imageSize());
}

ImageSpreadReadingAvailability ImageSpreadPresentationController::readingAvailability() const
{
    const DisplayedImageLocation &location = m_state.displayedImageLocation();
    return ImageSpreadReadingAvailability { m_primaryPresentation.hasImage(), !location.isEmpty(),
        location.openedCollectionScope().isComicBook() };
}

bool ImageSpreadPresentationController::secondaryPageVisibleForNavigation() const
{
    if (presentationTransitionState() == ImagePresentationTransitionState::PreviousActive) {
        return false;
    }

    return secondaryPageVisible();
}

void ImageSpreadPresentationController::scheduleAdjacentPredecode()
{
    invokeIfSet(m_callbacks.scheduleAdjacentPredecode);
}

ImageSpreadPageNavigationContext ImageSpreadPresentationController::pageNavigationContext() const
{
    return ImageSpreadPageNavigationContext { twoPageModeActive(),
        secondaryPageVisibleForNavigation(), pageNavigationSnapshot() };
}

ImageDocumentPageNavigationSnapshot
ImageSpreadPresentationController::pageNavigationSnapshot() const
{
    return m_callbacks.pageNavigationSnapshot ? m_callbacks.pageNavigationSnapshot()
                                              : ImageDocumentPageNavigationSnapshot {};
}

void ImageSpreadPresentationController::notifyTwoPageModeChanged()
{
    notifyChanges(imageDocumentTwoPageModeNotifications());
}

void ImageSpreadPresentationController::applyActivePresentationChanges(
    const ImageZoomChangeSet &changes, bool notifyPublicChanges)
{
    if (hasSpreadZoomStateChange(changes)) {
        if (spreadZoomChangesAffectViewportFrame(changes)) {
            refreshViewportFrame();
        }
        m_activePresentation->applyVisibleItemRects(rightToLeftReadingActive());
    }

    if (notifyPublicChanges) {
        notifyActivePresentationZoomChanged(changes);
    }
}

void ImageSpreadPresentationController::notifyActivePresentationZoomChanged(
    const ImageZoomChangeSet &changes)
{
    notifyChanges(imageDocumentSpreadZoomNotifications(changes));
}

bool ImageSpreadPresentationController::refreshViewportFrame(bool forceApplyVisibleItemRect)
{
    const QRectF previousVisibleItemRect = viewportFrame().visibleItemRect;
    const ImageViewportFrame previousFrame = viewportFrame();
    m_activePresentation->setViewportGeometry(m_primaryPresentation.viewportSize(), displaySize());
    const bool frameChanged = viewportFrame() != previousFrame;
    if (!frameChanged && !forceApplyVisibleItemRect) {
        return false;
    }

    applyViewportFrameVisibleItemRect(
        forceApplyVisibleItemRect || previousVisibleItemRect != viewportFrame().visibleItemRect);
    if (frameChanged) {
        notify(ImageDocumentChange::ViewportFrame);
    }
    return frameChanged;
}

bool ImageSpreadPresentationController::refreshViewportFrameForContentPosition(
    const QPointF &contentPosition, bool forceApplyVisibleItemRect)
{
    const QRectF previousVisibleItemRect = viewportFrame().visibleItemRect;
    const ImageViewportFrame previousFrame = viewportFrame();
    m_activePresentation->setViewportGeometry(m_primaryPresentation.viewportSize(), displaySize());
    m_activePresentation->requestViewportContentPosition(contentPosition);
    const bool frameChanged = viewportFrame() != previousFrame;
    if (!frameChanged && !forceApplyVisibleItemRect) {
        return false;
    }

    applyViewportFrameVisibleItemRect(
        forceApplyVisibleItemRect || previousVisibleItemRect != viewportFrame().visibleItemRect);
    if (frameChanged) {
        notify(ImageDocumentChange::ViewportFrame);
    }
    return frameChanged;
}

void ImageSpreadPresentationController::applyViewportFrameVisibleItemRect(bool force)
{
    if (!force) {
        return;
    }

    m_activePresentation->applyVisibleItemRects(rightToLeftReadingActive());
    notify(ImageDocumentChange::VisibleItemRect);
}

const ImageViewportFrame &ImageSpreadPresentationController::viewportFrame() const
{
    return m_activePresentation->viewportFrame();
}

void ImageSpreadPresentationController::notifyChanges(
    const std::vector<ImageDocumentChange> &changes)
{
    for (ImageDocumentChange change : changes) {
        notify(change);
    }
}

void ImageSpreadPresentationController::notify(ImageDocumentChange change)
{
    invokeIfSet(m_callbacks.change, change);
}
}
