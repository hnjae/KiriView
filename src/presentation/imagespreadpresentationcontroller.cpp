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
#include "presentation/imagespreadzoomcontroller.h"

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
    ImageNavigationCandidateProvider candidateProvider, ImageDecodeDependencies decodeDependencies)
    : m_state(state)
    , m_primaryPresentation(primaryPresentation)
    , m_callbacks(std::move(callbacks))
{
    RenderContextProvider secondaryRenderContextProvider = renderContextProvider;
    RenderContextProvider spreadRenderContextProvider = std::move(renderContextProvider);
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
        std::move(candidateProvider), std::move(decodeDependencies));
    m_modeController = std::make_unique<ImageSpreadModeController>();
    m_zoomController
        = std::make_unique<ImageSpreadZoomController>(std::move(spreadRenderContextProvider),
            m_primaryPresentation, m_secondaryPageController->presentationController());
}

ImageSpreadPresentationController::~ImageSpreadPresentationController() { shutdown(); }

bool ImageSpreadPresentationController::transitionInProgress() const
{
    return m_modeController->spreadTransitionInProgress();
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
    return secondaryPageVisible() ? spreadImageSize() : m_primaryPresentation.imageSize();
}

QSize ImageSpreadPresentationController::secondaryImageSize() const
{
    return secondaryPageVisible() ? m_secondaryPageController->imageSize() : QSize();
}

QSizeF ImageSpreadPresentationController::displaySize() const
{
    if (secondaryPageVisible()) {
        return m_zoomController->displaySize();
    }

    return m_primaryPresentation.displaySize();
}

QSizeF ImageSpreadPresentationController::primaryDisplaySize() const
{
    if (!secondaryPageVisible()) {
        return m_primaryPresentation.displaySize();
    }

    return m_zoomController->primaryDisplaySize();
}

QSizeF ImageSpreadPresentationController::secondaryDisplaySize() const
{
    if (!secondaryPageVisible()) {
        return {};
    }

    return m_zoomController->secondaryDisplaySize();
}

QPointF ImageSpreadPresentationController::viewportContentPosition() const
{
    return m_viewportFrame.contentPosition;
}

void ImageSpreadPresentationController::setViewportContentPosition(
    const QPointF &viewportContentPosition)
{
    refreshViewportFrameForContentPosition(viewportContentPosition);
}

QSizeF ImageSpreadPresentationController::viewportContentSize() const
{
    return m_viewportFrame.contentSize;
}

QRectF ImageSpreadPresentationController::viewportImageRect() const
{
    return m_viewportFrame.imageRect;
}

bool ImageSpreadPresentationController::viewportHorizontallyPannable() const
{
    return m_viewportFrame.horizontalPannable;
}

bool ImageSpreadPresentationController::viewportVerticallyPannable() const
{
    return m_viewportFrame.verticalPannable;
}

bool ImageSpreadPresentationController::viewportPannable() const
{
    return m_viewportFrame.pannable;
}

QRectF ImageSpreadPresentationController::visibleItemRect() const
{
    return m_viewportFrame.visibleItemRect;
}

void ImageSpreadPresentationController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    refreshViewportFrameForContentPosition(
        QPointF(visibleItemRect.x() + m_viewportFrame.imageRect.x(),
            visibleItemRect.y() + m_viewportFrame.imageRect.y()),
        true);
}

qreal ImageSpreadPresentationController::zoomPercent() const
{
    if (secondaryPageVisible()) {
        return m_zoomController->zoomPercent();
    }

    return m_primaryPresentation.zoomPercent();
}

void ImageSpreadPresentationController::setZoomPercent(qreal zoomPercent)
{
    if (secondaryPageVisible()) {
        applySpreadZoomChanges(m_zoomController->setZoomPercent(zoomPercent));
        return;
    }

    m_primaryPresentation.setZoomPercent(zoomPercent);
}

ImageZoomMode ImageSpreadPresentationController::zoomMode() const
{
    if (secondaryPageVisible()) {
        return m_zoomController->zoomMode();
    }

    return m_primaryPresentation.zoomMode();
}

qreal ImageSpreadPresentationController::maximumManualZoomPercent() const
{
    if (secondaryPageVisible()) {
        return m_zoomController->maximumManualZoomPercent();
    }

    return m_primaryPresentation.maximumManualZoomPercent();
}

qreal ImageSpreadPresentationController::clampedManualZoomPercent(qreal zoomPercent) const
{
    if (secondaryPageVisible()) {
        return m_zoomController->clampedManualZoomPercent(zoomPercent);
    }

    return m_primaryPresentation.clampedManualZoomPercent(zoomPercent);
}

qreal ImageSpreadPresentationController::steppedManualZoomPercent(qreal stepCount) const
{
    if (secondaryPageVisible()) {
        return m_zoomController->steppedManualZoomPercent(stepCount);
    }

    return m_primaryPresentation.steppedManualZoomPercent(stepCount);
}

int ImageSpreadPresentationController::rotationDegrees() const
{
    return secondaryPageVisible() ? 0 : m_primaryPresentation.rotationDegrees();
}

int ImageSpreadPresentationController::currentLastPageNumber() const
{
    return m_secondaryPageRefresh.currentLastPageNumber(pageNavigationContext());
}

ImageSpreadPageNavigationTarget ImageSpreadPresentationController::imageNavigationTarget(
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

    ImageZoomMode previousZoomMode = ImageZoomMode::Fit;
    qreal previousZoomPercent = 100.0;
    if (change.restorePrimaryZoom) {
        previousZoomMode = zoomMode();
        previousZoomPercent = zoomPercent();
    }

    if (change.resetSpreadZoom) {
        m_zoomController->clearZoomState();
    }
    if (enabled) {
        m_primaryPresentation.resetRotation();
    }
    if (change.finishTransition) {
        finishTransition();
    }
    if (change.clearSecondaryPage) {
        clearSecondaryPage();
    }
    if (change.restorePrimaryZoom) {
        m_zoomController->applyZoomToPrimaryPage(previousZoomMode, previousZoomPercent);
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
    };
}

DisplayedImageRenderSnapshot ImageSpreadPresentationController::renderSnapshot(
    DisplayedPageRole role) const
{
    if (transitionInProgress()) {
        return {};
    }

    if (role == DisplayedPageRole::Secondary) {
        DisplayedImageRenderSnapshot snapshot = m_secondaryPageController->renderSnapshot();
        snapshot.rotationDegrees = 0;
        snapshot.pageRole = DisplayedPageRole::Secondary;
        return snapshot;
    }

    DisplayedImageRenderSnapshot snapshot = m_primaryPresentation.renderSnapshot();
    snapshot.pageRole = DisplayedPageRole::Primary;
    return snapshot;
}

void ImageSpreadPresentationController::setViewportSize(const QSizeF &viewportSize)
{
    m_primaryPresentation.setViewportSize(viewportSize);
    m_secondaryPageController->setViewportSize(viewportSize);
    if (secondaryPageVisible()) {
        applySpreadZoomChanges(updateZoomState());
    }
}

void ImageSpreadPresentationController::resetZoom()
{
    if (secondaryPageVisible()) {
        applySpreadZoomChanges(m_zoomController->resetZoom());
        return;
    }

    m_primaryPresentation.resetZoom();
}

void ImageSpreadPresentationController::setFitMode(ImageZoomMode zoomMode)
{
    if (secondaryPageVisible()) {
        applySpreadZoomChanges(m_zoomController->setFitMode(zoomMode));
        return;
    }

    m_primaryPresentation.setFitMode(zoomMode);
}

void ImageSpreadPresentationController::rotateClockwise()
{
    if (twoPageModeActive()) {
        return;
    }

    m_primaryPresentation.rotateClockwise();
}

void ImageSpreadPresentationController::rotateCounterclockwise()
{
    if (twoPageModeActive()) {
        return;
    }

    m_primaryPresentation.rotateCounterclockwise();
}

void ImageSpreadPresentationController::updateRenderContext()
{
    m_primaryPresentation.updateRenderContext();
    m_secondaryPageController->updateRenderContext();
    if (secondaryPageVisible()) {
        applySpreadZoomChanges(m_zoomController->updateRenderContext());
    }
}

void ImageSpreadPresentationController::refreshSecondaryPage()
{
    m_secondaryPageRefresh.cachePageSize(m_state.displayedUrl(), m_primaryPresentation.imageSize());

    auto finishWithPrimaryPage = [this]() {
        const bool wasVisible = secondaryPageVisible();
        m_zoomController->applyStoredZoomToPrimaryPage();
        clearSecondaryPage();
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
    if (!m_modeController->beginSpreadTransition()) {
        notifyTransitionChanged();
        return;
    }

    notifyTransitionChanged();
}

void ImageSpreadPresentationController::finishTransition()
{
    if (!m_modeController->finishSpreadTransition()) {
        return;
    }

    notifyTransitionChanged();
}

void ImageSpreadPresentationController::clearSecondaryPage() { m_secondaryPageController->clear(); }

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
    m_secondaryPageController->startLoad(
        url, m_state.displayedArchiveDocument(), m_primaryPresentation.firstDisplayDecodeContext());
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
    clearSecondaryPage();
    m_zoomController->applyStoredZoomToPrimaryPage();
    finishTransition();
    notifyTwoPageModeChanged();
    scheduleAdjacentPredecode();
}

void ImageSpreadPresentationController::finishSecondaryPageVisible()
{
    applySpreadZoomChanges(updateZoomState(), false);
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
    if (!secondaryPageVisible()) {
        return {};
    }

    return m_zoomController->updateFromPrimaryPresentation();
}

QSize ImageSpreadPresentationController::spreadImageSize() const
{
    return m_zoomController->spreadImageSize();
}

bool ImageSpreadPresentationController::primaryPageIsWide() const
{
    return imageSpreadPageIsWide(m_primaryPresentation.imageSize());
}

ImageSpreadReadingAvailability ImageSpreadPresentationController::readingAvailability() const
{
    const DisplayedImageLocation &location = m_state.displayedImageLocation();
    return ImageSpreadReadingAvailability { m_primaryPresentation.hasImage(), !location.isEmpty(),
        location.archiveDocument().isComicBook() };
}

void ImageSpreadPresentationController::scheduleAdjacentPredecode()
{
    invokeIfSet(m_callbacks.scheduleAdjacentPredecode);
}

ImageSpreadPageNavigationContext ImageSpreadPresentationController::pageNavigationContext() const
{
    return ImageSpreadPageNavigationContext { twoPageModeActive(), secondaryPageVisible(),
        pageNavigationSnapshot() };
}

ImagePageNavigationSnapshot ImageSpreadPresentationController::pageNavigationSnapshot() const
{
    return m_callbacks.pageNavigationSnapshot ? m_callbacks.pageNavigationSnapshot()
                                              : ImagePageNavigationSnapshot {};
}

void ImageSpreadPresentationController::notifyTwoPageModeChanged()
{
    notifyChanges(imageDocumentTwoPageModeNotifications());
}

void ImageSpreadPresentationController::applySpreadZoomChanges(
    const ImageZoomChangeSet &changes, bool notifyPublicChanges)
{
    if (hasSpreadZoomStateChange(changes)) {
        if (spreadZoomChangesAffectViewportFrame(changes)) {
            refreshViewportFrame();
        }
        m_zoomController->applyZoomStateToPages(rightToLeftReadingActive());
    }

    if (notifyPublicChanges) {
        notifySpreadZoomChanged(changes);
    }
}

void ImageSpreadPresentationController::notifySpreadZoomChanged(const ImageZoomChangeSet &changes)
{
    notifyChanges(imageDocumentSpreadZoomNotifications(changes));
}

bool ImageSpreadPresentationController::refreshViewportFrame(bool forceApplyVisibleItemRect)
{
    return refreshViewportFrameForContentPosition(
        m_viewportFrame.contentPosition, forceApplyVisibleItemRect);
}

bool ImageSpreadPresentationController::refreshViewportFrameForContentPosition(
    const QPointF &contentPosition, bool forceApplyVisibleItemRect)
{
    const QRectF previousVisibleItemRect = m_viewportFrame.visibleItemRect;
    const ImageViewportFrame nextFrame = projectImageViewportFrame(
        m_primaryPresentation.viewportSize(), displaySize(), contentPosition);
    if (nextFrame == m_viewportFrame && !forceApplyVisibleItemRect) {
        return false;
    }

    const bool frameChanged = nextFrame != m_viewportFrame;
    m_viewportFrame = nextFrame;
    applyViewportFrameVisibleItemRect(
        forceApplyVisibleItemRect || previousVisibleItemRect != m_viewportFrame.visibleItemRect);
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

    if (secondaryPageVisible()) {
        m_zoomController->setVisibleItemRect(m_viewportFrame.visibleItemRect);
        m_zoomController->applyVisibleItemRects(rightToLeftReadingActive());
        notify(ImageDocumentChange::VisibleItemRect);
        return;
    }

    m_primaryPresentation.setVisibleItemRect(m_viewportFrame.visibleItemRect);
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
