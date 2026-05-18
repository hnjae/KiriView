// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadpresentationcontroller.h"

#include "imagecallback.h"
#include "imagedocumentnotifications.h"
#include "imagepresentationcontroller.h"
#include "imagesecondarypagecontroller.h"
#include "imagespreadgeometry.h"
#include "imagespreadmodecontroller.h"
#include "imagespreadnavigation.h"
#include "imagespreadzoomcontroller.h"
#include "imageurl.h"

#include <limits>
#include <utility>

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
                if (!m_callbacks.takePredecodedImage) {
                    return std::optional<PredecodedImage>();
                }

                return m_callbacks.takePredecodedImage(url);
            },
        },
        std::move(candidateProvider), std::move(decodeDependencies));
    m_modeController = std::make_unique<ImageSpreadModeController>([this]() {
        return ImageSpreadModeAvailability { m_primaryPresentation.hasImage(),
            m_state.displayedImageLocation() };
    });
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

QRectF ImageSpreadPresentationController::visibleItemRect() const
{
    return secondaryPageVisible() ? m_zoomController->visibleItemRect()
                                  : m_primaryPresentation.visibleItemRect();
}

void ImageSpreadPresentationController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    m_zoomController->setVisibleItemRect(visibleItemRect);
    if (secondaryPageVisible()) {
        m_zoomController->applyVisibleItemRects(rightToLeftReadingActive());
        notify(ImageDocumentChange::VisibleItemRect);
        return;
    }

    m_primaryPresentation.setVisibleItemRect(visibleItemRect);
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
        notifySpreadZoomChanged(
            m_zoomController->setZoomPercent(zoomPercent, rightToLeftReadingActive()));
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
    return imageSpreadNavigationCurrentLastPageNumber(navigationState());
}

ImageSpreadPageNavigationTarget ImageSpreadPresentationController::imageNavigationTarget(
    NavigationDirection direction) const
{
    ImageSpreadNavigationState state = navigationState();
    if (direction == NavigationDirection::Previous) {
        state.previousPageIsWide = previousPageIsWideForNavigation();
    }

    return imageSpreadPageNavigationTarget(direction, state);
}

int ImageSpreadPresentationController::relativePageNavigationTarget(int offset) const
{
    return imageSpreadRelativePageNavigationTarget(navigationState(), offset);
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
    return m_modeController->twoPageModeAvailable();
}

bool ImageSpreadPresentationController::twoPageModeActive() const
{
    return m_modeController->twoPageModeActive();
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
        m_zoomController->applyVisibleItemRects(rightToLeftReadingActive());
    }
    notifyRightToLeftReadingChanged();
}

bool ImageSpreadPresentationController::rightToLeftReadingAvailable() const
{
    return m_modeController->rightToLeftReadingAvailable();
}

bool ImageSpreadPresentationController::rightToLeftReadingActive() const
{
    return m_modeController->rightToLeftReadingActive();
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
        return snapshot;
    }

    return m_primaryPresentation.renderSnapshot();
}

std::optional<bool> ImageSpreadPresentationController::cachedPageIsWide(const QUrl &url) const
{
    return m_pageCache.cachedPageIsWide(url);
}

void ImageSpreadPresentationController::setViewportSize(const QSizeF &viewportSize)
{
    m_primaryPresentation.setViewportSize(viewportSize);
    m_secondaryPageController->setViewportSize(viewportSize);
    if (secondaryPageVisible()) {
        notifySpreadZoomChanged(updateZoomState());
    }
}

void ImageSpreadPresentationController::resetZoom()
{
    if (secondaryPageVisible()) {
        notifySpreadZoomChanged(m_zoomController->resetZoom(rightToLeftReadingActive()));
        return;
    }

    m_primaryPresentation.resetZoom();
}

void ImageSpreadPresentationController::setFitMode(ImageZoomMode zoomMode)
{
    if (secondaryPageVisible()) {
        notifySpreadZoomChanged(m_zoomController->setFitMode(zoomMode, rightToLeftReadingActive()));
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
        notifySpreadZoomChanged(m_zoomController->updateRenderContext(rightToLeftReadingActive()));
    }
}

void ImageSpreadPresentationController::refreshSecondaryPage()
{
    m_pageCache.cachePageSize(m_state.displayedUrl(), m_primaryPresentation.imageSize());

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

    const ImagePageNavigationSnapshot navigation = pageNavigationSnapshot();
    const int currentPage = navigation.currentPageNumber();
    const int nextPageNumber = currentPage == std::numeric_limits<int>::max() ? 0 : currentPage + 1;
    const std::optional<QUrl> nextUrl = navigation.urlAtPage(nextPageNumber);
    const bool nextPageIsWide = nextUrl.has_value() && cachedPageIsWide(*nextUrl).value_or(false);
    const bool currentSecondaryMatchesNext = nextUrl.has_value() && secondaryPageVisible()
        && m_secondaryPageController->displayedImageLocation().imageUrl() == *nextUrl;
    const ImageSpreadSecondaryPageRefreshPlan plan
        = imageSpreadSecondaryPageRefreshPlan(ImageSpreadSecondaryPageRefreshState {
            twoPageModeActive(),
            currentPage,
            navigation.imageCount(),
            primaryPageIsWide(),
            nextUrl.has_value(),
            nextPageIsWide,
            currentSecondaryMatchesNext,
        });
    if (plan.decision == ImageSpreadSecondaryPageDecision::PrimaryOnly) {
        finishWithPrimaryPage();
        return;
    }
    if (plan.decision == ImageSpreadSecondaryPageDecision::KeepCurrentSecondary) {
        return;
    }

    const std::optional<QUrl> targetUrl = plan.targetPageNumber == nextPageNumber
        ? nextUrl
        : navigation.urlAtPage(plan.targetPageNumber);
    if (!targetUrl.has_value()) {
        finishWithPrimaryPage();
        return;
    }

    startSecondaryPageLoad(*targetUrl);
}

void ImageSpreadPresentationController::handleDocumentChange(ImageDocumentChange change)
{
    if (change == ImageDocumentChange::ErrorString && !m_state.errorString().isEmpty()) {
        finishTransition();
    }

    if (change == ImageDocumentChange::PageNavigation) {
        if (primarySelectionMatchesDisplayed()) {
            refreshSecondaryPage();
        }
        notifyRightToLeftReadingChanged();
    }
}

bool ImageSpreadPresentationController::shouldBeginTransition(int targetPageNumber) const
{
    return imageSpreadShouldBeginNavigationTransition(navigationState(), targetPageNumber);
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

bool ImageSpreadPresentationController::shouldResetRightToLeftReadingForLoad(
    const ArchiveDocumentLocation &displayedArchiveDocument, const QUrl &sourceUrl,
    const QUrl &containerNavigationUrl) const
{
    return m_modeController->shouldResetRightToLeftReadingForLoad(
        displayedArchiveDocument, sourceUrl, containerNavigationUrl);
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
        m_pageCache.cachePageSize(location.imageUrl(), imageSize);
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
    updateZoomState();
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

    return m_zoomController->updateFromPrimaryPresentation(rightToLeftReadingActive());
}

QSize ImageSpreadPresentationController::spreadImageSize() const
{
    return m_zoomController->spreadImageSize();
}

bool ImageSpreadPresentationController::primaryPageIsWide() const
{
    return imageSpreadPageIsWide(m_primaryPresentation.imageSize());
}

bool ImageSpreadPresentationController::previousPageIsWideForNavigation() const
{
    const ImagePageNavigationSnapshot navigation = pageNavigationSnapshot();
    const int pageNumber = navigation.currentPageNumber();
    if (!secondaryPageVisible() || pageNumber <= 2) {
        return false;
    }

    const std::optional<QUrl> previousUrl = navigation.urlAtPage(pageNumber - 1);
    return previousUrl.has_value() ? cachedPageIsWide(*previousUrl).value_or(false) : false;
}

bool ImageSpreadPresentationController::primarySelectionMatchesDisplayed() const
{
    const ImagePageNavigationSnapshot navigation = pageNavigationSnapshot();
    const int pageNumber = navigation.currentPageNumber();
    if (pageNumber <= 0) {
        return true;
    }

    const std::optional<QUrl> pageUrl = navigation.urlAtPage(pageNumber);
    if (!pageUrl.has_value()) {
        return true;
    }

    return sameNormalizedUrl(*pageUrl, m_state.displayedUrl());
}

void ImageSpreadPresentationController::scheduleAdjacentPredecode()
{
    invokeIfSet(m_callbacks.scheduleAdjacentPredecode);
}

ImageSpreadNavigationState ImageSpreadPresentationController::navigationState(
    bool previousPageIsWide) const
{
    const ImagePageNavigationSnapshot navigation = pageNavigationSnapshot();
    return ImageSpreadNavigationState { twoPageModeActive(), navigation.currentPageNumber(),
        navigation.imageCount(), secondaryPageVisible(), previousPageIsWide };
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

void ImageSpreadPresentationController::notifySpreadZoomChanged(const ImageZoomChangeSet &changes)
{
    notifyChanges(imageDocumentSpreadZoomNotifications(changes));
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
