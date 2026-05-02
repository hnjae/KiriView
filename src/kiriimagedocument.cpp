// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedocument.h"

#include "imagedocumentcontroller.h"
#include "imageformatregistry.h"

#include <algorithm>
#include <cmath>
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
        [this](ImageDocumentChange change) { handleDocumentChange(change); });
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

QSize KiriImageDocument::imageSize() const { return m_documentController->imageSize(); }

QSizeF KiriImageDocument::viewportSize() const { return m_documentController->viewportSize(); }

void KiriImageDocument::setViewportSize(const QSizeF &viewportSize)
{
    m_documentController->setViewportSize(viewportSize);
}

QSizeF KiriImageDocument::displaySize() const { return m_documentController->displaySize(); }

double KiriImageDocument::zoomPercent() const { return m_documentController->zoomPercent(); }

void KiriImageDocument::setZoomPercent(double zoomPercent)
{
    m_documentController->setZoomPercent(zoomPercent);
}

KiriImageDocument::ZoomMode KiriImageDocument::zoomMode() const
{
    return fromImageZoomMode(m_documentController->zoomMode());
}

int KiriImageDocument::minimumManualZoomPercent() const
{
    return static_cast<int>(KiriView::ImageZoomState::minimumManualZoomPercent);
}

int KiriImageDocument::maximumManualZoomPercent() const
{
    return static_cast<int>(KiriView::ImageZoomState::maximumManualZoomPercent);
}

int KiriImageDocument::zoomStepPercent() const
{
    return KiriView::ImageZoomState::manualZoomStepPercent;
}

QStringList KiriImageDocument::openDialogNameFilters() const
{
    return KiriView::openDialogNameFilters();
}

int KiriImageDocument::currentPageNumber() const
{
    return m_documentController->currentPageNumber();
}

int KiriImageDocument::imageCount() const { return m_documentController->imageCount(); }

bool KiriImageDocument::containerNavigationAvailable() const
{
    return m_documentController->containerNavigationAvailable();
}

const QImage &KiriImageDocument::image() const { return m_documentController->image(); }

quint64 KiriImageDocument::imageRevision() const { return m_documentController->imageRevision(); }

void KiriImageDocument::setRenderContextProvider(RenderContextProvider provider)
{
    m_renderContextProvider = std::move(provider);
    updateRenderContext();
}

void KiriImageDocument::openPreviousImage() { m_documentController->openPreviousImage(); }

void KiriImageDocument::openNextImage() { m_documentController->openNextImage(); }

void KiriImageDocument::openPreviousContainer() { m_documentController->openPreviousContainer(); }

void KiriImageDocument::openNextContainer() { m_documentController->openNextContainer(); }

void KiriImageDocument::openImageAtPage(int pageNumber)
{
    m_documentController->openImageAtPage(pageNumber);
}

void KiriImageDocument::resetZoom() { m_documentController->resetZoom(); }

void KiriImageDocument::setFitMode(ZoomMode zoomMode)
{
    m_documentController->setFitMode(toImageZoomMode(zoomMode));
}

double KiriImageDocument::clampedManualZoomPercent(double zoomPercent) const
{
    if (!std::isfinite(zoomPercent)) {
        return zoomPercent;
    }

    return std::clamp(zoomPercent, KiriView::ImageZoomState::minimumManualZoomPercent,
        KiriView::ImageZoomState::maximumManualZoomPercent);
}

void KiriImageDocument::updateRenderContext() { m_documentController->updateRenderContext(); }

void KiriImageDocument::handleDocumentChange(ImageDocumentChange change)
{
    switch (change) {
    case ImageDocumentChange::SourceUrl:
        Q_EMIT sourceUrlChanged();
        return;
    case ImageDocumentChange::Status:
        Q_EMIT statusChanged();
        return;
    case ImageDocumentChange::Loading:
        Q_EMIT loadingChanged();
        return;
    case ImageDocumentChange::ErrorString:
        Q_EMIT errorStringChanged();
        return;
    case ImageDocumentChange::WindowTitleFileName:
        Q_EMIT windowTitleFileNameChanged();
        return;
    case ImageDocumentChange::ImageSize:
        Q_EMIT imageSizeChanged();
        return;
    case ImageDocumentChange::ViewportSize:
        Q_EMIT viewportSizeChanged();
        return;
    case ImageDocumentChange::DisplaySize:
        Q_EMIT displaySizeChanged();
        return;
    case ImageDocumentChange::ZoomPercent:
        Q_EMIT zoomPercentChanged();
        return;
    case ImageDocumentChange::ZoomMode:
        Q_EMIT zoomModeChanged();
        return;
    case ImageDocumentChange::PageNavigation:
        Q_EMIT pageNavigationChanged();
        return;
    case ImageDocumentChange::ContainerNavigation:
        Q_EMIT containerNavigationChanged();
        return;
    case ImageDocumentChange::Repaint:
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
