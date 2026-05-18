// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedocument.h"

#include "filedeletion.h"
#include "imagedocumentnotifications.h"
#include "imagedocumentruntime.h"
#include "imageformatregistry.h"

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
}

KiriImageDocument::KiriImageDocument(QObject *parent)
    : QObject(parent)
{
    m_runtime = std::make_unique<KiriView::ImageDocumentRuntime>(
        this, [this]() { return renderContext(); },
        [this](ImageDocumentChange change) { handleDocumentChange(change); },
        KiriView::ImageAsyncDependencies {},
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

QRectF KiriImageDocument::visibleItemRect() const { return m_runtime->visibleItemRect(); }

void KiriImageDocument::setVisibleItemRect(const QRectF &visibleItemRect)
{
    m_runtime->setVisibleItemRect(visibleItemRect);
}

QSizeF KiriImageDocument::displaySize() const { return m_runtime->displaySize(); }

QSizeF KiriImageDocument::primaryDisplaySize() const { return m_runtime->primaryDisplaySize(); }

QSizeF KiriImageDocument::secondaryDisplaySize() const { return m_runtime->secondaryDisplaySize(); }

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

KiriView::DisplayedImageRenderSnapshot KiriImageDocument::renderSnapshot(
    KiriView::DisplayedPageRole role) const
{
    return m_runtime->renderSnapshot(role);
}

void KiriImageDocument::setRenderContextProvider(RenderContextProvider provider)
{
    m_renderContextProvider = std::move(provider);
    updateRenderContext();
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

void KiriImageDocument::handleDocumentChange(ImageDocumentChange change)
{
    for (KiriView::ImageDocumentPublicSignal signal :
        KiriView::imageDocumentPublicSignals(change)) {
        emitDocumentSignal(signal);
    }
}

void KiriImageDocument::emitDocumentSignal(KiriView::ImageDocumentPublicSignal signal)
{
    switch (signal) {
    case KiriView::ImageDocumentPublicSignal::SourceUrl:
        Q_EMIT sourceUrlChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::Status:
        Q_EMIT statusChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::Loading:
        Q_EMIT loadingChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::ErrorString:
        Q_EMIT errorStringChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::WindowTitleFileName:
        Q_EMIT windowTitleFileNameChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::DisplayedUrl:
        Q_EMIT displayedUrlChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::ImageSize:
        Q_EMIT imageSizeChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::ViewportSize:
        Q_EMIT viewportSizeChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::VisibleItemRect:
        Q_EMIT visibleItemRectChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::DisplaySize:
        Q_EMIT displaySizeChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::ZoomPercent:
        Q_EMIT zoomPercentChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::ZoomMode:
        Q_EMIT zoomModeChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::MaximumManualZoomPercent:
        Q_EMIT maximumManualZoomPercentChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::PageNavigation:
        Q_EMIT pageNavigationChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::ContainerNavigation:
        Q_EMIT containerNavigationChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::FileDeletionInProgress:
        Q_EMIT fileDeletionInProgressChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::TwoPageMode:
        Q_EMIT twoPageModeChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::RightToLeftReading:
        Q_EMIT rightToLeftReadingChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::RotationDegrees:
        Q_EMIT rotationDegreesChanged();
        return;
    case KiriView::ImageDocumentPublicSignal::Repaint:
        Q_EMIT repaintRequested();
        return;
    }
}

KiriView::ImageDocumentRenderContext KiriImageDocument::renderContext() const
{
    if (m_renderContextProvider) {
        return m_renderContextProvider();
    }

    return KiriView::ImageDocumentRenderContext {};
}
