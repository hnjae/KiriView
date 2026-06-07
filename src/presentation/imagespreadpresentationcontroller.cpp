// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadpresentationcontroller.h"

#include "async/imagecallback.h"
#include "document/imagedocumentnotifications.h"
#include "location/imagedocumentlocation.h"
#include "presentation/imagepagesurfacecontroller.h"
#include "presentation/imagesecondarypagecontroller.h"
#include "presentation/imagespreadgeometry.h"
#include "presentation/imagespreadmodepolicy.h"
#include "presentation/imagespreadnavigation.h"

#include <utility>

namespace {
bool hasSpreadZoomStateChange(const KiriView::ImageZoomChangeSet &changes)
{
    return changes.imageSizeChanged || changes.viewportSizeChanged || changes.zoomModeChanged
        || changes.zoomPercentChanged || changes.displaySizeChanged
        || changes.maximumManualZoomPercentChanged || changes.displayProjectionUpdateNeeded;
}

KiriView::ImagePresentationScopeKey presentationScopeKeyForLocation(
    const KiriView::DisplayedImageLocation &location)
{
    const QUrl openedCollectionScopeUrl = KiriView::zoomScopeUrlForLocation(location);
    if (!openedCollectionScopeUrl.isEmpty()) {
        return KiriView::ImagePresentationScopeKey::openedCollection(openedCollectionScopeUrl);
    }

    return KiriView::ImagePresentationScopeKey::directImage(location.imageUrl());
}
}

namespace KiriView {
ImageSpreadPresentationController::ImageSpreadPresentationController(QObject *parent,
    RenderContextProvider renderContextProvider, ImageDocumentState &state,
    ImagePageSurfaceController &primaryPageSurface, ImagePresentationRuntime &presentationRuntime,
    ImageSpreadPresentationController::Callbacks callbacks,
    ImageDocumentPageCandidateProvider candidateProvider,
    ImageDecodeDependencies decodeDependencies, ImageCacheBudgets cacheBudgets)
    : m_state(state)
    , m_primaryPageSurface(primaryPageSurface)
    , m_presentationRuntime(presentationRuntime)
    , m_callbacks(std::move(callbacks))
{
    m_secondaryPageController
        = std::make_unique<ImageSecondaryPageController>(parent, std::move(renderContextProvider),
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
}

ImageSpreadPresentationController::~ImageSpreadPresentationController() { shutdown(); }

bool ImageSpreadPresentationController::transitionInProgress() const
{
    return m_presentationRuntime.transitionInProgress();
}

ImagePresentationTransitionState
ImageSpreadPresentationController::presentationTransitionState() const
{
    return m_presentationRuntime.transitionState();
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
    return m_presentationRuntime.imageSize();
}

QSize ImageSpreadPresentationController::primaryImageSize() const
{
    return secondaryPageVisible() ? m_primaryPageSurface.imageSize()
                                  : m_presentationRuntime.imageSize();
}

QSize ImageSpreadPresentationController::secondaryImageSize() const
{
    return secondaryPageVisible() ? m_secondaryPageController->imageSize() : QSize();
}

QSizeF ImageSpreadPresentationController::displaySize() const
{
    return m_presentationRuntime.displaySize();
}

QSizeF ImageSpreadPresentationController::primaryDisplaySize() const
{
    return m_presentationRuntime.primaryDisplaySize();
}

QSizeF ImageSpreadPresentationController::secondaryDisplaySize() const
{
    return m_presentationRuntime.secondaryDisplaySize();
}

QPointF ImageSpreadPresentationController::viewportContentPosition() const
{
    return viewportFrame().contentPosition;
}

ImageViewportCommand ImageSpreadPresentationController::requestViewportContentPosition(
    const QPointF &viewportContentPosition)
{
    const ImageViewportCommand command
        = m_presentationRuntime.requestViewportContentPosition(viewportContentPosition);
    updateDisplayProjections();
    notify(ImageDocumentChange::VisibleItemRect);
    notify(ImageDocumentChange::ViewportFrame);
    return command;
}

bool ImageSpreadPresentationController::beginViewportCommandApplication(quint64 commandRevision)
{
    const bool changed = m_presentationRuntime.beginViewportCommandApplication(commandRevision);
    notify(ImageDocumentChange::ViewportFrame);
    return changed;
}

bool ImageSpreadPresentationController::completeViewportCommandApplication(
    quint64 commandRevision, const QPointF &actualContentPosition)
{
    const bool changed = m_presentationRuntime.completeViewportCommandApplication(
        commandRevision, actualContentPosition);
    if (changed) {
        updateDisplayProjections();
        notify(ImageDocumentChange::VisibleItemRect);
    }
    notify(ImageDocumentChange::ViewportFrame);
    return changed;
}

bool ImageSpreadPresentationController::acknowledgeViewportCommand(
    quint64 commandRevision, const QPointF &actualContentPosition)
{
    const bool changed
        = m_presentationRuntime.acknowledgeViewportCommand(commandRevision, actualContentPosition);
    if (changed) {
        updateDisplayProjections();
        notify(ImageDocumentChange::VisibleItemRect);
    }
    notify(ImageDocumentChange::ViewportFrame);
    return changed;
}

bool ImageSpreadPresentationController::observeViewportContentPosition(
    const QPointF &contentPosition, ImageViewportObservationOrigin origin)
{
    if (!m_presentationRuntime.observeViewportContentPosition(contentPosition, origin)) {
        return false;
    }

    updateDisplayProjections();
    notify(ImageDocumentChange::VisibleItemRect);
    notify(ImageDocumentChange::ViewportFrame);
    return true;
}

quint64 ImageSpreadPresentationController::viewportCommandRevision() const
{
    return m_presentationRuntime.viewportProjection().commandRevision;
}

quint64 ImageSpreadPresentationController::viewportAppliedCommandRevision() const
{
    return m_presentationRuntime.viewportProjection().appliedCommandRevision;
}

quint64 ImageSpreadPresentationController::viewportObservationRevision() const
{
    return m_presentationRuntime.viewportProjection().observationRevision;
}

ImageViewportCommandStatus ImageSpreadPresentationController::viewportCommandStatus() const
{
    return m_presentationRuntime.viewportProjection().status;
}

ImageViewportObservationOrigin ImageSpreadPresentationController::viewportObservationOrigin() const
{
    return m_presentationRuntime.viewportProjection().observationOrigin;
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

qreal ImageSpreadPresentationController::zoomPercent() const
{
    return m_presentationRuntime.zoomPercent();
}

void ImageSpreadPresentationController::requestManualZoomPercent(qreal zoomPercent)
{
    applyActivePresentationChanges(m_presentationRuntime.setZoomPercent(zoomPercent));
}

ImageZoomMode ImageSpreadPresentationController::zoomMode() const
{
    return m_presentationRuntime.zoomMode();
}

ImageZoomMode ImageSpreadPresentationController::fitModeSelection() const
{
    return m_presentationRuntime.fitModeSelection();
}

qreal ImageSpreadPresentationController::maximumManualZoomPercent() const
{
    return m_presentationRuntime.maximumManualZoomPercent();
}

qreal ImageSpreadPresentationController::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_presentationRuntime.clampedManualZoomPercent(zoomPercent);
}

qreal ImageSpreadPresentationController::steppedManualZoomPercent(qreal stepCount) const
{
    return m_presentationRuntime.steppedManualZoomPercent(stepCount);
}

int ImageSpreadPresentationController::rotationDegrees() const
{
    return m_presentationRuntime.rotationDegrees();
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
    return m_presentationRuntime.twoPageModeEnabled();
}

void ImageSpreadPresentationController::setTwoPageModeEnabled(bool enabled)
{
    const ImageSpreadTwoPageModeChange change
        = imageSpreadTwoPageModeChange(twoPageModeEnabled(), enabled, secondaryPageVisible());
    if (!change.changed) {
        return;
    }

    m_presentationRuntime.setTwoPageModeEnabled(enabled);
    if (enabled) {
        const ImagePresentationRotationChange rotation
            = m_presentationRuntime.resetRotationChange();
        if (rotation.changed) {
            applyActivePresentationChanges(rotation.zoomChanges);
            notifyChanges({ ImageDocumentChange::Rotation, ImageDocumentChange::DisplaySource });
        }
    } else {
        applyActivePresentationChanges(
            m_presentationRuntime.setMode(ImagePresentationMode::SinglePage), false);
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
    return imageSpreadReadingControlsAvailable(readingAvailability());
}

bool ImageSpreadPresentationController::twoPageModeActive() const
{
    return twoPageModeEnabled() && twoPageModeAvailable();
}

bool ImageSpreadPresentationController::rightToLeftReadingEnabled() const
{
    return m_presentationRuntime.rightToLeftReadingEnabled();
}

void ImageSpreadPresentationController::setRightToLeftReadingEnabled(bool enabled)
{
    if (!m_presentationRuntime.setRightToLeftReadingEnabled(enabled)) {
        return;
    }

    if (secondaryPageVisible()) {
        updateDisplayProjections();
    }
    notifyRightToLeftReadingChanged();
}

bool ImageSpreadPresentationController::rightToLeftReadingAvailable() const
{
    return imageSpreadReadingControlsAvailable(readingAvailability());
}

bool ImageSpreadPresentationController::rightToLeftReadingActive() const
{
    return rightToLeftReadingEnabled() && rightToLeftReadingAvailable();
}

bool ImageSpreadPresentationController::secondaryPageVisible() const
{
    return m_presentationRuntime.secondaryPageVisible();
}

std::optional<DisplayedPredecodeImage>
ImageSpreadPresentationController::secondaryDisplayedPredecodeImage() const
{
    if (!secondaryPageVisible()) {
        return std::nullopt;
    }

    const ImagePageSurfaceController &pageSurface
        = m_secondaryPageController->pageSurfaceController();
    return DisplayedPredecodeImage {
        m_secondaryPageController->displayedImageLocation(),
        pageSurface.isPredecodeCacheable(),
        pageSurface.displayImage(),
        {},
    };
}

ImageDisplaySourceProjection ImageSpreadPresentationController::displaySourceProjection(
    DisplayedPageRole role) const
{
    return m_presentationRuntime.displaySourceProjection(role);
}

void ImageSpreadPresentationController::acknowledgeDisplayImageLoad(DisplayedPageRole role,
    const QUrl &providerUrl, quint64 revision, const QString &sourceIdentity,
    ImageDisplayLoadOutcome outcome)
{
    bool accepted = false;
    switch (role) {
    case DisplayedPageRole::Primary:
        accepted = m_primaryPageSurface.acknowledgeDisplayImageLoad(
            providerUrl, revision, sourceIdentity, outcome);
        break;
    case DisplayedPageRole::Secondary:
        if (m_secondaryPageController != nullptr) {
            accepted
                = m_secondaryPageController->pageSurfaceController().acknowledgeDisplayImageLoad(
                    providerUrl, revision, sourceIdentity, outcome);
        }
        break;
    }

    if (accepted && updatePresentationPageSlot(role)) {
        updateDisplayProjections();
        notify(ImageDocumentChange::DisplaySource);
    }
}

void ImageSpreadPresentationController::acknowledgeStillImageDisplayLoad(DisplayedPageRole role,
    const QUrl &providerUrl, quint64 revision, const QString &sourceIdentity,
    ImageDisplayLoadOutcome outcome)
{
    bool accepted = false;
    switch (role) {
    case DisplayedPageRole::Primary:
        accepted = m_primaryPageSurface.acknowledgeStillImageDisplayLoad(
            providerUrl, revision, sourceIdentity, outcome);
        break;
    case DisplayedPageRole::Secondary:
        if (m_secondaryPageController != nullptr) {
            accepted = m_secondaryPageController->pageSurfaceController()
                           .acknowledgeStillImageDisplayLoad(
                               providerUrl, revision, sourceIdentity, outcome);
        }
        break;
    }

    if (accepted && updatePresentationPageSlot(role)) {
        updateDisplayProjections();
        notify(ImageDocumentChange::DisplaySource);
    }
}

void ImageSpreadPresentationController::commitPrimaryPageSlot(
    const DisplayedImageLocation &location)
{
    const ImageZoomChangeSet changes = m_presentationRuntime.commitPrimaryPageSlot(
        m_primaryPageSurface.snapshot(), presentationScopeKeyForLocation(location));
    updateDisplayProjections();
    std::vector<ImageDocumentChange> notifications
        = imageDocumentPresentationZoomNotifications(changes);
    notifications.push_back(ImageDocumentChange::VisibleItemRect);
    notifications.push_back(ImageDocumentChange::ViewportFrame);
    notifications.push_back(ImageDocumentChange::DisplaySource);
    notifyChanges(notifications);
}

void ImageSpreadPresentationController::clearPrimaryPageSlot()
{
    m_presentationRuntime.clearPrimaryPageSlot();
    notifyChanges({ ImageDocumentChange::ImageSize, ImageDocumentChange::DisplaySize,
        ImageDocumentChange::ZoomPercent, ImageDocumentChange::ZoomMode,
        ImageDocumentChange::MaximumManualZoomPercent, ImageDocumentChange::VisibleItemRect,
        ImageDocumentChange::ViewportFrame, ImageDocumentChange::DisplaySource });
}

void ImageSpreadPresentationController::setViewportSize(const QSizeF &viewportSize)
{
    applyActivePresentationChanges(m_presentationRuntime.setViewportSize(viewportSize));
}

void ImageSpreadPresentationController::resetZoom()
{
    applyActivePresentationChanges(m_presentationRuntime.resetZoom());
}

void ImageSpreadPresentationController::setFitMode(ImageZoomMode zoomMode)
{
    applyActivePresentationChanges(m_presentationRuntime.setFitMode(zoomMode));
}

void ImageSpreadPresentationController::rotateClockwise()
{
    if (twoPageModeActive()) {
        return;
    }

    const ImagePresentationRotationChange rotation = m_presentationRuntime.rotateClockwiseChange();
    if (!rotation.changed) {
        return;
    }

    applyActivePresentationChanges(rotation.zoomChanges);
    notifyChanges({ ImageDocumentChange::Rotation, ImageDocumentChange::ImageSize,
        ImageDocumentChange::DisplaySource });
}

void ImageSpreadPresentationController::rotateCounterclockwise()
{
    if (twoPageModeActive()) {
        return;
    }

    const ImagePresentationRotationChange rotation
        = m_presentationRuntime.rotateCounterclockwiseChange();
    if (!rotation.changed) {
        return;
    }

    applyActivePresentationChanges(rotation.zoomChanges);
    notifyChanges({ ImageDocumentChange::Rotation, ImageDocumentChange::ImageSize,
        ImageDocumentChange::DisplaySource });
}

void ImageSpreadPresentationController::updateRenderContext()
{
    applyActivePresentationChanges(m_presentationRuntime.updateRenderContext());
}

void ImageSpreadPresentationController::refreshSecondaryPage()
{
    m_secondaryPageRefresh.cachePageSize(m_state.displayedUrl(), m_primaryPageSurface.imageSize());

    auto finishWithPrimaryPage = [this]() {
        const bool wasVisible = secondaryPageVisible();
        applyActivePresentationChanges(
            m_presentationRuntime.setMode(ImagePresentationMode::SinglePage), false);
        discardSecondaryPage();
        m_presentationRuntime.clearSecondaryPageSlot();
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
    if (!m_presentationRuntime.beginTransition()) {
        notifyTransitionChanged();
        return;
    }

    notifyTransitionChanged();
}

void ImageSpreadPresentationController::showTransitionPlaceholder()
{
    if (!m_presentationRuntime.showTransitionPlaceholder()) {
        return;
    }

    notifyTransitionChanged();
}

void ImageSpreadPresentationController::finishTransition()
{
    if (!m_presentationRuntime.finishTransition()) {
        return;
    }

    notifyTransitionChanged();
}

void ImageSpreadPresentationController::abortTransition()
{
    if (!m_presentationRuntime.abortTransition()) {
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
    m_presentationRuntime.clearSecondaryPageSlot();
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
    m_presentationRuntime.resetRightToLeftReading();
}

void ImageSpreadPresentationController::notifyRightToLeftReadingChanged()
{
    notifyChanges(imageDocumentRightToLeftReadingNotifications(secondaryPageVisible()));
}

void ImageSpreadPresentationController::startSecondaryPageLoad(const QUrl &url)
{
    m_secondaryPageController->startLoad(url, m_state.displayedOpenedCollectionScope(),
        m_presentationRuntime.firstDisplayDecodeContext());
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
    m_presentationRuntime.clearSecondaryPageSlot();
    applyActivePresentationChanges(
        m_presentationRuntime.setMode(ImagePresentationMode::SinglePage), false);
    finishTransition();
    notifyTwoPageModeChanged();
    scheduleAdjacentPredecode();
}

void ImageSpreadPresentationController::finishSecondaryPageVisible()
{
    m_presentationRuntime.setSecondaryPageVisible(true);
    applyActivePresentationChanges(m_presentationRuntime.commitSecondaryPageSlot(
                                       m_secondaryPageController->pageSlotSnapshot()),
        false);
    applyActivePresentationChanges(
        m_presentationRuntime.setMode(ImagePresentationMode::TwoPageSpread), false);
    finishTransition();
    notifyTwoPageModeChanged();
    scheduleAdjacentPredecode();
}

void ImageSpreadPresentationController::notifyTransitionChanged()
{
    notifyChanges(imageDocumentSpreadTransitionNotifications());
}

QSize ImageSpreadPresentationController::spreadImageSize() const
{
    return m_presentationRuntime.spreadImageSize();
}

bool ImageSpreadPresentationController::primaryPageIsWide() const
{
    return imageSpreadPageIsWide(m_primaryPageSurface.imageSize());
}

ImageSpreadReadingAvailability ImageSpreadPresentationController::readingAvailability() const
{
    const DisplayedImageLocation &location = m_state.displayedImageLocation();
    return ImageSpreadReadingAvailability { m_primaryPageSurface.hasImage(), !location.isEmpty(),
        location.openedCollectionScope().isComicBook() };
}

bool ImageSpreadPresentationController::secondaryPageVisibleForNavigation() const
{
    if (presentationTransitionState() == ImagePresentationTransitionState::PreviousActive) {
        return false;
    }

    return secondaryPageVisible();
}

ImageSpreadPageNavigationContext ImageSpreadPresentationController::pageNavigationContext() const
{
    return ImageSpreadPageNavigationContext { twoPageModeActive(),
        secondaryPageVisibleForNavigation(), pageNavigationSnapshot() };
}

void ImageSpreadPresentationController::scheduleAdjacentPredecode()
{
    invokeIfSet(m_callbacks.scheduleAdjacentPredecode);
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
        updateDisplayProjections();
        notify(ImageDocumentChange::VisibleItemRect);
        notify(ImageDocumentChange::ViewportFrame);
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

bool ImageSpreadPresentationController::updatePresentationPageSlot(DisplayedPageRole role)
{
    switch (role) {
    case DisplayedPageRole::Primary:
        return m_presentationRuntime.updatePrimaryPageSlot(m_primaryPageSurface.snapshot());
    case DisplayedPageRole::Secondary:
        if (m_secondaryPageController == nullptr) {
            return false;
        }
        return m_presentationRuntime.updateSecondaryPageSlot(
            m_secondaryPageController->pageSlotSnapshot());
    }

    return false;
}

void ImageSpreadPresentationController::updateDisplayProjections()
{
    m_primaryPageSurface.updateDisplayProjection(
        m_presentationRuntime.renderProjection(DisplayedPageRole::Primary));
    m_secondaryPageController->pageSurfaceController().updateDisplayProjection(
        m_presentationRuntime.renderProjection(DisplayedPageRole::Secondary));
}

const ImageViewportFrame &ImageSpreadPresentationController::viewportFrame() const
{
    return m_presentationRuntime.viewportFrame();
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
