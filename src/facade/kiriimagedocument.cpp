// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriimagedocument.h"

#include "decoding/imageformatregistry.h"
#include "document/filedeletion.h"
#include "document/imagedocumentruntime.h"
#include "facade/imagedocumentpublicsignals.h"

#include <memory>
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
    operations.rotationDegreesChanged = [&document]() { Q_EMIT document.rotationDegreesChanged(); };
    operations.documentScopeChanged = [&document]() { Q_EMIT document.documentScopeChanged(); };
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
        [this](const QString &errorString) { Q_EMIT fileDeletionFailed(errorString); });
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

void KiriImageDocument::setViewportContentPosition(const QPointF &viewportContentPosition)
{
    m_runtime->setViewportContentPosition(viewportContentPosition);
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

void KiriImageDocument::setVisibleItemRect(const QRectF &visibleItemRect)
{
    m_runtime->setVisibleItemRect(visibleItemRect);
}

QSizeF KiriImageDocument::displaySize() const { return m_runtime->displaySize(); }

QSizeF KiriImageDocument::primaryDisplaySize() const { return m_runtime->primaryDisplaySize(); }

QSizeF KiriImageDocument::secondaryDisplaySize() const { return m_runtime->secondaryDisplaySize(); }

bool KiriImageDocument::zoomPercentKnown() const { return m_runtime->zoomPercentKnown(); }

double KiriImageDocument::zoomPercent() const { return m_runtime->zoomPercent(); }

void KiriImageDocument::setZoomPercent(double zoomPercent)
{
    m_runtime->setZoomPercent(zoomPercent);
}

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

int KiriImageDocument::imageCount() const { return m_runtime->imageCount(); }

bool KiriImageDocument::containerNavigationAvailable() const
{
    return m_runtime->containerNavigationAvailable();
}

bool KiriImageDocument::ordinaryDirectMediaScopeActive() const
{
    return m_runtime->ordinaryDirectMediaScopeActive();
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

std::optional<KiriView::DisplayedPredecodeImage>
KiriImageDocument::primaryDisplayedPredecodeImage() const
{
    return m_runtime->primaryDisplayedPredecodeImage();
}

KiriView::ImageFirstDisplayDecodeContext KiriImageDocument::firstDisplayDecodeContext() const
{
    return m_runtime->firstDisplayDecodeContext();
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

void KiriImageDocument::openPreviousImage() { m_runtime->openPreviousImage(); }

void KiriImageDocument::openNextImage() { m_runtime->openNextImage(); }

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

void KiriImageDocument::updateRenderContext() { m_runtime->updateRenderContext(); }

void KiriImageDocument::handleDocumentChanges(const std::vector<ImageDocumentChange> &changes)
{
    KiriView::ImageDocumentPublicSignalEmitter(publicSignalOperations(*this)).emitChanges(changes);
}
