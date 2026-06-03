// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriimagedocument.h"

#include "decoding/imageformatregistry.h"
#include "document/imagedocumentruntime.h"
#include "facade/imagedocumentpublicsignals.h"
#include "location/imagelocation.h"
#include "system/filedeletion.h"

#include <cmath>
#include <memory>
#include <optional>
#include <utility>

namespace {
using KiriView::ImageDocumentChange;
using KiriView::ImageDocumentStatus;
using KiriView::ImageZoomMode;

ImageZoomMode toImageZoomMode(KiriImageDocument::ZoomMode zoomMode)
{
    switch (zoomMode) {
    case KiriImageDocument::ZoomMode::Fit:
        return ImageZoomMode::Fit;
    case KiriImageDocument::ZoomMode::FitHeight:
        return ImageZoomMode::FitHeight;
    case KiriImageDocument::ZoomMode::FitWidth:
        return ImageZoomMode::FitWidth;
    case KiriImageDocument::ZoomMode::Manual:
        return ImageZoomMode::Manual;
    }

    return ImageZoomMode::Fit;
}

KiriImageDocument::ZoomMode fromImageZoomMode(ImageZoomMode zoomMode)
{
    switch (zoomMode) {
    case ImageZoomMode::Fit:
        return KiriImageDocument::ZoomMode::Fit;
    case ImageZoomMode::FitHeight:
        return KiriImageDocument::ZoomMode::FitHeight;
    case ImageZoomMode::FitWidth:
        return KiriImageDocument::ZoomMode::FitWidth;
    case ImageZoomMode::Manual:
        return KiriImageDocument::ZoomMode::Manual;
    }

    return KiriImageDocument::ZoomMode::Fit;
}

QString viewportRevisionToken(quint64 revision) { return QString::number(revision); }

std::optional<quint64> viewportRevisionFromToken(const QString &token)
{
    bool ok = false;
    const quint64 revision = token.toULongLong(&ok);
    if (!ok) {
        return std::nullopt;
    }

    return revision;
}

bool revisionIsNewerThanToken(quint64 revision, const QString &token)
{
    const std::optional<quint64> tokenRevision = viewportRevisionFromToken(token);
    return !tokenRevision.has_value() || revision > *tokenRevision;
}

KiriView::FileDeletionMode toFileDeletionMode(KiriImageDocument::DeletionMode deletionMode)
{
    switch (deletionMode) {
    case KiriImageDocument::DeletionMode::MoveToTrash:
        return KiriView::FileDeletionMode::MoveToTrash;
    case KiriImageDocument::DeletionMode::DeletePermanently:
        return KiriView::FileDeletionMode::DeletePermanently;
    }

    return KiriView::FileDeletionMode::MoveToTrash;
}

KiriView::ImageViewportObservationOrigin toImageViewportObservationOrigin(
    KiriImageDocument::ViewportObservationOrigin origin)
{
    switch (origin) {
    case KiriImageDocument::ViewportObservationOrigin::Command:
        return KiriView::ImageViewportObservationOrigin::Command;
    case KiriImageDocument::ViewportObservationOrigin::User:
        return KiriView::ImageViewportObservationOrigin::User;
    case KiriImageDocument::ViewportObservationOrigin::Inertia:
        return KiriView::ImageViewportObservationOrigin::Inertia;
    case KiriImageDocument::ViewportObservationOrigin::Overshoot:
        return KiriView::ImageViewportObservationOrigin::Overshoot;
    case KiriImageDocument::ViewportObservationOrigin::Resize:
        return KiriView::ImageViewportObservationOrigin::Resize;
    case KiriImageDocument::ViewportObservationOrigin::Rotation:
        return KiriView::ImageViewportObservationOrigin::Rotation;
    case KiriImageDocument::ViewportObservationOrigin::DevicePixelRatio:
        return KiriView::ImageViewportObservationOrigin::DevicePixelRatio;
    case KiriImageDocument::ViewportObservationOrigin::System:
        return KiriView::ImageViewportObservationOrigin::System;
    }

    return KiriView::ImageViewportObservationOrigin::System;
}

KiriImageDocument::ViewportObservationOrigin fromImageViewportObservationOrigin(
    KiriView::ImageViewportObservationOrigin origin)
{
    switch (origin) {
    case KiriView::ImageViewportObservationOrigin::Command:
        return KiriImageDocument::ViewportObservationOrigin::Command;
    case KiriView::ImageViewportObservationOrigin::User:
        return KiriImageDocument::ViewportObservationOrigin::User;
    case KiriView::ImageViewportObservationOrigin::Inertia:
        return KiriImageDocument::ViewportObservationOrigin::Inertia;
    case KiriView::ImageViewportObservationOrigin::Overshoot:
        return KiriImageDocument::ViewportObservationOrigin::Overshoot;
    case KiriView::ImageViewportObservationOrigin::Resize:
        return KiriImageDocument::ViewportObservationOrigin::Resize;
    case KiriView::ImageViewportObservationOrigin::Rotation:
        return KiriImageDocument::ViewportObservationOrigin::Rotation;
    case KiriView::ImageViewportObservationOrigin::DevicePixelRatio:
        return KiriImageDocument::ViewportObservationOrigin::DevicePixelRatio;
    case KiriView::ImageViewportObservationOrigin::System:
        return KiriImageDocument::ViewportObservationOrigin::System;
    }

    return KiriImageDocument::ViewportObservationOrigin::System;
}

bool pointIsFinite(const QPointF &point)
{
    return std::isfinite(point.x()) && std::isfinite(point.y());
}

KiriImageDocument::PresentationTransitionState fromImagePresentationTransitionState(
    KiriView::ImagePresentationTransitionState state)
{
    switch (state) {
    case KiriView::ImagePresentationTransitionState::PreviousActive:
        return KiriImageDocument::PresentationTransitionState::PreviousActive;
    case KiriView::ImagePresentationTransitionState::TransitioningPlaceholder:
        return KiriImageDocument::PresentationTransitionState::TransitioningPlaceholder;
    case KiriView::ImagePresentationTransitionState::CommittedActive:
        return KiriImageDocument::PresentationTransitionState::CommittedActive;
    }

    return KiriImageDocument::PresentationTransitionState::CommittedActive;
}

KiriImageDocument::Status fromImageDocumentStatus(ImageDocumentStatus status)
{
    switch (status) {
    case ImageDocumentStatus::Null:
        return KiriImageDocument::Status::Null;
    case ImageDocumentStatus::Loading:
        return KiriImageDocument::Status::Loading;
    case ImageDocumentStatus::Ready:
        return KiriImageDocument::Status::Ready;
    case ImageDocumentStatus::Error:
        return KiriImageDocument::Status::Error;
    }

    return KiriImageDocument::Status::Null;
}

KiriView::ImageDocumentPublicSignalOperations publicSignalOperations(KiriImageDocument &document)
{
    KiriView::ImageDocumentPublicSignalOperations operations;
    operations.sourceUrlChanged = [&document]() { Q_EMIT document.sourceUrlChanged(); };
    operations.statusChanged = [&document]() { Q_EMIT document.statusChanged(); };
    operations.loadingChanged = [&document]() { Q_EMIT document.loadingChanged(); };
    operations.errorStringChanged = [&document]() { Q_EMIT document.errorStringChanged(); };
    operations.windowTitleFileNameChanged
        = [&document]() { Q_EMIT document.windowTitleFileNameChanged(); };
    operations.displayedUrlChanged = [&document]() { Q_EMIT document.displayedUrlChanged(); };
    operations.imageSizeChanged = [&document]() { Q_EMIT document.imageSizeChanged(); };
    operations.viewportSizeChanged = [&document]() { Q_EMIT document.viewportSizeChanged(); };
    operations.viewportFrameChanged = [&document]() { Q_EMIT document.viewportFrameChanged(); };
    operations.visibleItemRectChanged = [&document]() { Q_EMIT document.visibleItemRectChanged(); };
    operations.displaySizeChanged = [&document]() { Q_EMIT document.displaySizeChanged(); };
    operations.zoomPercentKnownChanged
        = [&document]() { Q_EMIT document.zoomPercentKnownChanged(); };
    operations.zoomPercentChanged = [&document]() { Q_EMIT document.zoomPercentChanged(); };
    operations.zoomModeChanged = [&document]() { Q_EMIT document.zoomModeChanged(); };
    operations.maximumManualZoomPercentChanged
        = [&document]() { Q_EMIT document.maximumManualZoomPercentChanged(); };
    operations.pageNavigationChanged = [&document]() { Q_EMIT document.pageNavigationChanged(); };
    operations.containerNavigationChanged
        = [&document]() { Q_EMIT document.containerNavigationChanged(); };
    operations.fileDeletionInProgressChanged
        = [&document]() { Q_EMIT document.fileDeletionInProgressChanged(); };
    operations.twoPageModeChanged = [&document]() { Q_EMIT document.twoPageModeChanged(); };
    operations.rightToLeftReadingChanged
        = [&document]() { Q_EMIT document.rightToLeftReadingChanged(); };
    operations.presentationTransitionStateChanged
        = [&document]() { Q_EMIT document.presentationTransitionStateChanged(); };
    operations.rotationDegreesChanged = [&document]() { Q_EMIT document.rotationDegreesChanged(); };
    operations.imageDocumentSourceScopeChanged
        = [&document]() { Q_EMIT document.imageDocumentSourceScopeChanged(); };
    operations.unsupportedOpenedCollectionVideoChanged
        = [&document]() { Q_EMIT document.unsupportedOpenedCollectionVideoChanged(); };
    operations.embeddedMetadataChanged
        = [&document]() { Q_EMIT document.embeddedMetadataChanged(); };
    operations.repaintRequested = [&document]() { Q_EMIT document.repaintRequested(); };
    return operations;
}
}

KiriImageDocument::KiriImageDocument(QObject *parent)
    : KiriImageDocument(KiriView::ImageDocumentRuntimeDependencyOverrides {}, parent)
{
}

KiriImageDocument::KiriImageDocument(
    KiriView::ImageDocumentRuntimeDependencyOverrides dependencies, QObject *parent)
    : QObject(parent)
{
    m_runtime = std::make_unique<KiriView::ImageDocumentRuntime>(
        this, RenderContextProvider {},
        [this](const std::vector<ImageDocumentChange> &changes) { handleDocumentChanges(changes); },
        std::move(dependencies),
        [this](const QString &errorString) { Q_EMIT fileDeletionFailed(errorString); },
        [this](const QString &message) { Q_EMIT unsupportedOpenedCollectionVideoEntered(message); },
        [this](const QString &message) { Q_EMIT containerNavigationBoundaryReached(message); });
}

KiriImageDocument::~KiriImageDocument() = default;

QUrl KiriImageDocument::sourceUrl() const { return m_runtime->sourceUrl(); }

void KiriImageDocument::setSourceUrl(const QUrl &sourceUrl) { m_runtime->setSourceUrl(sourceUrl); }

KiriImageDocument::Status KiriImageDocument::status() const
{
    return fromImageDocumentStatus(m_runtime->status());
}

bool KiriImageDocument::loading() const { return m_runtime->loading(); }

QString KiriImageDocument::errorString() const { return m_runtime->errorString(); }

QString KiriImageDocument::windowTitleFileName() const { return m_runtime->windowTitleFileName(); }

QUrl KiriImageDocument::displayedUrl() const { return m_runtime->displayedUrl(); }

KiriView::OpenedCollectionScopeLocation KiriImageDocument::displayedOpenedCollectionScope() const
{
    return m_runtime->displayedOpenedCollectionScope();
}

QSize KiriImageDocument::imageSize() const { return m_runtime->imageSize(); }

QSize KiriImageDocument::primaryImageSize() const { return m_runtime->primaryImageSize(); }

QSize KiriImageDocument::secondaryImageSize() const { return m_runtime->secondaryImageSize(); }

QSizeF KiriImageDocument::viewportSize() const { return m_runtime->viewportSize(); }

void KiriImageDocument::setViewportSize(const QSizeF &viewportSize)
{
    m_runtime->setViewportSize(viewportSize);
}

QPointF KiriImageDocument::viewportContentPosition() const
{
    return m_runtime->viewportContentPosition();
}

quint64 KiriImageDocument::viewportCommandRevision() const
{
    return m_runtime->viewportCommandRevision();
}

QString KiriImageDocument::viewportCommandRevisionToken() const
{
    return viewportRevisionToken(viewportCommandRevision());
}

quint64 KiriImageDocument::viewportAppliedCommandRevision() const
{
    return m_runtime->viewportAppliedCommandRevision();
}

quint64 KiriImageDocument::viewportObservationRevision() const
{
    return m_runtime->viewportObservationRevision();
}

QString KiriImageDocument::viewportObservationRevisionToken() const
{
    return viewportRevisionToken(viewportObservationRevision());
}

int KiriImageDocument::viewportCommandStatus() const
{
    return static_cast<int>(m_runtime->viewportCommandStatus());
}

KiriImageDocument::ViewportObservationOrigin KiriImageDocument::viewportObservationOrigin() const
{
    return fromImageViewportObservationOrigin(m_runtime->viewportObservationOrigin());
}

QSizeF KiriImageDocument::viewportContentSize() const { return m_runtime->viewportContentSize(); }

QRectF KiriImageDocument::viewportImageRect() const { return m_runtime->viewportImageRect(); }

bool KiriImageDocument::viewportHorizontallyPannable() const
{
    return m_runtime->viewportHorizontallyPannable();
}

bool KiriImageDocument::viewportVerticallyPannable() const
{
    return m_runtime->viewportVerticallyPannable();
}

bool KiriImageDocument::viewportPannable() const { return m_runtime->viewportPannable(); }

QRectF KiriImageDocument::visibleItemRect() const { return m_runtime->visibleItemRect(); }

QSizeF KiriImageDocument::displaySize() const { return m_runtime->displaySize(); }

QSizeF KiriImageDocument::primaryDisplaySize() const { return m_runtime->primaryDisplaySize(); }

QSizeF KiriImageDocument::secondaryDisplaySize() const { return m_runtime->secondaryDisplaySize(); }

bool KiriImageDocument::zoomPercentKnown() const { return m_runtime->zoomPercentKnown(); }

double KiriImageDocument::zoomPercent() const { return m_runtime->zoomPercent(); }

KiriImageDocument::ZoomMode KiriImageDocument::zoomMode() const
{
    return fromImageZoomMode(m_runtime->zoomMode());
}

int KiriImageDocument::rotationDegrees() const { return m_runtime->rotationDegrees(); }

int KiriImageDocument::minimumManualZoomPercent() const
{
    return static_cast<int>(KiriView::ImageZoomState::minimumManualZoomPercent());
}

int KiriImageDocument::maximumManualZoomPercent() const
{
    return KiriView::ImageZoomState::manualZoomPercentPropertyValue(
        m_runtime->maximumManualZoomPercent());
}

double KiriImageDocument::zoomStepFactor() const
{
    return KiriView::ImageZoomState::manualZoomStepFactor();
}

QStringList KiriImageDocument::openDialogNameFilters() const
{
    return KiriView::openDialogNameFilters();
}

int KiriImageDocument::currentPageNumber() const { return m_runtime->currentPageNumber(); }

int KiriImageDocument::currentLastPageNumber() const { return m_runtime->currentLastPageNumber(); }

int KiriImageDocument::pageCount() const { return m_runtime->pageCount(); }

KiriView::ImageDocumentPageNavigationSnapshot KiriImageDocument::pageNavigationSnapshot() const
{
    return m_runtime->pageNavigationSnapshot();
}

KiriView::ImageDocumentPageActiveNavigationSnapshot
KiriImageDocument::activeNavigationSnapshot() const
{
    return m_runtime->activeNavigationSnapshot();
}

bool KiriImageDocument::containerNavigationAvailable() const
{
    return m_runtime->containerNavigationAvailable();
}

bool KiriImageDocument::ordinaryDirectMediaScopeActive() const
{
    return m_runtime->ordinaryDirectMediaScopeActive();
}

bool KiriImageDocument::openedCollectionScopeActive() const
{
    return m_runtime->openedCollectionScopeActive();
}

bool KiriImageDocument::fileDeletionInProgress() const
{
    return m_runtime->fileDeletionInProgress();
}

bool KiriImageDocument::twoPageModeEnabled() const { return m_runtime->twoPageModeEnabled(); }

void KiriImageDocument::setTwoPageModeEnabled(bool enabled)
{
    m_runtime->setTwoPageModeEnabled(enabled);
}

bool KiriImageDocument::twoPageModeAvailable() const { return m_runtime->twoPageModeAvailable(); }

bool KiriImageDocument::rightToLeftReadingEnabled() const
{
    return m_runtime->rightToLeftReadingEnabled();
}

void KiriImageDocument::setRightToLeftReadingEnabled(bool enabled)
{
    m_runtime->setRightToLeftReadingEnabled(enabled);
}

bool KiriImageDocument::rightToLeftReadingAvailable() const
{
    return m_runtime->rightToLeftReadingAvailable();
}

bool KiriImageDocument::secondaryPageVisible() const { return m_runtime->secondaryPageVisible(); }

KiriImageDocument::PresentationTransitionState
KiriImageDocument::presentationTransitionState() const
{
    return fromImagePresentationTransitionState(m_runtime->presentationTransitionState());
}

bool KiriImageDocument::unsupportedOpenedCollectionVideo() const
{
    return m_runtime->unsupportedOpenedCollectionVideo();
}

std::optional<KiriView::DisplayedPredecodeImage>
KiriImageDocument::primaryDisplayedPredecodeImage() const
{
    return m_runtime->primaryDisplayedPredecodeImage();
}

KiriView::ImageFirstDisplayDecodeContext KiriImageDocument::firstDisplayDecodeContext() const
{
    return m_runtime->firstDisplayDecodeContext();
}

const KiriView::EmbeddedMetadata &KiriImageDocument::embeddedMetadata() const
{
    return m_runtime->embeddedMetadata();
}

KiriView::DisplayedImageRenderSnapshot KiriImageDocument::renderSnapshot(
    KiriView::DisplayedPageRole role) const
{
    return m_runtime->renderSnapshot(role);
}

void KiriImageDocument::setRenderContextProvider(RenderContextProvider provider)
{
    m_runtime->setRenderContextProvider(std::move(provider));
}

void KiriImageDocument::openPreviousPage() { m_runtime->openPreviousPage(); }

void KiriImageDocument::openNextPage() { m_runtime->openNextPage(); }

void KiriImageDocument::openPreviousSinglePage() { m_runtime->openPreviousSinglePage(); }

void KiriImageDocument::openNextSinglePage() { m_runtime->openNextSinglePage(); }

void KiriImageDocument::openPreviousContainer() { m_runtime->openPreviousContainer(); }

void KiriImageDocument::openNextContainer() { m_runtime->openNextContainer(); }

void KiriImageDocument::deleteDisplayedFile(DeletionMode mode)
{
    m_runtime->deleteDisplayedFile(toFileDeletionMode(mode));
}

void KiriImageDocument::openImageAtPage(int pageNumber) { m_runtime->openImageAtPage(pageNumber); }

void KiriImageDocument::resetZoom() { m_runtime->resetZoom(); }

void KiriImageDocument::setFitMode(ZoomMode zoomMode)
{
    m_runtime->setFitMode(toImageZoomMode(zoomMode));
}

void KiriImageDocument::rotateClockwise() { m_runtime->rotateClockwise(); }

void KiriImageDocument::rotateCounterclockwise() { m_runtime->rotateCounterclockwise(); }

double KiriImageDocument::clampedManualZoomPercent(double zoomPercent) const
{
    return m_runtime->clampedManualZoomPercent(zoomPercent);
}

double KiriImageDocument::steppedManualZoomPercent(double stepCount) const
{
    return m_runtime->steppedManualZoomPercent(stepCount);
}

bool KiriImageDocument::requestManualZoomPercent(double zoomPercent)
{
    if (status() != Status::Ready) {
        return false;
    }

    return requestAnchoredManualZoom(clampedManualZoomPercent(zoomPercent),
        QPointF(viewportSize().width() / 2.0, viewportSize().height() / 2.0));
}

bool KiriImageDocument::requestZoomByStep(double stepCount, const QPointF &viewportAnchorPoint)
{
    if (status() != Status::Ready || !pointIsFinite(viewportAnchorPoint)) {
        return false;
    }

    const QPointF anchorPoint = m_viewportInteraction.nearestImageViewportPoint(
        viewportInteractionSnapshot(), viewportContentPosition(), viewportAnchorPoint);
    if (!pointIsFinite(anchorPoint)) {
        return false;
    }

    return requestAnchoredManualZoom(steppedManualZoomPercent(stepCount), anchorPoint);
}

bool KiriImageDocument::requestZoomByStepAtCenter(double stepCount)
{
    return requestZoomByStep(
        stepCount, QPointF(viewportSize().width() / 2.0, viewportSize().height() / 2.0));
}

bool KiriImageDocument::requestActualSizeAtCenter()
{
    if (status() != Status::Ready) {
        return false;
    }

    return requestAnchoredManualZoom(clampedManualZoomPercent(100.0),
        QPointF(viewportSize().width() / 2.0, viewportSize().height() / 2.0));
}

bool KiriImageDocument::requestFitMode(ZoomMode zoomMode)
{
    if (status() != Status::Ready) {
        return false;
    }

    if (zoomMode == ZoomMode::Fit) {
        resetZoom();
        return true;
    }

    setFitMode(zoomMode);
    return true;
}

bool KiriImageDocument::requestToggleFitOrActualSize(const QPointF &viewportPoint)
{
    if (status() != Status::Ready || !pointIsFinite(viewportPoint)) {
        return false;
    }

    if (zoomMode() != ZoomMode::Fit) {
        setFitMode(ZoomMode::Fit);
        return true;
    }

    const QPointF anchorPoint = m_viewportInteraction.nearestImageViewportPoint(
        viewportInteractionSnapshot(), viewportContentPosition(), viewportPoint);
    if (!pointIsFinite(anchorPoint)) {
        return false;
    }

    return requestAnchoredManualZoom(clampedManualZoomPercent(100.0), anchorPoint);
}

bool KiriImageDocument::requestViewportPanBy(double deltaX, double deltaY)
{
    if (!viewportPannable()) {
        return false;
    }

    const QPointF nextContentPosition = m_viewportInteraction.panContentPosition(
        viewportInteractionSnapshot(), viewportContentPosition(), QPointF(deltaX, deltaY));
    return requestViewportInteractionContentPosition(nextContentPosition);
}

bool KiriImageDocument::requestViewportPanToInitialScanPosition()
{
    if (!viewportPannable()) {
        return false;
    }

    return requestViewportInteractionContentPosition(
        m_viewportInteraction.initialScanContentPosition(viewportInteractionSnapshot()));
}

bool KiriImageDocument::requestViewportPanToFinalScanPosition()
{
    if (!viewportPannable()) {
        return false;
    }

    return requestViewportInteractionContentPosition(
        m_viewportInteraction.finalScanContentPosition(viewportInteractionSnapshot()));
}

bool KiriImageDocument::requestViewportScanForward()
{
    if (!viewportPannable()) {
        return false;
    }

    return requestViewportInteractionContentPosition(m_viewportInteraction.nextScanContentPosition(
        viewportInteractionSnapshot(), viewportContentPosition()));
}

bool KiriImageDocument::requestViewportScanBackward()
{
    if (!viewportPannable()) {
        return false;
    }

    return requestViewportInteractionContentPosition(
        m_viewportInteraction.previousScanContentPosition(
            viewportInteractionSnapshot(), viewportContentPosition()));
}

void KiriImageDocument::requestNextDisplayedImageStartToFinalScanPosition()
{
    m_viewportInteraction.requestNextDisplayedImageFinalScanStart();
}

bool KiriImageDocument::requestDisplayedImageInitialContentPosition()
{
    return requestViewportInteractionContentPosition(
        m_viewportInteraction.displayedImageInitialContentPosition(viewportInteractionSnapshot()));
}

void KiriImageDocument::requestToggleTwoPageMode() { setTwoPageModeEnabled(!twoPageModeEnabled()); }

void KiriImageDocument::requestToggleRightToLeftReading()
{
    setRightToLeftReadingEnabled(!rightToLeftReadingEnabled());
}

void KiriImageDocument::updateRenderContext() { m_runtime->updateRenderContext(); }

bool KiriImageDocument::requestViewportContentPosition(const QPointF &viewportContentPosition)
{
    return m_runtime->requestViewportContentPosition(viewportContentPosition) > 0;
}

bool KiriImageDocument::viewportCommandRevisionNewerThan(const QString &revisionToken) const
{
    return revisionIsNewerThanToken(viewportCommandRevision(), revisionToken);
}

bool KiriImageDocument::viewportProjectionNewerThan(
    const QString &commandRevisionToken, const QString &observationRevisionToken) const
{
    const std::optional<quint64> commandRevision = viewportRevisionFromToken(commandRevisionToken);
    const std::optional<quint64> observationRevision
        = viewportRevisionFromToken(observationRevisionToken);
    if (!commandRevision.has_value() || !observationRevision.has_value()) {
        return true;
    }

    const quint64 currentCommandRevision = viewportCommandRevision();
    return currentCommandRevision > *commandRevision
        || (currentCommandRevision == *commandRevision
            && viewportObservationRevision() > *observationRevision);
}

bool KiriImageDocument::beginViewportCommandApplication(quint64 commandRevision)
{
    return m_runtime->beginViewportCommandApplication(commandRevision);
}

bool KiriImageDocument::beginViewportCommandApplication(const QString &commandRevisionToken)
{
    const std::optional<quint64> commandRevision = viewportRevisionFromToken(commandRevisionToken);
    return commandRevision.has_value() && beginViewportCommandApplication(*commandRevision);
}

bool KiriImageDocument::completeViewportCommandApplication(
    quint64 commandRevision, const QPointF &actualContentPosition)
{
    return m_runtime->completeViewportCommandApplication(commandRevision, actualContentPosition);
}

bool KiriImageDocument::completeViewportCommandApplication(
    const QString &commandRevisionToken, const QPointF &actualContentPosition)
{
    const std::optional<quint64> commandRevision = viewportRevisionFromToken(commandRevisionToken);
    return commandRevision.has_value()
        && completeViewportCommandApplication(*commandRevision, actualContentPosition);
}

bool KiriImageDocument::acknowledgeViewportCommand(
    quint64 commandRevision, const QPointF &actualContentPosition)
{
    return m_runtime->acknowledgeViewportCommand(commandRevision, actualContentPosition);
}

bool KiriImageDocument::acknowledgeViewportCommand(
    const QString &commandRevisionToken, const QPointF &actualContentPosition)
{
    const std::optional<quint64> commandRevision = viewportRevisionFromToken(commandRevisionToken);
    return commandRevision.has_value()
        && acknowledgeViewportCommand(*commandRevision, actualContentPosition);
}

bool KiriImageDocument::observeViewportContentPosition(
    const QPointF &contentPosition, KiriImageDocument::ViewportObservationOrigin origin)
{
    return m_runtime->observeViewportContentPosition(
        contentPosition, toImageViewportObservationOrigin(origin));
}

KiriView::ImageViewportInteractionSnapshot KiriImageDocument::viewportInteractionSnapshot() const
{
    return KiriView::ImageViewportInteractionSnapshot {
        imageSize(),
        viewportSize(),
        displaySize(),
        renderSnapshot().devicePixelRatio,
        rightToLeftReadingEnabled() && rightToLeftReadingAvailable(),
    };
}

bool KiriImageDocument::requestViewportInteractionContentPosition(const QPointF &contentPosition)
{
    if (!pointIsFinite(contentPosition)) {
        return false;
    }

    const QPointF previousContentPosition = viewportContentPosition();
    const quint64 previousCommandRevision = viewportCommandRevision();
    const quint64 commandRevision = m_runtime->requestViewportContentPosition(contentPosition);
    return previousContentPosition != viewportContentPosition()
        || commandRevision > previousCommandRevision;
}

bool KiriImageDocument::requestAnchoredManualZoom(
    double zoomPercent, const QPointF &viewportAnchorPoint)
{
    if (!pointIsFinite(viewportAnchorPoint)) {
        return false;
    }

    const double nextZoomPercent = clampedManualZoomPercent(zoomPercent);
    if (std::abs(nextZoomPercent - this->zoomPercent()) < 0.001) {
        return false;
    }

    const QPointF nextContentPosition
        = m_viewportInteraction.zoomContentPosition(viewportInteractionSnapshot(),
            viewportContentPosition(), viewportAnchorPoint, nextZoomPercent);
    m_runtime->requestManualZoomPercent(nextZoomPercent);
    requestViewportInteractionContentPosition(nextContentPosition);
    return true;
}

void KiriImageDocument::handleDocumentChanges(const std::vector<ImageDocumentChange> &changes)
{
    KiriView::ImageDocumentPublicSignalEmitter(publicSignalOperations(*this)).emitChanges(changes);
}
