// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedocument.h"

#include "filedeletion.h"
#include "imageasyncdependencies.h"
#include "imagedocumentcontroller.h"
#include "imagedocumentnotifications.h"
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
    m_documentController = std::make_unique<KiriView::ImageDocumentController>(
        this, [this]() { return renderContext(); },
        [this](ImageDocumentChange change) { handleDocumentChange(change); },
        KiriView::defaultImageAsyncDependencies(),
        [this](const QString &errorString) { Q_EMIT fileDeletionFailed(errorString); });
}

KiriImageDocument::~KiriImageDocument() = default;

QUrl KiriImageDocument::sourceUrl() const { return m_documentController->sourceUrl(); }

void KiriImageDocument::setSourceUrl(const QUrl &sourceUrl)
{
    m_documentController->setSourceUrl(sourceUrl);
}

KiriImageDocument::Status KiriImageDocument::status() const
{
    return fromImageDocumentStatus(m_documentController->status());
}

bool KiriImageDocument::loading() const { return m_documentController->loading(); }

QString KiriImageDocument::errorString() const { return m_documentController->errorString(); }

QString KiriImageDocument::windowTitleFileName() const
{
    return m_documentController->windowTitleFileName();
}

QUrl KiriImageDocument::displayedUrl() const { return m_documentController->displayedUrl(); }

QSize KiriImageDocument::imageSize() const { return m_documentController->imageSize(); }

QSize KiriImageDocument::primaryImageSize() const
{
    return m_documentController->primaryImageSize();
}

QSize KiriImageDocument::secondaryImageSize() const
{
    return m_documentController->secondaryImageSize();
}

QSizeF KiriImageDocument::viewportSize() const { return m_documentController->viewportSize(); }

void KiriImageDocument::setViewportSize(const QSizeF &viewportSize)
{
    m_documentController->setViewportSize(viewportSize);
}

QRectF KiriImageDocument::visibleItemRect() const
{
    return m_documentController->visibleItemRect();
}

void KiriImageDocument::setVisibleItemRect(const QRectF &visibleItemRect)
{
    m_documentController->setVisibleItemRect(visibleItemRect);
}

QSizeF KiriImageDocument::displaySize() const { return m_documentController->displaySize(); }

QSizeF KiriImageDocument::primaryDisplaySize() const
{
    return m_documentController->primaryDisplaySize();
}

QSizeF KiriImageDocument::secondaryDisplaySize() const
{
    return m_documentController->secondaryDisplaySize();
}

double KiriImageDocument::zoomPercent() const { return m_documentController->zoomPercent(); }

void KiriImageDocument::setZoomPercent(double zoomPercent)
{
    m_documentController->setZoomPercent(zoomPercent);
}

KiriImageDocument::ZoomMode KiriImageDocument::zoomMode() const
{
    return fromImageZoomMode(m_documentController->zoomMode());
}

int KiriImageDocument::rotationDegrees() const { return m_documentController->rotationDegrees(); }

int KiriImageDocument::minimumManualZoomPercent() const
{
    return static_cast<int>(KiriView::ImageZoomState::minimumManualZoomPercent());
}

int KiriImageDocument::maximumManualZoomPercent() const
{
    return KiriView::ImageZoomState::manualZoomPercentPropertyValue(
        m_documentController->maximumManualZoomPercent());
}

double KiriImageDocument::zoomStepFactor() const
{
    return KiriView::ImageZoomState::manualZoomStepFactor();
}

QStringList KiriImageDocument::openDialogNameFilters() const
{
    return KiriView::openDialogNameFilters();
}

int KiriImageDocument::currentPageNumber() const
{
    return m_documentController->currentPageNumber();
}

int KiriImageDocument::currentLastPageNumber() const
{
    return m_documentController->currentLastPageNumber();
}

int KiriImageDocument::imageCount() const { return m_documentController->imageCount(); }

bool KiriImageDocument::containerNavigationAvailable() const
{
    return m_documentController->containerNavigationAvailable();
}

bool KiriImageDocument::fileDeletionInProgress() const
{
    return m_documentController->fileDeletionInProgress();
}

bool KiriImageDocument::twoPageModeEnabled() const
{
    return m_documentController->twoPageModeEnabled();
}

void KiriImageDocument::setTwoPageModeEnabled(bool enabled)
{
    m_documentController->setTwoPageModeEnabled(enabled);
}

bool KiriImageDocument::twoPageModeAvailable() const
{
    return m_documentController->twoPageModeAvailable();
}

bool KiriImageDocument::rightToLeftReadingEnabled() const
{
    return m_documentController->rightToLeftReadingEnabled();
}

void KiriImageDocument::setRightToLeftReadingEnabled(bool enabled)
{
    m_documentController->setRightToLeftReadingEnabled(enabled);
}

bool KiriImageDocument::rightToLeftReadingAvailable() const
{
    return m_documentController->rightToLeftReadingAvailable();
}

bool KiriImageDocument::secondaryPageVisible() const
{
    return m_documentController->secondaryPageVisible();
}

KiriView::DisplayedImageRenderSnapshot KiriImageDocument::renderSnapshot(
    KiriView::DisplayedPageRole role) const
{
    return m_documentController->renderSnapshot(role);
}

const QImage &KiriImageDocument::image() const { return m_documentController->image(); }

void KiriImageDocument::setRenderContextProvider(RenderContextProvider provider)
{
    m_renderContextProvider = std::move(provider);
    updateRenderContext();
}

void KiriImageDocument::openPreviousImage() { m_documentController->openPreviousImage(); }

void KiriImageDocument::openNextImage() { m_documentController->openNextImage(); }

void KiriImageDocument::openPreviousSinglePage() { m_documentController->openPreviousSinglePage(); }

void KiriImageDocument::openNextSinglePage() { m_documentController->openNextSinglePage(); }

void KiriImageDocument::openPreviousContainer() { m_documentController->openPreviousContainer(); }

void KiriImageDocument::openNextContainer() { m_documentController->openNextContainer(); }

void KiriImageDocument::deleteDisplayedFile(DeletionMode mode)
{
    m_documentController->deleteDisplayedFile(toFileDeletionMode(mode));
}

void KiriImageDocument::openImageAtPage(int pageNumber)
{
    m_documentController->openImageAtPage(pageNumber);
}

void KiriImageDocument::resetZoom() { m_documentController->resetZoom(); }

void KiriImageDocument::setFitMode(ZoomMode zoomMode)
{
    m_documentController->setFitMode(toImageZoomMode(zoomMode));
}

void KiriImageDocument::rotateClockwise() { m_documentController->rotateClockwise(); }

void KiriImageDocument::rotateCounterclockwise() { m_documentController->rotateCounterclockwise(); }

double KiriImageDocument::clampedManualZoomPercent(double zoomPercent) const
{
    return m_documentController->clampedManualZoomPercent(zoomPercent);
}

double KiriImageDocument::steppedManualZoomPercent(double stepCount) const
{
    return m_documentController->steppedManualZoomPercent(stepCount);
}

void KiriImageDocument::updateRenderContext() { m_documentController->updateRenderContext(); }

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
