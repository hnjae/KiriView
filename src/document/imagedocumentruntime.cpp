// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntime.h"

#include "async/imagecallback.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentruntimecontrollers.h"
#include "imagedocumentsourceloadrequest.h"
#include "imageopenworkflow.h"
#include "presentation/imagepagesurfacecontroller.h"
#include "presentation/imagepresentationruntime.h"
#include "presentation/imagespreadpresentationcontroller.h"

#include <QObject>
#include <QUrl>
#include <cmath>
#include <utility>

namespace kiriview {
namespace {
    bool pointIsFinite(QPointF point)
    {
        return std::isfinite(point.x()) && std::isfinite(point.y());
    }

    ImageDocumentSourceLoadSnapshot sourceLoadSnapshot(
        const ImageDocumentState& state, const ImageSpreadPresentationController& spreadController)
    {
        return ImageDocumentSourceLoadSnapshot {
            state.sourceUrl(),
            state.displayedOpenedCollectionScope(),
            spreadController.rightToLeftReadingEnabled(),
        };
    }
}

ImageDocumentRuntime::ImageDocumentRuntime(QObject* documentObject,
    RenderContextProvider renderContextProvider, ChangeCallback changeCallback,
    ImageDocumentRuntimeDependencyOverrides dependencies,
    FileDeletionFailedCallback fileDeletionFailedCallback,
    UnsupportedOpenedCollectionVideoEnteredCallback unsupportedOpenedCollectionVideoEnteredCallback,
    ContainerNavigationBoundaryReachedCallback containerNavigationBoundaryReachedCallback)
    : changeBatcher(ImageDocumentChangeBatcher::ChangeBatchCallback(
          [this](const std::vector<ImageDocumentChange>& changes) { publishChanges(changes); }))
    , state(changeBatcher)
    , changeCallback(std::move(changeCallback))
    , renderContextProvider(std::move(renderContextProvider))
{
    controllers = std::make_unique<ImageDocumentRuntimeControllers>(documentObject, state,
        std::move(dependencies),
        ImageDocumentRuntimeControllerCallbacks {
            [this]() { return renderContext(); },
            [this](ImageDocumentChange change) { notify(change); },
            [this](const ImageDocumentSourceLoadRequest& request) { loadSource(request); },
            std::move(fileDeletionFailedCallback),
            std::move(unsupportedOpenedCollectionVideoEnteredCallback),
            std::move(containerNavigationBoundaryReachedCallback),
        });
}

ImageDocumentRuntime::~ImageDocumentRuntime() { shutdown(); }

QUrl ImageDocumentRuntime::sourceUrl() const { return state.sourceUrl(); }

void ImageDocumentRuntime::setSourceUrl(const QUrl& sourceUrl)
{
    loadSource(ImageDocumentSourceLoadRequest::fromUrl(sourceUrl));
}

ImageDocumentStatus ImageDocumentRuntime::status() const
{
    return controllers->spreadController().status(state.status());
}

bool ImageDocumentRuntime::loading() const
{
    return controllers->spreadController().loading(state.loading());
}

QString ImageDocumentRuntime::errorString() const { return state.errorString(); }

const std::optional<ImageLoadFailure>& ImageDocumentRuntime::loadFailure() const
{
    return state.loadFailure();
}

QString ImageDocumentRuntime::windowTitleFileName() const { return state.windowTitleFileName(); }

QUrl ImageDocumentRuntime::displayedUrl() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return {};
    }

    return state.displayedUrl();
}

OpenedCollectionScopeLocation ImageDocumentRuntime::displayedOpenedCollectionScope() const
{
    return state.displayedOpenedCollectionScope();
}

QSize ImageDocumentRuntime::imageSize() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return {};
    }

    return controllers->spreadController().imageSize();
}

QSize ImageDocumentRuntime::primaryImageSize() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return {};
    }

    return controllers->spreadController().primaryImageSize();
}

QSize ImageDocumentRuntime::secondaryImageSize() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return {};
    }

    return controllers->spreadController().secondaryImageSize();
}

QSizeF ImageDocumentRuntime::viewportSize() const
{
    return controllers->presentationRuntime().viewportSize();
}

void ImageDocumentRuntime::setViewportSize(QSizeF viewportSize)
{
    controllers->spreadController().setViewportSize(viewportSize);
}

QPointF ImageDocumentRuntime::viewportContentPosition() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return {};
    }

    return controllers->spreadController().viewportContentPosition();
}

quint64 ImageDocumentRuntime::requestViewportContentPosition(QPointF viewportContentPosition)
{
    if (!pointIsFinite(viewportContentPosition)) {
        return 0;
    }

    return controllers->spreadController()
        .requestViewportContentPosition(viewportContentPosition)
        .revision;
}

quint64 ImageDocumentRuntime::requestViewportPanBy(QPointF delta)
{
    if (!viewportPannable() || !pointIsFinite(delta)) {
        return 0;
    }

    return requestViewportInteractionContentPosition(viewportInteraction.panContentPosition(
        viewportInteractionSnapshot(), viewportContentPosition(), delta));
}

quint64 ImageDocumentRuntime::requestViewportPanToInitialScanPosition()
{
    if (!viewportPannable()) {
        return 0;
    }

    return requestViewportInteractionContentPosition(
        viewportInteraction.initialScanContentPosition(viewportInteractionSnapshot()));
}

quint64 ImageDocumentRuntime::requestViewportPanToFinalScanPosition()
{
    if (!viewportPannable()) {
        return 0;
    }

    return requestViewportInteractionContentPosition(
        viewportInteraction.finalScanContentPosition(viewportInteractionSnapshot()));
}

quint64 ImageDocumentRuntime::requestViewportScanForward()
{
    if (!viewportPannable()) {
        return 0;
    }

    const QPointF nextContentPosition = viewportInteraction.nextScanContentPosition(
        viewportInteractionSnapshot(), viewportContentPosition());
    if (nextContentPosition == viewportContentPosition()) {
        return 0;
    }

    return requestViewportInteractionContentPosition(nextContentPosition);
}

quint64 ImageDocumentRuntime::requestViewportScanBackward()
{
    if (!viewportPannable()) {
        return 0;
    }

    const QPointF previousContentPosition = viewportInteraction.previousScanContentPosition(
        viewportInteractionSnapshot(), viewportContentPosition());
    if (previousContentPosition == viewportContentPosition()) {
        return 0;
    }

    return requestViewportInteractionContentPosition(previousContentPosition);
}

void ImageDocumentRuntime::requestNextDisplayedImageStartToFinalScanPosition()
{
    viewportInteraction.requestNextDisplayedImageFinalScanStart();
}

quint64 ImageDocumentRuntime::requestDisplayedImageInitialContentPosition()
{
    return requestViewportContentPosition(
        viewportInteraction.displayedImageInitialContentPosition(viewportInteractionSnapshot()));
}

bool ImageDocumentRuntime::beginViewportCommandApplication(quint64 commandRevision)
{
    return controllers->spreadController().beginViewportCommandApplication(commandRevision);
}

bool ImageDocumentRuntime::completeViewportCommandApplication(
    quint64 commandRevision, QPointF actualContentPosition)
{
    return controllers->spreadController().completeViewportCommandApplication(
        commandRevision, actualContentPosition);
}

bool ImageDocumentRuntime::acknowledgeViewportCommand(
    quint64 commandRevision, QPointF actualContentPosition)
{
    return controllers->spreadController().acknowledgeViewportCommand(
        commandRevision, actualContentPosition);
}

bool ImageDocumentRuntime::observeViewportContentPosition(
    QPointF contentPosition, ImageViewportObservationOrigin origin)
{
    return controllers->spreadController().observeViewportContentPosition(contentPosition, origin);
}

quint64 ImageDocumentRuntime::viewportCommandRevision() const
{
    return controllers->spreadController().viewportCommandRevision();
}

quint64 ImageDocumentRuntime::viewportAppliedCommandRevision() const
{
    return controllers->spreadController().viewportAppliedCommandRevision();
}

quint64 ImageDocumentRuntime::viewportObservationRevision() const
{
    return controllers->spreadController().viewportObservationRevision();
}

ImageViewportCommandStatus ImageDocumentRuntime::viewportCommandStatus() const
{
    return controllers->spreadController().viewportCommandStatus();
}

ImageViewportObservationOrigin ImageDocumentRuntime::viewportObservationOrigin() const
{
    return controllers->spreadController().viewportObservationOrigin();
}

QSizeF ImageDocumentRuntime::viewportContentSize() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return {};
    }

    return controllers->spreadController().viewportContentSize();
}

QRectF ImageDocumentRuntime::viewportImageRect() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return {};
    }

    return controllers->spreadController().viewportImageRect();
}

bool ImageDocumentRuntime::viewportHorizontallyPannable() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return false;
    }

    return controllers->spreadController().viewportHorizontallyPannable();
}

bool ImageDocumentRuntime::viewportVerticallyPannable() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return false;
    }

    return controllers->spreadController().viewportVerticallyPannable();
}

bool ImageDocumentRuntime::viewportPannable() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return false;
    }

    return controllers->spreadController().viewportPannable();
}

QRectF ImageDocumentRuntime::visibleItemRect() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return {};
    }

    return controllers->spreadController().visibleItemRect();
}

QSizeF ImageDocumentRuntime::displaySize() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return {};
    }

    return controllers->spreadController().displaySize();
}

QSizeF ImageDocumentRuntime::primaryDisplaySize() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return {};
    }

    return controllers->spreadController().primaryDisplaySize();
}

QSizeF ImageDocumentRuntime::secondaryDisplaySize() const
{
    if (status() != ImageDocumentStatus::Ready) {
        return {};
    }

    return controllers->spreadController().secondaryDisplaySize();
}

bool ImageDocumentRuntime::zoomPercentKnown() const
{
    return status() == ImageDocumentStatus::Ready && !imageSize().isEmpty()
        && !displaySize().isEmpty();
}

qreal ImageDocumentRuntime::zoomPercent() const
{
    return controllers->spreadController().zoomPercent();
}

void ImageDocumentRuntime::requestManualZoomPercent(qreal zoomPercent)
{
    controllers->spreadController().requestManualZoomPercent(zoomPercent);
}

bool ImageDocumentRuntime::requestManualZoomPercentAtCenter(qreal zoomPercent)
{
    if (status() != ImageDocumentStatus::Ready) {
        return false;
    }

    return requestAnchoredManualZoom(clampedManualZoomPercent(zoomPercent),
        QPointF(viewportSize().width() / 2.0, viewportSize().height() / 2.0));
}

bool ImageDocumentRuntime::requestZoomByStep(qreal stepCount, QPointF viewportAnchorPoint)
{
    if (status() != ImageDocumentStatus::Ready || !pointIsFinite(viewportAnchorPoint)) {
        return false;
    }

    const QPointF anchorPoint = viewportInteraction.nearestImageViewportPoint(
        viewportInteractionSnapshot(), viewportContentPosition(), viewportAnchorPoint);
    if (!pointIsFinite(anchorPoint)) {
        return false;
    }

    return requestAnchoredManualZoom(steppedManualZoomPercent(stepCount), anchorPoint);
}

bool ImageDocumentRuntime::requestZoomByStepAtCenter(qreal stepCount)
{
    return requestZoomByStep(
        stepCount, QPointF(viewportSize().width() / 2.0, viewportSize().height() / 2.0));
}

bool ImageDocumentRuntime::requestActualSizeAtCenter()
{
    if (status() != ImageDocumentStatus::Ready) {
        return false;
    }

    return requestAnchoredManualZoom(clampedManualZoomPercent(100.0),
        QPointF(viewportSize().width() / 2.0, viewportSize().height() / 2.0));
}

bool ImageDocumentRuntime::requestToggleFitOrActualSize(QPointF viewportPoint)
{
    if (status() != ImageDocumentStatus::Ready || !pointIsFinite(viewportPoint)) {
        return false;
    }

    if (zoomMode() != ImageZoomMode::Fit) {
        setFitMode(ImageZoomMode::Fit);
        return true;
    }

    const QPointF anchorPoint = viewportInteraction.nearestImageViewportPoint(
        viewportInteractionSnapshot(), viewportContentPosition(), viewportPoint);
    if (!pointIsFinite(anchorPoint)) {
        return false;
    }

    return requestAnchoredManualZoom(clampedManualZoomPercent(100.0), anchorPoint);
}

ImageZoomMode ImageDocumentRuntime::zoomMode() const
{
    return controllers->spreadController().zoomMode();
}

ImageZoomMode ImageDocumentRuntime::fitModeSelection() const
{
    return controllers->spreadController().fitModeSelection();
}

qreal ImageDocumentRuntime::maximumManualZoomPercent() const
{
    return controllers->spreadController().maximumManualZoomPercent();
}

qreal ImageDocumentRuntime::clampedManualZoomPercent(qreal zoomPercent) const
{
    return controllers->spreadController().clampedManualZoomPercent(zoomPercent);
}

qreal ImageDocumentRuntime::steppedManualZoomPercent(qreal stepCount) const
{
    return controllers->spreadController().steppedManualZoomPercent(stepCount);
}

int ImageDocumentRuntime::rotationDegrees() const
{
    return controllers->spreadController().rotationDegrees();
}

int ImageDocumentRuntime::currentPageNumber() const
{
    return controllers->navigationController().currentPageNumber();
}

int ImageDocumentRuntime::currentLastPageNumber() const
{
    return controllers->spreadController().currentLastPageNumber();
}

int ImageDocumentRuntime::pageCount() const
{
    return controllers->navigationController().pageCount();
}

ImageDocumentPageNavigationSnapshot ImageDocumentRuntime::pageNavigationSnapshot() const
{
    return controllers->navigationController().pageNavigationSnapshot();
}

ImageDocumentPageActiveNavigationSnapshot ImageDocumentRuntime::activeNavigationSnapshot() const
{
    return controllers->spreadController().activeNavigationSnapshot();
}

bool ImageDocumentRuntime::containerNavigationAvailable() const
{
    return state.containerNavigationAvailable();
}

bool ImageDocumentRuntime::ordinaryDirectMediaScopeActive() const
{
    return !state.displayedUrl().isEmpty() && state.displayedOpenedCollectionScope().isEmpty();
}

bool ImageDocumentRuntime::openedCollectionScopeActive() const
{
    return !state.displayedOpenedCollectionScope().isEmpty();
}

bool ImageDocumentRuntime::fileDeletionInProgress() const
{
    return controllers->deletionController().inProgress();
}

bool ImageDocumentRuntime::twoPageModeEnabled() const
{
    return controllers->spreadController().twoPageModeEnabled();
}

void ImageDocumentRuntime::setTwoPageModeEnabled(bool enabled)
{
    controllers->spreadController().setTwoPageModeEnabled(enabled);
}

bool ImageDocumentRuntime::twoPageModeAvailable() const
{
    return controllers->spreadController().twoPageModeAvailable();
}

bool ImageDocumentRuntime::rightToLeftReadingEnabled() const
{
    return controllers->spreadController().rightToLeftReadingEnabled();
}

void ImageDocumentRuntime::setRightToLeftReadingEnabled(bool enabled)
{
    controllers->spreadController().setRightToLeftReadingEnabled(enabled);
}

bool ImageDocumentRuntime::rightToLeftReadingAvailable() const
{
    return controllers->spreadController().rightToLeftReadingAvailable();
}

bool ImageDocumentRuntime::secondaryPageVisible() const
{
    return controllers->spreadController().secondaryPageVisible();
}

ImagePresentationTransitionState ImageDocumentRuntime::presentationTransitionState() const
{
    return controllers->spreadController().presentationTransitionState();
}

bool ImageDocumentRuntime::viewportPointInsideImage(QPointF viewportPoint) const
{
    return viewportInteraction.viewportPointInsideImage(
        viewportInteractionSnapshot(), viewportContentPosition(), viewportPoint);
}

QPointF ImageDocumentRuntime::nearestImageViewportPoint(QPointF viewportPoint) const
{
    return viewportInteraction.nearestImageViewportPoint(
        viewportInteractionSnapshot(), viewportContentPosition(), viewportPoint);
}

bool ImageDocumentRuntime::unsupportedOpenedCollectionVideo() const
{
    return state.unsupportedOpenedCollectionVideo();
}

std::optional<DisplayedPredecodeImage> ImageDocumentRuntime::primaryDisplayedPredecodeImage() const
{
    std::optional<StaticDisplayImagePayload> displayImage
        = controllers->pageSurfaceController().displayImage();
    if (!controllers->pageSurfaceController().hasImage() || state.displayedUrl().isEmpty()
        || !displayImage.has_value()) {
        return std::nullopt;
    }

    return DisplayedPredecodeImage {
        state.displayedImageLocation(),
        controllers->pageSurfaceController().isPredecodeCacheable(),
        std::move(displayImage),
        state.embeddedMetadata(),
    };
}

ImageFirstDisplayDecodeContext ImageDocumentRuntime::firstDisplayDecodeContext() const
{
    return controllers->presentationRuntime().firstDisplayDecodeContext();
}

const EmbeddedMetadata& ImageDocumentRuntime::embeddedMetadata() const
{
    return state.embeddedMetadata();
}

ImageDisplaySourceProjection ImageDocumentRuntime::displaySourceProjection(
    DisplayedPageRole role) const
{
    if (displayedUrl().isEmpty()) {
        ImageDisplaySourceProjection projection;
        projection.pageRole = role;
        projection.revisionToken = imageDisplaySourceRevisionToken(projection.revision);
        return projection;
    }

    return controllers->spreadController().displaySourceProjection(role);
}

void ImageDocumentRuntime::acknowledgeStillImageDisplayLoad(DisplayedPageRole role,
    const QUrl& providerUrl, quint64 revision, const QString& sourceIdentity,
    ImageDisplayLoadOutcome outcome)
{
    controllers->spreadController().acknowledgeStillImageDisplayLoad(
        role, providerUrl, revision, sourceIdentity, outcome);
}

void ImageDocumentRuntime::acknowledgeDisplayImageLoad(DisplayedPageRole role,
    const QUrl& providerUrl, quint64 revision, const QString& sourceIdentity,
    ImageDisplayLoadOutcome outcome)
{
    controllers->spreadController().acknowledgeDisplayImageLoad(
        role, providerUrl, revision, sourceIdentity, outcome);
}

void ImageDocumentRuntime::notify(ImageDocumentChange change) { changeBatcher.notify(change); }

void ImageDocumentRuntime::setRenderContextProvider(RenderContextProvider provider)
{
    renderContextProvider = std::move(provider);
    updateRenderContext();
}

ImageDocumentRenderContext ImageDocumentRuntime::renderContext() const
{
    if (renderContextProvider) {
        return renderContextProvider();
    }

    return ImageDocumentRenderContext {};
}

quint64 ImageDocumentRuntime::requestViewportInteractionContentPosition(QPointF contentPosition)
{
    if (!pointIsFinite(contentPosition)) {
        return 0;
    }

    const QPointF previousContentPosition = viewportContentPosition();
    const quint64 previousCommandRevision = viewportCommandRevision();
    const quint64 commandRevision = requestViewportContentPosition(contentPosition);
    if (previousContentPosition != viewportContentPosition()
        || commandRevision > previousCommandRevision) {
        return commandRevision;
    }

    return 0;
}

bool ImageDocumentRuntime::requestAnchoredManualZoom(qreal zoomPercent, QPointF viewportAnchorPoint)
{
    if (!pointIsFinite(viewportAnchorPoint)) {
        return false;
    }

    const qreal nextZoomPercent = clampedManualZoomPercent(zoomPercent);
    if (std::abs(nextZoomPercent - this->zoomPercent()) < 0.001) {
        return false;
    }

    const QPointF nextContentPosition
        = viewportInteraction.zoomContentPosition(viewportInteractionSnapshot(),
            viewportContentPosition(), viewportAnchorPoint, nextZoomPercent);
    requestManualZoomPercent(nextZoomPercent);
    return requestViewportInteractionContentPosition(nextContentPosition) > 0;
}

ImageViewportInteractionSnapshot ImageDocumentRuntime::viewportInteractionSnapshot() const
{
    const ImageFirstDisplayDecodeContext firstDisplayContext = firstDisplayDecodeContext();
    const double devicePixelRatio = firstDisplayContext.isValid() && viewportSize().width() > 0.0
        ? firstDisplayContext.physicalViewportSize.width() / viewportSize().width()
        : 1.0;
    return ImageViewportInteractionSnapshot {
        imageSize(),
        viewportSize(),
        displaySize(),
        devicePixelRatio,
        rightToLeftReadingEnabled() && rightToLeftReadingAvailable(),
    };
}

void ImageDocumentRuntime::updateViewportInteractionForPublishedChanges(
    const std::vector<ImageDocumentChange>& changes)
{
    for (ImageDocumentChange change : changes) {
        switch (change) {
        case ImageDocumentChange::Loading:
            if (!loading()) {
                viewportInteraction.cancelPendingDisplayedImageStart();
            }
            break;
        case ImageDocumentChange::DisplayedUrl:
            viewportInteraction.beginDisplayedImage();
            break;
        default:
            break;
        }
    }
}

void ImageDocumentRuntime::loadSource(const ImageDocumentSourceLoadRequest& request)
{
    controllers->dispatchPlan(ImageOpenWorkflow::sourceLoadPlan(
        sourceLoadSnapshot(state, controllers->spreadController()), request));
}

void ImageDocumentRuntime::publishChanges(const std::vector<ImageDocumentChange>& changes)
{
    for (ImageDocumentChange change : changes) {
        controllers->spreadController().handleDocumentChange(change);
    }
    updateViewportInteractionForPublishedChanges(changes);
    invokeIfSet(changeCallback, changes);
}

void ImageDocumentRuntime::shutdown() { controllers->shutdownRuntime(); }

void ImageDocumentRuntime::openPreviousPage()
{
    controllers->navigationController().openAdjacentPage(NavigationDirection::Previous);
}

void ImageDocumentRuntime::openNextPage()
{
    controllers->navigationController().openAdjacentPage(NavigationDirection::Next);
}

void ImageDocumentRuntime::openPreviousSinglePage()
{
    controllers->navigationController().openImageAtRelativePageOffset(-1);
}

void ImageDocumentRuntime::openNextSinglePage()
{
    controllers->navigationController().openImageAtRelativePageOffset(1);
}

void ImageDocumentRuntime::openPreviousContainer()
{
    controllers->navigationController().openAdjacentContainer(NavigationDirection::Previous);
}

void ImageDocumentRuntime::openNextContainer()
{
    controllers->navigationController().openAdjacentContainer(NavigationDirection::Next);
}

void ImageDocumentRuntime::deleteDisplayedFile(FileDeletionMode mode)
{
    controllers->deletionController().deleteDisplayedFile(mode);
}

void ImageDocumentRuntime::resetZoom() { controllers->spreadController().resetZoom(); }

void ImageDocumentRuntime::setFitMode(ImageZoomMode zoomMode)
{
    controllers->spreadController().setFitMode(zoomMode);
}

void ImageDocumentRuntime::rotateClockwise() { controllers->spreadController().rotateClockwise(); }

void ImageDocumentRuntime::rotateCounterclockwise()
{
    controllers->spreadController().rotateCounterclockwise();
}

void ImageDocumentRuntime::updateRenderContext()
{
    controllers->spreadController().updateRenderContext();
}

void ImageDocumentRuntime::openImageAtPage(int pageNumber)
{
    controllers->navigationController().openImageAtPage(pageNumber);
}
}
