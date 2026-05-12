// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadpresentationcontroller.h"

#include "imagecallback.h"
#include "imagepresentationcontroller.h"
#include "imagesecondarypagecontroller.h"
#include "imagespreadgeometry.h"
#include "imagespreadmodecontroller.h"
#include "imagespreadzoomcontroller.h"

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
        m_zoomController->setZoomPercent(zoomPercent, rightToLeftReadingActive());
        notifySpreadZoomChanged();
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

int ImageSpreadPresentationController::currentLastPageNumber() const
{
    return imageSpreadCurrentLastPageNumber(currentPageNumber(), secondaryPageVisible());
}

ImageSpreadPageNavigationTarget ImageSpreadPresentationController::imageNavigationTarget(
    NavigationDirection direction) const
{
    if (!twoPageModeActive() || currentPageNumber() <= 0) {
        return {};
    }

    if (direction == NavigationDirection::Next) {
        return ImageSpreadPageNavigationTarget {
            true,
            imageSpreadNextPageTarget(currentLastPageNumber(), imageCount()),
        };
    }

    bool previousPageIsWide = false;
    if (secondaryPageVisible() && currentPageNumber() > 2) {
        const std::optional<QUrl> previousUrl = urlAtPage(currentPageNumber() - 1);
        if (previousUrl.has_value()) {
            previousPageIsWide = cachedPageIsWide(*previousUrl).value_or(false);
        }
    }

    return ImageSpreadPageNavigationTarget {
        true,
        imageSpreadPreviousPageTarget(
            currentPageNumber(), secondaryPageVisible(), previousPageIsWide),
    };
}

int ImageSpreadPresentationController::relativePageNavigationTarget(int offset) const
{
    return imageSpreadRelativePageTarget(currentPageNumber(), imageCount(), offset);
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

std::shared_ptr<DisplayedImageSurface> ImageSpreadPresentationController::imageSurface(
    DisplayedPageRole role) const
{
    if (transitionInProgress()) {
        return nullptr;
    }

    if (role == DisplayedPageRole::Secondary) {
        return secondaryImageSurface();
    }

    return m_primaryPresentation.imageSurface();
}

quint64 ImageSpreadPresentationController::imageRevision(DisplayedPageRole role) const
{
    if (transitionInProgress()) {
        return 0;
    }

    if (role == DisplayedPageRole::Secondary) {
        return secondaryImageRevision();
    }

    return m_primaryPresentation.imageRevision();
}

std::shared_ptr<DisplayedImageSurface>
ImageSpreadPresentationController::secondaryImageSurface() const
{
    return secondaryPageVisible() ? m_secondaryPageController->imageSurface() : nullptr;
}

quint64 ImageSpreadPresentationController::secondaryImageRevision() const
{
    return secondaryPageVisible() ? m_secondaryPageController->imageRevision() : 0;
}

std::optional<bool> ImageSpreadPresentationController::cachedPageIsWide(const QUrl &url) const
{
    return m_secondaryPageController->cachedPageIsWide(url);
}

void ImageSpreadPresentationController::setViewportSize(const QSizeF &viewportSize)
{
    m_primaryPresentation.setViewportSize(viewportSize);
    m_secondaryPageController->setViewportSize(viewportSize);
    if (secondaryPageVisible()) {
        updateZoomState();
    }
}

void ImageSpreadPresentationController::resetZoom()
{
    if (secondaryPageVisible()) {
        m_zoomController->resetZoom(rightToLeftReadingActive());
        notifySpreadZoomChanged();
        return;
    }

    m_primaryPresentation.resetZoom();
}

void ImageSpreadPresentationController::setFitMode(ImageZoomMode zoomMode)
{
    if (secondaryPageVisible()) {
        if (!m_zoomController->setFitMode(zoomMode, rightToLeftReadingActive())) {
            return;
        }
        notifySpreadZoomChanged();
        return;
    }

    m_primaryPresentation.setFitMode(zoomMode);
}

void ImageSpreadPresentationController::updateRenderContext()
{
    m_primaryPresentation.updateRenderContext();
    m_secondaryPageController->updateRenderContext();
    if (secondaryPageVisible()) {
        m_zoomController->updateRenderContext(rightToLeftReadingActive());
        notify(ImageDocumentChange::DisplaySize);
        notify(ImageDocumentChange::MaximumManualZoomPercent);
        notify(ImageDocumentChange::Repaint);
        notify(ImageDocumentChange::TwoPageMode);
    }
}

void ImageSpreadPresentationController::refreshSecondaryPage()
{
    m_secondaryPageController->cachePageSize(
        m_state.displayedUrl(), m_primaryPresentation.imageSize());

    auto finishWithPrimaryPage = [this]() {
        const bool wasVisible = secondaryPageVisible();
        m_zoomController->applyStoredZoomToPrimaryPage();
        clearSecondaryPage();
        if (wasVisible) {
            notifyTwoPageModeChanged();
        }
        finishTransition();
    };

    const int nextPageNumber = currentPageNumber() + 1;
    const std::optional<QUrl> nextUrl = urlAtPage(nextPageNumber);
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

bool ImageSpreadPresentationController::shouldBeginTransition(int targetPageNumber) const
{
    return imageSpreadShouldBeginTransition(
        twoPageModeActive(), currentPageNumber(), targetPageNumber, imageCount());
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
        m_secondaryPageController->cachePageSize(location.imageUrl(), imageSize);
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
}

void ImageSpreadPresentationController::finishSecondaryPageVisible()
{
    updateZoomState();
    finishTransition();
    notifyTwoPageModeChanged();
}

void ImageSpreadPresentationController::notifyTransitionChanged()
{
    notifyChanges(imageDocumentSpreadTransitionNotifications());
}

void ImageSpreadPresentationController::updateZoomState()
{
    if (!secondaryPageVisible()) {
        return;
    }

    m_zoomController->updateFromPrimaryPresentation(rightToLeftReadingActive());
}

QSize ImageSpreadPresentationController::spreadImageSize() const
{
    return m_zoomController->spreadImageSize();
}

bool ImageSpreadPresentationController::primaryPageIsWide() const
{
    return imageSpreadPageIsWide(m_primaryPresentation.imageSize());
}

int ImageSpreadPresentationController::currentPageNumber() const
{
    return m_callbacks.currentPageNumber ? m_callbacks.currentPageNumber() : 0;
}

int ImageSpreadPresentationController::imageCount() const
{
    return m_callbacks.imageCount ? m_callbacks.imageCount() : 0;
}

std::optional<QUrl> ImageSpreadPresentationController::urlAtPage(int pageNumber) const
{
    if (!m_callbacks.urlAtPage) {
        return std::nullopt;
    }

    return m_callbacks.urlAtPage(pageNumber);
}

void ImageSpreadPresentationController::notifyTwoPageModeChanged()
{
    notifyChanges(imageDocumentTwoPageModeNotifications());
}

void ImageSpreadPresentationController::notifySpreadZoomChanged()
{
    notifyChanges(imageDocumentSpreadZoomNotifications());
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
