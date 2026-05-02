// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimageview.h"

#include "imagedocumentcontroller.h"
#include "imageformatregistry.h"
#include "imageviewportgeometry.h"
#include "kiriimagedecoder.h"
#include "kiriimagerendernode.h"

#include <QQuickWindow>
#include <algorithm>
#include <cmath>
#include <memory>
#include <rhi/qrhi.h>

namespace {
using KiriView::ImageDocumentChange;
using KiriView::ImageDocumentStatus;
using KiriView::ImageZoomMode;

ImageZoomMode toImageZoomMode(KiriImageView::ZoomMode zoomMode)
{
    switch (zoomMode) {
    case KiriImageView::ZoomMode::Fit:
        return ImageZoomMode::Fit;
    case KiriImageView::ZoomMode::FitHeight:
        return ImageZoomMode::FitHeight;
    case KiriImageView::ZoomMode::FitWidth:
        return ImageZoomMode::FitWidth;
    case KiriImageView::ZoomMode::Manual:
        return ImageZoomMode::Manual;
    }

    return ImageZoomMode::Fit;
}

KiriImageView::ZoomMode fromImageZoomMode(ImageZoomMode zoomMode)
{
    switch (zoomMode) {
    case ImageZoomMode::Fit:
        return KiriImageView::ZoomMode::Fit;
    case ImageZoomMode::FitHeight:
        return KiriImageView::ZoomMode::FitHeight;
    case ImageZoomMode::FitWidth:
        return KiriImageView::ZoomMode::FitWidth;
    case ImageZoomMode::Manual:
        return KiriImageView::ZoomMode::Manual;
    }

    return KiriImageView::ZoomMode::Fit;
}

KiriImageView::Status fromImageDocumentStatus(ImageDocumentStatus status)
{
    switch (status) {
    case ImageDocumentStatus::Null:
        return KiriImageView::Status::Null;
    case ImageDocumentStatus::Loading:
        return KiriImageView::Status::Loading;
    case ImageDocumentStatus::Ready:
        return KiriImageView::Status::Ready;
    case ImageDocumentStatus::Error:
        return KiriImageView::Status::Error;
    }

    return KiriImageView::Status::Null;
}
}

KiriImageView::KiriImageView(QQuickItem *parent)
    : QQuickItem(parent)
{
    m_documentController = std::make_unique<KiriView::ImageDocumentController>(
        this, [this]() { return renderContext(); },
        [this](ImageDocumentChange change) { handleDocumentChange(change); });
    setFlag(ItemHasContents, true);
}

KiriImageView::~KiriImageView() = default;

QUrl KiriImageView::sourceUrl() const { return m_documentController->sourceUrl(); }

void KiriImageView::setSourceUrl(const QUrl &sourceUrl)
{
    m_documentController->setSourceUrl(sourceUrl);
}

KiriImageView::Status KiriImageView::status() const
{
    return fromImageDocumentStatus(m_documentController->status());
}

bool KiriImageView::loading() const { return m_documentController->loading(); }

QString KiriImageView::errorString() const { return m_documentController->errorString(); }

QString KiriImageView::windowTitleFileName() const
{
    return m_documentController->windowTitleFileName();
}

QSize KiriImageView::imageSize() const { return m_documentController->imageSize(); }

QSizeF KiriImageView::viewportSize() const { return m_documentController->viewportSize(); }

void KiriImageView::setViewportSize(const QSizeF &viewportSize)
{
    m_documentController->setViewportSize(viewportSize);
}

QSizeF KiriImageView::displaySize() const { return m_documentController->displaySize(); }

double KiriImageView::zoomPercent() const { return m_documentController->zoomPercent(); }

void KiriImageView::setZoomPercent(double zoomPercent)
{
    m_documentController->setZoomPercent(zoomPercent);
}

KiriImageView::ZoomMode KiriImageView::zoomMode() const
{
    return fromImageZoomMode(m_documentController->zoomMode());
}

int KiriImageView::minimumManualZoomPercent() const
{
    return static_cast<int>(KiriView::ImageZoomState::minimumManualZoomPercent);
}

int KiriImageView::maximumManualZoomPercent() const
{
    return static_cast<int>(KiriView::ImageZoomState::maximumManualZoomPercent);
}

int KiriImageView::zoomStepPercent() const
{
    return KiriView::ImageZoomState::manualZoomStepPercent;
}

QStringList KiriImageView::openDialogNameFilters() const
{
    return KiriView::openDialogNameFilters();
}

int KiriImageView::currentPageNumber() const { return m_documentController->currentPageNumber(); }

int KiriImageView::imageCount() const { return m_documentController->imageCount(); }

bool KiriImageView::containerNavigationAvailable() const
{
    return m_documentController->containerNavigationAvailable();
}

void KiriImageView::openPreviousImage() { m_documentController->openPreviousImage(); }

void KiriImageView::openNextImage() { m_documentController->openNextImage(); }

void KiriImageView::openPreviousContainer() { m_documentController->openPreviousContainer(); }

void KiriImageView::openNextContainer() { m_documentController->openNextContainer(); }

void KiriImageView::openImageAtPage(int pageNumber)
{
    m_documentController->openImageAtPage(pageNumber);
}

void KiriImageView::resetZoom() { m_documentController->resetZoom(); }

void KiriImageView::setFitMode(ZoomMode zoomMode)
{
    m_documentController->setFitMode(toImageZoomMode(zoomMode));
}

QPointF KiriImageView::panContentPosition(
    const QPointF &contentPosition, const QPointF &delta) const
{
    const QRectF imageRect = KiriView::imageViewportImageRect(viewportSize(), displaySize());
    return KiriView::imageViewportPanPosition(viewportSize(), imageRect, contentPosition, delta);
}

bool KiriImageView::viewportPointInsideImage(
    const QPointF &contentPosition, const QPointF &viewportPoint) const
{
    const QRectF imageRect = KiriView::imageViewportImageRect(viewportSize(), displaySize());
    return KiriView::imageViewportPointInsideImage(contentPosition, viewportPoint, imageRect);
}

double KiriImageView::clampedManualZoomPercent(double zoomPercent) const
{
    if (!std::isfinite(zoomPercent)) {
        return zoomPercent;
    }

    return std::clamp(zoomPercent, KiriView::ImageZoomState::minimumManualZoomPercent,
        KiriView::ImageZoomState::maximumManualZoomPercent);
}

QPointF KiriImageView::zoomContentPosition(const QPointF &contentPosition,
    const QPointF &viewportAnchorPoint, double nextZoomPercent) const
{
    const QSizeF nextDisplaySize = KiriView::imageViewportDisplaySizeForZoom(
        imageSize(), nextZoomPercent, displayDevicePixelRatio());
    return KiriView::imageViewportContentPositionForZoom(
        viewportSize(), displaySize(), nextDisplaySize, contentPosition, viewportAnchorPoint);
}

QSGNode *KiriImageView::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (m_documentController->image().isNull()) {
        delete oldNode;
        return nullptr;
    }

    const QSizeF boundsSize(width(), height());
    if (boundsSize.isEmpty()) {
        delete oldNode;
        return nullptr;
    }

    auto *node = static_cast<KiriView::KiriImageRenderNode *>(oldNode);
    if (node == nullptr) {
        node = new KiriView::KiriImageRenderNode;
    }

    node->setRhi(window() == nullptr ? nullptr : window()->rhi());
    node->setImage(m_documentController->image(), m_documentController->imageRevision());
    node->setTargetRect(
        KiriView::imageTargetRect(m_documentController->image().size(), boundsSize));
    node->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
    return node;
}

void KiriImageView::itemChange(ItemChange change, const ItemChangeData &value)
{
    QQuickItem::itemChange(change, value);

    if (change == ItemSceneChange || change == ItemDevicePixelRatioHasChanged) {
        m_documentController->updateRenderContext();
    }
}

void KiriImageView::handleDocumentChange(ImageDocumentChange change)
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
        update();
        return;
    }
}

KiriView::ImageDocumentRenderContext KiriImageView::renderContext() const
{
    return KiriView::ImageDocumentRenderContext { displayDevicePixelRatio(), maximumTextureSize() };
}

qreal KiriImageView::displayDevicePixelRatio() const
{
    const QQuickWindow *quickWindow = window();
    if (quickWindow == nullptr) {
        return 1.0;
    }

    const qreal devicePixelRatio = quickWindow->effectiveDevicePixelRatio();
    if (!std::isfinite(devicePixelRatio) || devicePixelRatio <= 0.0) {
        return 1.0;
    }

    return devicePixelRatio;
}

int KiriImageView::maximumTextureSize() const
{
    const QQuickWindow *quickWindow = window();
    QRhi *rhi = quickWindow == nullptr ? nullptr : quickWindow->rhi();
    if (rhi == nullptr) {
        return KiriView::fallbackTextureSizeMax;
    }

    const int maximumTextureSize = rhi->resourceLimit(QRhi::TextureSizeMax);
    return maximumTextureSize > 0 ? maximumTextureSize : KiriView::fallbackTextureSizeMax;
}
